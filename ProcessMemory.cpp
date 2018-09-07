#include <iostream>
#include "ProcessMemory.hpp"
//#include "ToolsException.hpp"
//#include "Log.hpp"

//-------------------------------------------------------------------------
std::vector<unsigned char>
ReadProcessMemory(HANDLE hProcess, DWORD address, size_t size)
{
	std::vector<unsigned char> data(size);
	ReadProcessMemory(hProcess,
	                  address,
	                  &data[0],
	                  data.size());
	return data;
}

//-------------------------------------------------------------------------
void ReadProcessMemory(HANDLE hProcess,
                       DWORD address,
                       void* buffer,
                       SIZE_T size)
{
	SIZE_T totalBytesRead = 0;
	SIZE_T bytesRead = 0;

	while (totalBytesRead < size)
	{
		if (!::ReadProcessMemory(
		        hProcess,
		        reinterpret_cast<void*>(address),
		        &reinterpret_cast<char*>(buffer)[totalBytesRead],
		        size - totalBytesRead,
		        &bytesRead))
		{
			std::cerr << "Cannot read memory";
		}
		if (bytesRead == 0)
			throw("Cannot ready process memory");

		totalBytesRead += bytesRead;
	}
}

//-------------------------------------------------------------------------
void WriteProcessMemory(HANDLE hProcess,
                        DWORD address,
                        void* buffer,
                        size_t size)
{
	SIZE_T totalWritten = 0;
	SIZE_T written = 0;

	while (totalWritten < size)
	{
		auto startBuffer = static_cast<char*>(buffer) + totalWritten;
		if (!::WriteProcessMemory(hProcess,
		                          (void*)address,
		                          startBuffer,
		                          size - totalWritten,
		                          &written))
		{
			std::cerr << "Cannot write memory:";
		}

		if (written == 0)
			throw("Cannot write process memory");

		if (!FlushInstructionCache(hProcess, startBuffer, written))
			throw("Cannot flush memory:");
		totalWritten += written;
	}
}

