#include "Debugger.hpp"
#include "Process.hpp"
#include "IDebugEventsHandler.hpp"
#include <iostream>

//-------------------------------------------------------------------------
namespace
{
	void OnRip(const RIP_INFO& ripInfo)
	{
		std::cerr << "Debugee process terminate unexpectedly:"	<< "(type:" << ripInfo.dwType << ")" << ripInfo.dwError;
	}
}	

//-------------------------------------------------------------------------
struct Debugger::ProcessStatus
{
	ProcessStatus() = default;

	ProcessStatus(
		MayBe<int> exitCode,
		MayBe<DWORD> continueStatus)
		: exitCode_{ exitCode }
		, continueStatus_{ continueStatus }
	{
	}

	MayBe<int> exitCode_;
	MayBe<DWORD> continueStatus_;
};

//-------------------------------------------------------------------------
Debugger::Debugger(
	bool coverChildren,
	bool continueAfterCppException)
	: coverChildren_{ coverChildren }
	, continueAfterCppException_{ continueAfterCppException }
{
}

//-------------------------------------------------------------------------
int Debugger::Debug(
	const StartInfo& startInfo,
	IDebugEventsHandler& debugEventsHandler)
{
	Process process(startInfo);
	process.Start((coverChildren_) ? DEBUG_PROCESS: DEBUG_ONLY_THIS_PROCESS);
	
	DEBUG_EVENT debugEvent;
	MayBe<int> exitCode;

	processHandles_.clear();
	threadHandles_.clear();
	rootProcessId_ = MayBe<DWORD>();

	while (!exitCode || !processHandles_.empty())
	{
		if (!WaitForDebugEvent(&debugEvent, INFINITE))
			throw (L"Error WaitForDebugEvent:" + GetLastError());

		ProcessStatus processStatus = HandleDebugEvent(debugEvent, debugEventsHandler);
		
		// Get the exit code of the root process
		// Set once as we do not want EXCEPTION_BREAKPOINT to be override
		if (processStatus.exitCode_ && ((DWORD)rootProcessId_ == debugEvent.dwProcessId) && !exitCode)
			exitCode = processStatus.exitCode_;

		auto continueStatus = (processStatus.continueStatus_ ? processStatus.continueStatus_ : DBG_CONTINUE);

		if (!ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, continueStatus))
			throw("Error in ContinueDebugEvent:" + GetLastError());
	}

	return exitCode;
}

//-------------------------------------------------------------------------
Debugger::ProcessStatus Debugger::HandleDebugEvent(
	const DEBUG_EVENT& debugEvent,
	IDebugEventsHandler& debugEventsHandler)
{
	auto dwProcessId = debugEvent.dwProcessId;
	auto dwThreadId = debugEvent.dwThreadId;

	switch (debugEvent.dwDebugEventCode)
	{
		case CREATE_PROCESS_DEBUG_EVENT: OnCreateProcess(debugEvent, debugEventsHandler); break;
		case CREATE_THREAD_DEBUG_EVENT: OnCreateThread(debugEvent.u.CreateThread.hThread, dwThreadId); break;
		default:
		{
			auto hProcess = GetProcessHandle(dwProcessId);
			auto hThread = GetThreadHandle(dwThreadId);
			return HandleNotCreationalEvent(debugEvent, debugEventsHandler, hProcess, hThread, dwThreadId);
		}
	}

	return{};
}

//-------------------------------------------------------------------------
Debugger::ProcessStatus
	Debugger::HandleNotCreationalEvent(
	const DEBUG_EVENT& debugEvent,
	IDebugEventsHandler& debugEventsHandler,
	HANDLE hProcess,
	HANDLE hThread,
	DWORD dwThreadId)
{
	switch (debugEvent.dwDebugEventCode)
	{
		case EXIT_PROCESS_DEBUG_EVENT:
		{
			auto exitCode = OnExitProcess(debugEvent, hProcess, hThread, debugEventsHandler);
			return ProcessStatus{exitCode, MayBe<DWORD>()};
		}
		case EXIT_THREAD_DEBUG_EVENT: OnExitThread(dwThreadId); break;
		case LOAD_DLL_DEBUG_EVENT:
		{
			const auto& loadDll = debugEvent.u.LoadDll;
//TODO			Tools::ScopedAction scopedAction{ [&loadDll]{ CloseHandle(loadDll.hFile); } };
			debugEventsHandler.OnLoadDll(hProcess, hThread, loadDll);
			break;
		}
		case UNLOAD_DLL_DEBUG_EVENT:
		{
			debugEventsHandler.OnUnloadDll(hProcess, hThread, debugEvent.u.UnloadDll);
			break;
		}
		case EXCEPTION_DEBUG_EVENT: return OnException(debugEvent, debugEventsHandler, hProcess, hThread);
		case RIP_EVENT: OnRip(debugEvent.u.RipInfo); break;
		default: std::cerr << "Debug event:" << debugEvent.dwDebugEventCode; break;
	}

	return ProcessStatus{};
}

