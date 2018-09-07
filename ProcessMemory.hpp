#pragma once

#include <Windows.h>
#include <memory>
#include <vector>

void WriteProcessMemory(HANDLE hProcess, DWORD address, void* buffer, size_t size);
std::vector<unsigned char> ReadProcessMemory(HANDLE hProcess, DWORD address, size_t size);
void ReadProcessMemory(HANDLE hProcess, DWORD address, void* buffer, SIZE_T size);

//-------------------------------------------------------------------------
template <typename T>
std::unique_ptr<T> ReadStructInProcessMemory(HANDLE hProcess, DWORD address)
{
	auto data = std::make_unique<T>();
	ReadProcessMemory(hProcess, address, data.get(), sizeof(T));

	return data;
}
