#include "CppProfilerRunner.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include "Debugger.hpp"
#include "ProcessMemory.hpp"

//-------------------------------------------------------------------------
CppProfilerRunner::CppProfilerRunner(SymbolInfoPDB& symtab, std::wstring ofname)
	: symtab(symtab),
	  originalCode(),
	  outf (ofname)
{
	exceptionHandler_ = std::make_unique<ExceptionHandler>();
	singleStep = 0;
	boolean ok = outf.good();
}

//-------------------------------------------------------------------------
CppProfilerRunner::~CppProfilerRunner()
{
	outf.close();
}

//-------------------------------------------------------------------------
int CppProfilerRunner::Run(const StartInfo& startInfo)
{
	Debugger debugger{ true, true};

	int exitCode = debugger.Debug(startInfo, *this);
	return exitCode;
}

//-------------------------------------------------------------------------
void CppProfilerRunner::OnCreateProcess(const CREATE_PROCESS_DEBUG_INFO& processDebugInfo)
{
	hProcess = processDebugInfo.hProcess;
	auto lpBaseOfImage = processDebugInfo.lpBaseOfImage;
	baseOfImage = (DWORD)lpBaseOfImage;

	installTracepoints();

	LoadModule(hProcess, processDebugInfo.hFile, lpBaseOfImage);
}

//-------------------------------------------------------------------------
void CppProfilerRunner::OnExitProcess(HANDLE hProcess, HANDLE, const EXIT_PROCESS_DEBUG_INFO&)
{
}

//-------------------------------------------------------------------------
void CppProfilerRunner::OnLoadDll(
	HANDLE hProcess, 
	HANDLE hThread, 
	const LOAD_DLL_DEBUG_INFO& dllDebugInfo)
{
	LoadModule(hProcess, dllDebugInfo.hFile, dllDebugInfo.lpBaseOfDll);
}

//-------------------------------------------------------------------------
void CppProfilerRunner::OnUnloadDll(
	HANDLE hProcess,
	HANDLE hThread,
	const UNLOAD_DLL_DEBUG_INFO& unloadDllDebugInfo)
{
}

//-------------------------------------------------------------------------
IDebugEventsHandler::ExceptionType CppProfilerRunner::OnException(
	HANDLE hProcess, 
	HANDLE hThread, 
	const EXCEPTION_DEBUG_INFO& exceptionDebugInfo)
{
	std::wostringstream ostr;
	
	auto status = exceptionHandler_->HandleException(hProcess, exceptionDebugInfo, ostr);

	switch (status)
	{
		case ExceptionHandlerStatus::BreakPoint:
		{
			if (OnBreakPoint(exceptionDebugInfo, hProcess, hThread))
				return IDebugEventsHandler::ExceptionType::BreakPoint;
			return IDebugEventsHandler::ExceptionType::InvalidBreakPoint;
		}
		case ExceptionHandlerStatus::FirstChanceException:
		{
			return IDebugEventsHandler::ExceptionType::NotHandled;
		}
		case ExceptionHandlerStatus::Error:
		{
			std::cerr << ostr.str().c_str();
			
			return IDebugEventsHandler::ExceptionType::Error;
		}
		case ExceptionHandlerStatus::CppError:
		{
			std::cerr << ostr.str().c_str();

			return IDebugEventsHandler::ExceptionType::CppError;
		}
	}

	return IDebugEventsHandler::ExceptionType::NotHandled;
}

