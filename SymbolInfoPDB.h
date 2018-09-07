#pragma once
#include "MayBe.h"
#include <string>
#include <map>
#include "dia2.h"
#include "Callback.h"

class SymbolInfoPDB
{
private:
	bool initializationOk;
	std::wstring g_filename;
	IDiaDataSource *g_pDiaDataSource;
	IDiaSession *g_pDiaSession;
	IDiaSymbol *g_pGlobalSymbol;
	DWORD g_dwMachineType = CV_CFL_80386;

public:
	std::map<DWORD, std::wstring> tracepoints;

public:
	SymbolInfoPDB(const std::wstring& symbolfile);
	~SymbolInfoPDB();
	MayBe<DWORD> getRVAByNames(std::wstring classname, std::wstring methodname);
	void addTracepoint(std::wstring qualifier);
};