//-------------------------------------------------------------------------
Debugger::ProcessStatus
	Debugger::OnException(
	const DEBUG_EVENT& debugEvent,
	IDebugEventsHandler& debugEventsHandler,
	HANDLE hProcess,
	HANDLE hThread) const
{
	const auto& exception = debugEvent.u.Exception;
	auto exceptionType = debugEventsHandler.OnException(hProcess, hThread, exception);

	switch (exceptionType)
	{
		case IDebugEventsHandler::ExceptionType::BreakPoint:
		{
			return ProcessStatus{ MayBe<int>(), DBG_CONTINUE };
		}
		case IDebugEventsHandler::ExceptionType::InvalidBreakPoint:
		{
			std::cerr << "\n";
			std::cerr << "It seems there is an assertion failure or you call DebugBreak() in your program.";
			std::cerr << "\n";

			return ProcessStatus( EXCEPTION_BREAKPOINT, DBG_CONTINUE );
		}
		case IDebugEventsHandler::ExceptionType::NotHandled:
		{
			return ProcessStatus{ MayBe<int>(), DBG_EXCEPTION_NOT_HANDLED };
		}
		case IDebugEventsHandler::ExceptionType::Error:
		{
			return ProcessStatus{ MayBe<int>(), DBG_EXCEPTION_NOT_HANDLED };
		}
		case IDebugEventsHandler::ExceptionType::CppError:
		{
			if (continueAfterCppException_)
			{
				const auto& exceptionRecord = exception.ExceptionRecord;
				std::cerr << "Continue after a C++ exception.";
				return ProcessStatus{ static_cast<int>(exceptionRecord.ExceptionCode), DBG_CONTINUE };
			}
			return ProcessStatus{ MayBe<int>(), DBG_EXCEPTION_NOT_HANDLED };
		}
	}
	throw(L"Invalid exception Type.");
}

//-------------------------------------------------------------------------
void Debugger::OnCreateProcess(
	const DEBUG_EVENT& debugEvent,
	IDebugEventsHandler& debugEventsHandler)
{		
	const auto& processInfo = debugEvent.u.CreateProcessInfo;
//TODO	Tools::ScopedAction scopedAction{ [&processInfo]{ CloseHandle(processInfo.hFile); } };

	std::cerr << "Create Process: processId=" << debugEvent.dwProcessId << ", handle=" << processInfo.hProcess << std::endl;

	if (!rootProcessId_ && processHandles_.empty())
		rootProcessId_ = debugEvent.dwProcessId;

	if (!processHandles_.emplace(debugEvent.dwProcessId, processInfo.hProcess).second)
		throw(L"Process id already exist");
			
	debugEventsHandler.OnCreateProcess(processInfo);

	OnCreateThread(processInfo.hThread, debugEvent.dwThreadId);
}

//-------------------------------------------------------------------------
int Debugger::OnExitProcess(
	const DEBUG_EVENT& debugEvent,
	HANDLE hProcess,
	HANDLE hThread,
	IDebugEventsHandler& debugEventsHandler)
{
	OnExitThread(debugEvent.dwThreadId);
	auto processId = debugEvent.dwProcessId;

	std::cerr << "Exit Process:" << processId;

	auto exitProcess = debugEvent.u.ExitProcess;
	debugEventsHandler.OnExitProcess(hProcess, hThread, exitProcess);

	if (processHandles_.erase(processId) != 1)
		throw("Cannot find exited process.");

	return exitProcess.dwExitCode;
}

//-------------------------------------------------------------------------
void Debugger::OnCreateThread(
	HANDLE hThread,
	DWORD dwThreadId)
{
	std::cerr << "Create Thread: threadId=" << dwThreadId << ", handle=" << hThread << std::endl;

	if (!threadHandles_.emplace(dwThreadId, hThread).second)
		throw("Thread id already exist");
}

//-------------------------------------------------------------------------
void Debugger::OnExitThread(DWORD dwThreadId)
{	
	std::cerr << "Exit thread:" << dwThreadId;

	if (threadHandles_.erase(dwThreadId) != 1)
		throw("Cannot find exited thread.");
}

//-------------------------------------------------------------------------
HANDLE Debugger::GetProcessHandle(DWORD dwProcessId) const
{
	return processHandles_.at(dwProcessId);
}

//-------------------------------------------------------------------------
HANDLE Debugger::GetThreadHandle(DWORD dwThreadId) const
{
	return threadHandles_.at(dwThreadId);
}

//-------------------------------------------------------------------------
size_t Debugger::GetRunningProcesses() const
{
	return processHandles_.size();
}

//-------------------------------------------------------------------------
size_t Debugger::GetRunningThreads() const
{
	return threadHandles_.size();
}