//-------------------------------------------------------------------------
bool CppProfilerRunner::OnBreakPoint(
	const EXCEPTION_DEBUG_INFO& exceptionDebugInfo,
	HANDLE hProcess,
	HANDLE hThread)
{
	const auto& exceptionRecord = exceptionDebugInfo.ExceptionRecord;
	DWORD address = (DWORD) exceptionRecord.ExceptionAddress;

	if (singleStep > 0)
	{
//		outf << "singleStep: " << address << "  " << singleStepAddress << std::endl;

		UCHAR brkinstruction = 0xcc; // re-emplace breakpoint after the original instruction was executed
		WriteProcessMemory(hProcess, singleStepAddress, &brkinstruction, 1);
		singleStep = false;
		return true;
	}
	else {
		auto i = this->symtab.tracepoints.find(address - this->baseOfImage);
		if (i != this->symtab.tracepoints.cend())
		{
			outf << "enter: " << std::setw(8) << std::setfill('0') << std::hex << address << "  " << i->second.c_str() << std::endl;

			RemoveBreakPoint(hProcess, address); // temporarily restore the original instruction

			CONTEXT lcContext;
			lcContext.ContextFlags = CONTEXT_ALL;
			if (!GetThreadContext(hThread, &lcContext)) throw ("Error in GetThreadContext");

			--lcContext.Eip; // Move back one byte. Will executes the restored instruction.
			lcContext.EFlags |= 0x100; // Set trap flag. Enforces SingleStep exception once.
			singleStep = 1;
			singleStepAddress = address;

			// examine stack to access return address (assuming last instruction was a CALL)
			DWORD retAddress;
			ReadProcessMemory(hProcess, lcContext.Esp, &retAddress, 4U);
			this->returnStack.push_back(retAddress);
			WriteBreakPoint(hProcess, retAddress);

			if (!SetThreadContext(hThread, &lcContext))	throw ("Error in SetThreadContext");
			return true;
		}
		else if (!returnStack.empty() && address == returnStack.back())
		{
			outf << "exit: " << std::setw(8) << std::setfill('0') << std::hex << address << std::endl;

			if (singleStep > 0) throw (L"unexpected singleStep exception");
			RemoveBreakPoint(hProcess, address);
			returnStack.pop_back();

			CONTEXT lcContext;
			lcContext.ContextFlags = CONTEXT_ALL;
			if (!GetThreadContext(hThread, &lcContext)) throw ("Error in GetThreadContext #2");
			--lcContext.Eip; // Move back one byte. Will execute the restored instruction.
			// if address is still on the returnStack (due to recursive call) we need to re-install the breakpoint
			if (findOnReturnStack (address))
			{
				lcContext.EFlags |= 0x100; // Set trap flag. Enforces SingleStep exception once.
				singleStep = 2;
				singleStepAddress = address;
			}
			if (!SetThreadContext(hThread, &lcContext))	throw ("Error in SetThreadContext #2");

			return true;
		}
		else {
			throw (L"unexpected breakpoint interrupt. address not found.");
		}
	}

	return false;
}

//-------------------------------------------------------------------------
void CppProfilerRunner::LoadModule(HANDLE hProcess,
                                    HANDLE hFile,
                                    void* baseOfImage)
{
	/* TODO
	HandleInformation handleInformation;

	std::wstring filename = handleInformation.ComputeFilename(hFile);

	auto isSelected = coverageFilterManager_->IsModuleSelected(filename);
	if (isSelected)
	{
		isSelected = monitoredLineRegister_->RegisterLineToMonitor(
		    filename, hProcess, baseOfImage);
	}
	filterAssistant_->OnNewModule(filename, isSelected);
	*/
}

//-------------------------------------------------------------------------
void CppProfilerRunner::WriteBreakPoint(HANDLE hProcess, DWORD address)
{
	std::vector<UCHAR> buf = ReadProcessMemory(hProcess, address, 1);
	this->originalCode.emplace(address, buf[0]);
	UCHAR brkinstruction = 0xcc;
	WriteProcessMemory(hProcess, address, &brkinstruction, 1);
}

//-------------------------------------------------------------------------
void CppProfilerRunner::RemoveBreakPoint(HANDLE hProcess, DWORD address)
{
	auto i = this->originalCode.find(address);
	if (i == this->originalCode.cend()) throw (L"originalCode not found");
	UCHAR code = i->second;
	WriteProcessMemory(hProcess, address, &code, 1U);
}

//-------------------------------------------------------------------------
void CppProfilerRunner::installTracepoints()
{
	for (auto i = symtab.tracepoints.cbegin(); i != symtab.tracepoints.cend(); ++i) {
		DWORD address = i->first;
		address += baseOfImage;
		WriteBreakPoint(hProcess, address);
		//		std::vector<UCHAR> buf = ReadProcessMemory (hProcess, address, 1);
		//		this->originalCode.emplace(address, buf[0]);
		//		UCHAR brkinstruction = 0xcc;
		//		WriteProcessMemory(hProcess, address, &brkinstruction, 1);
	}
	singleStep = false;
}

//-------------------------------------------------------------------------
boolean CppProfilerRunner::findOnReturnStack(DWORD address)
{
	bool found = false;
	for each (DWORD addr in this->returnStack) {
		found |= (addr == address);
	}
	return found;
}
