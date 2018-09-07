#pragma once

#include <Windows.h>
#include <iosfwd>
#include <unordered_map>
#include <map>

enum class ExceptionHandlerStatus
{		
	BreakPoint,
	FirstChanceException,
	Error,
	CppError
};

class ExceptionHandler
{
public:
	static const std::wstring UnhandledExceptionErrorMessage;
	static const std::wstring ExceptionCpp;
	static const std::wstring ExceptionAccesViolation;
	static const std::wstring ExceptionUnknown;
	static const int ExceptionEmulationX86ErrorCode;
	static const int CppExceptionErrorCode;

	ExceptionHandler();

	ExceptionHandlerStatus HandleException(HANDLE hProcess, const EXCEPTION_DEBUG_INFO&, std::wostream&);
	void OnExitProcess(HANDLE hProcess);

private:
	ExceptionHandler(const ExceptionHandler&) = delete;
	ExceptionHandler& operator=(const ExceptionHandler&) = delete;

	void InitExceptionCode();
	std::wstring GetExceptionStrFromCode(DWORD) const;

	std::unordered_map<DWORD, std::wstring> exceptionCode_;
	std::map<DWORD, std::vector<HANDLE>> breakPointExceptionCode_;
};
