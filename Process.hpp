#pragma once

#include <Windows.h>
#include "StartInfo.hpp"

class Process {
	public:
		static const std::wstring CannotFindPathMessage;
		static const std::wstring CheckIfValidExecutableMessage;

		Process(const StartInfo& startInfo);
		~Process();
		
		void Start(DWORD creationFlags);

	private:
		Process(const Process&) = delete;
		Process& operator=(const Process&) = delete;

	private:
		MayBe<PROCESS_INFORMATION> processInformation_;
		const StartInfo startInfo_;
};

