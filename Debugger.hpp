#pragma once

#include "MayBe.h"
#include <unordered_map>
#include <Windows.h>

class StartInfo;
class IDebugEventsHandler;

class Debugger	{
	public:
		Debugger(
			bool coverChildren,
			bool continueAfterCppException);

		int Debug(const StartInfo&, IDebugEventsHandler&);
		size_t GetRunningProcesses() const;
		size_t GetRunningThreads() const;

	private:
		Debugger(const Debugger&) = delete;
		Debugger& operator=(const Debugger&) = delete;
	
		void OnCreateProcess(
			const DEBUG_EVENT& debugEvent,
			IDebugEventsHandler& debugEventsHandler);

		int OnExitProcess(
			const DEBUG_EVENT& debugEvent,
			HANDLE hProcess,
			HANDLE hThread,
			IDebugEventsHandler& debugEventsHandler);

		void OnCreateThread(
			HANDLE hThread,
			DWORD dwThreadId);

		void OnExitThread(DWORD dwProcessId);

		HANDLE GetProcessHandle(DWORD dwProcessId) const;
		HANDLE GetThreadHandle(DWORD dwThreadId) const;

		struct ProcessStatus;

		ProcessStatus HandleDebugEvent(const DEBUG_EVENT&, IDebugEventsHandler&);

		ProcessStatus HandleNotCreationalEvent(
			const DEBUG_EVENT& debugEvent,
			IDebugEventsHandler& debugEventsHandler,
			HANDLE hProcess,
			HANDLE hThread,
			DWORD dwThreadId);

		ProcessStatus OnException(const DEBUG_EVENT&, IDebugEventsHandler&, HANDLE hProcess, HANDLE hThread) const;

	private:
		std::unordered_map<DWORD, HANDLE> processHandles_;
		std::unordered_map<DWORD, HANDLE> threadHandles_;
		MayBe<DWORD> rootProcessId_;
		bool coverChildren_;
		bool continueAfterCppException_;
};
