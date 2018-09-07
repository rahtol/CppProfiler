#include "Process.hpp"
#include <Windows.h>
#include "StartInfo.hpp"
#include <iostream>
#include <sstream>
#include <fstream>

namespace
{
	//---------------------------------------------------------------------
	MayBe<std::vector<wchar_t>> 
		CreateCommandLine(const std::vector<std::wstring>& arguments)
	{
		MayBe<std::vector<wchar_t>> commandLine;

		if (!arguments.empty())
		{				
			std::vector<wchar_t> buffer;
			for (const auto& argument : arguments)
			{
				buffer.push_back(L'\"');
				buffer.insert(buffer.end(), argument.begin(), argument.end());
				buffer.push_back(L'\"');
				buffer.push_back(L' ');
			}
				
			buffer.push_back(L'\0');
			return buffer;
		}

		return commandLine;
	}		
}

const std::wstring Process::CannotFindPathMessage = L"Cannot find path: ";
const std::wstring Process::CheckIfValidExecutableMessage =
    L"Cannot run process, check if it is a valid executable:";

//-------------------------------------------------------------------------
Process::Process(const StartInfo& startInfo)
	: startInfo_(startInfo)
{		
}

//-------------------------------------------------------------------------
Process::~Process()
{
	if (processInformation_)
	{			
		auto hProcess = processInformation_->hProcess;
		if (hProcess && !CloseHandle(hProcess))
			std::cerr << "Cannot close process handle";

		auto hThread = processInformation_->hThread;
		if (hThread && !CloseHandle(hThread))
			std::cerr << "Cannot close thread handle";
	}
}

//-------------------------------------------------------------------------
void Process::Start(DWORD creationFlags)
{
	if (processInformation_)
		throw (L"Process already started");

	STARTUPINFO lpStartupInfo;

	ZeroMemory(&lpStartupInfo, sizeof(lpStartupInfo));
	auto workindDirectory = startInfo_.GetWorkingDirectory();
	auto optionalCommandLine = CreateCommandLine(startInfo_.GetArguments());
	auto commandLine = (optionalCommandLine) ? &(optionalCommandLine.get())[0] : nullptr;

	processInformation_ = PROCESS_INFORMATION{};
	if (!CreateProcess(
		nullptr,
		commandLine,
		nullptr,
		nullptr,
		FALSE,
		creationFlags,
		nullptr,
		(workindDirectory) ? workindDirectory.get().c_str() : nullptr,
		&lpStartupInfo,
		&processInformation_.get()
		))
	{
		std::wostringstream ostr;
		std::ifstream f (startInfo_.GetPath().c_str());

		if (!f.good())
			ostr << CannotFindPathMessage + startInfo_.GetPath();
		else
		{
			ostr
			    << CheckIfValidExecutableMessage
			    << std::endl;

#ifndef _WIN64
			ostr << L"\n*** This version support only 32 bits executable "
			        L"***.\n\n";
#endif
			ostr << startInfo_ << GetLastError();
		}
		throw (ostr.str().c_str());
	}		
}
