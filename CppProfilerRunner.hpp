#pragma once

#include <memory>
#include <vector>
#include <fstream>
#include "IDebugEventsHandler.hpp"
#include "StartInfo.hpp"
#include "ExceptionHandler.hpp"
#include "SymbolInfoPdb.h"

class CppProfilerRunner : private IDebugEventsHandler
{
public:
	CppProfilerRunner(SymbolInfoPDB& symtab, std::wstring);
	~CppProfilerRunner();

	int Run(const StartInfo&);

private:
	virtual void OnCreateProcess(const CREATE_PROCESS_DEBUG_INFO&) override;
	virtual void OnExitProcess(HANDLE hProcess, HANDLE hThread, const EXIT_PROCESS_DEBUG_INFO&) override;
	virtual void OnLoadDll(HANDLE hProcess, HANDLE hThread, const LOAD_DLL_DEBUG_INFO&) override;
	virtual void OnUnloadDll(HANDLE hProcess, HANDLE hThread, const UNLOAD_DLL_DEBUG_INFO&) override;
	virtual ExceptionType OnException(HANDLE hProcess, HANDLE hThread, const EXCEPTION_DEBUG_INFO&) override;

private:
	CppProfilerRunner(const CppProfilerRunner&) = delete;
	CppProfilerRunner& operator=(const CppProfilerRunner&) = delete;

	void LoadModule(HANDLE hProcess, HANDLE hFile, void* baseOfImage);
	bool OnBreakPoint(const EXCEPTION_DEBUG_INFO&, HANDLE hProcess, HANDLE hThread);

	void WriteBreakPoint(HANDLE hProcess, DWORD address);
	void RemoveBreakPoint(HANDLE hProcess, DWORD address);
	void installTracepoints();

	boolean findOnReturnStack(DWORD address);

private:
	SymbolInfoPDB& symtab;
	std::unique_ptr<ExceptionHandler> exceptionHandler_;
	std::map<DWORD, UCHAR> originalCode;
	std::vector<DWORD> returnStack;
	DWORD baseOfImage;
	HANDLE hProcess;
	int singleStep;
	DWORD singleStepAddress;
	std::ofstream outf;
};
