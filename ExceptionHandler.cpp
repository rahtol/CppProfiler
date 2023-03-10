#include "ExceptionHandler.hpp"
//#include "ProgramOptions.hpp"

//#include "Tools/ScopedAction.hpp"
//#include "Tools/Tool.hpp"
#include <ostream>
#include <sstream>

//-------------------------------------------------------------------------
const std::wstring separatorLine = L"----------------------------------------------------";
const std::wstring ExceptionHandler::UnhandledExceptionErrorMessage = L"Unhandled exception code: ";
const std::wstring ExceptionHandler::ExceptionCpp = L"Exception C++";
const std::wstring ExceptionHandler::ExceptionAccesViolation = L"EXCEPTION_ACCESS_VIOLATION";
const std::wstring ExceptionHandler::ExceptionUnknown = L"Unknown";
const int ExceptionHandler::ExceptionEmulationX86ErrorCode = 0x4000001f;
const int ExceptionHandler::CppExceptionErrorCode = 0xE06D7363;

//-------------------------------------------------------------------------
ExceptionHandler::ExceptionHandler()
{
	breakPointExceptionCode_.emplace(EXCEPTION_BREAKPOINT, std::vector<HANDLE>{});
	breakPointExceptionCode_.emplace(EXCEPTION_SINGLE_STEP, std::vector<HANDLE>{});
	breakPointExceptionCode_.emplace(ExceptionEmulationX86ErrorCode, std::vector<HANDLE>{});
	InitExceptionCode();
}

//-------------------------------------------------------------------------
void ExceptionHandler::InitExceptionCode()
{
	exceptionCode_.emplace(EXCEPTION_ACCESS_VIOLATION, L"EXCEPTION_ACCESS_VIOLATION");
	exceptionCode_.emplace(EXCEPTION_ARRAY_BOUNDS_EXCEEDED, L"EXCEPTION_ARRAY_BOUNDS_EXCEEDED");
	exceptionCode_.emplace(EXCEPTION_BREAKPOINT, L"EXCEPTION_BREAKPOINT");
	exceptionCode_.emplace(EXCEPTION_DATATYPE_MISALIGNMENT, L"EXCEPTION_DATATYPE_MISALIGNMENT");
	exceptionCode_.emplace(EXCEPTION_FLT_DENORMAL_OPERAND, L"EXCEPTION_FLT_DENORMAL_OPERAND");
	exceptionCode_.emplace(EXCEPTION_FLT_DIVIDE_BY_ZERO, L"EXCEPTION_FLT_DIVIDE_BY_ZERO");
	exceptionCode_.emplace(EXCEPTION_FLT_INEXACT_RESULT, L"EXCEPTION_FLT_INEXACT_RESULT");
	exceptionCode_.emplace(EXCEPTION_FLT_INVALID_OPERATION, L"EXCEPTION_FLT_INVALID_OPERATION");
	exceptionCode_.emplace(EXCEPTION_FLT_OVERFLOW, L"EXCEPTION_FLT_OVERFLOW");
	exceptionCode_.emplace(EXCEPTION_FLT_STACK_CHECK, L"EXCEPTION_FLT_STACK_CHECK");
	exceptionCode_.emplace(EXCEPTION_FLT_UNDERFLOW, L"EXCEPTION_FLT_UNDERFLOW");
	exceptionCode_.emplace(EXCEPTION_ILLEGAL_INSTRUCTION, L"EXCEPTION_ILLEGAL_INSTRUCTION");
	exceptionCode_.emplace(EXCEPTION_IN_PAGE_ERROR, L"EXCEPTION_IN_PAGE_ERROR");
	exceptionCode_.emplace(EXCEPTION_INT_DIVIDE_BY_ZERO, L"EXCEPTION_INT_DIVIDE_BY_ZERO");
	exceptionCode_.emplace(EXCEPTION_INT_OVERFLOW, L"EXCEPTION_INT_OVERFLOW");
	exceptionCode_.emplace(EXCEPTION_INVALID_DISPOSITION, L"EXCEPTION_INVALID_DISPOSITION");
	exceptionCode_.emplace(EXCEPTION_NONCONTINUABLE_EXCEPTION, L"EXCEPTION_NONCONTINUABLE_EXCEPTION");
	exceptionCode_.emplace(EXCEPTION_PRIV_INSTRUCTION, L"EXCEPTION_PRIV_INSTRUCTION");
	exceptionCode_.emplace(EXCEPTION_SINGLE_STEP, L"EXCEPTION_SINGLE_STEP");
	exceptionCode_.emplace(EXCEPTION_STACK_OVERFLOW, L"EXCEPTION_STACK_OVERFLOW");
	
	exceptionCode_.emplace(CppExceptionErrorCode, ExceptionCpp);
}

//-------------------------------------------------------------------------
ExceptionHandlerStatus ExceptionHandler::HandleException(
	HANDLE hProcess,
	const EXCEPTION_DEBUG_INFO& exceptionDebugInfo,
	std::wostream& message)
{
	const auto& exceptionRecord = exceptionDebugInfo.ExceptionRecord;
	const auto exceptionCode = exceptionRecord.ExceptionCode;

	if (exceptionDebugInfo.dwFirstChance)
	{
		auto it = breakPointExceptionCode_.find(exceptionCode);
		
		if (it != breakPointExceptionCode_.end())
		{
			auto& processHandles = breakPointExceptionCode_.find(EXCEPTION_BREAKPOINT)->second;
			// Breakpoint exception need to be ignore the first time by process.
			if (std::find(processHandles.begin(), processHandles.end(), hProcess) == processHandles.end())
				processHandles.push_back(hProcess);
			else
				return ExceptionHandlerStatus::BreakPoint;
		}

		return ExceptionHandlerStatus::FirstChanceException;
	}
			
	message << std::endl << std::endl;
	message << separatorLine << std::endl;
	message << L"*** ";
	message << UnhandledExceptionErrorMessage << exceptionRecord.ExceptionCode;
	message << L": " << GetExceptionStrFromCode(exceptionRecord.ExceptionCode) << std::endl;
	message << separatorLine << std::endl;

	return (exceptionCode == CppExceptionErrorCode) 
		? ExceptionHandlerStatus::CppError : ExceptionHandlerStatus::Error;
}

//-------------------------------------------------------------------------
void ExceptionHandler::OnExitProcess(HANDLE hProcess)
{
	for (auto& pair : breakPointExceptionCode_)
	{
		std::vector<HANDLE>& processes = pair.second;
		auto it = std::find(processes.begin(), processes.end(), hProcess);

		if (it != processes.end())
			processes.erase(it);
	}
}

//-------------------------------------------------------------------------
std::wstring ExceptionHandler::GetExceptionStrFromCode(DWORD exceptionCode) const
{
	auto it = exceptionCode_.find(exceptionCode);

	if (it != exceptionCode_.end())
		return it->second;
	
	LPTSTR message = nullptr;
	HMODULE ntDllModule = LoadLibrary(L"NTDLL.DLL");
// TODO	Tools::ScopedAction closeLibrary{ [=](){ FreeLibrary(ntDllModule); } };

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_FROM_HMODULE,
		ntDllModule,
		exceptionCode,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		reinterpret_cast<LPTSTR>(&message),
		0,
		nullptr);		
// TODO	Tools::ScopedAction freeMessage{ [=](){ LocalFree(message); } };

	if (message)
		return message;

	return ExceptionUnknown;
}
