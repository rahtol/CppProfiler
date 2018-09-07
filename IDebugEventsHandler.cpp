#include "IDebugEventsHandler.hpp"

//-------------------------------------------------------------------------
void IDebugEventsHandler::OnCreateProcess(const CREATE_PROCESS_DEBUG_INFO&) 
{
}

//-------------------------------------------------------------------------
void IDebugEventsHandler::OnExitProcess(HANDLE hProcess, HANDLE hThread, const EXIT_PROCESS_DEBUG_INFO&)
{
}

//-------------------------------------------------------------------------
void IDebugEventsHandler::OnLoadDll(HANDLE hProcess, HANDLE hThread, const LOAD_DLL_DEBUG_INFO&)
{
}

//-------------------------------------------------------------------------
void IDebugEventsHandler::OnUnloadDll(HANDLE hProcess, HANDLE hThread, const UNLOAD_DLL_DEBUG_INFO&)
{
}

//-------------------------------------------------------------------------
IDebugEventsHandler::ExceptionType IDebugEventsHandler::OnException(HANDLE hProcess, HANDLE hThread, const EXCEPTION_DEBUG_INFO&)
{ 
	return IDebugEventsHandler::ExceptionType::NotHandled;
}
