#include "SymbolInfoPDB.h"

//-------------------------------------------------------------------------

// Create an IDiaData source and open a PDB file
//
bool LoadDataFromPdb(
	const wchar_t    *szFilename,
	IDiaDataSource  **ppSource,
	IDiaSession     **ppSession,
	IDiaSymbol      **ppGlobal)
{
	wchar_t wszExt[MAX_PATH];
	wchar_t *wszSearchPath = L"SRV**\\\\symbols\\symbols"; // Alternate path to search for debug data
	DWORD dwMachType = 0;

	HRESULT hr = CoInitialize(NULL);

	// Obtain access to the provider

	hr = CoCreateInstance(__uuidof(DiaSource),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(IDiaDataSource),
		(void **)ppSource);

	if (FAILED(hr)) {
		wprintf(L"CoCreateInstance failed - HRESULT = %08X\n", hr);

		return false;
	}

	_wsplitpath_s(szFilename, NULL, 0, NULL, 0, NULL, 0, wszExt, MAX_PATH);

	if (!_wcsicmp(wszExt, L".pdb")) {
		// Open and prepare a program database (.pdb) file as a debug data source

		hr = (*ppSource)->loadDataFromPdb(szFilename);

		if (FAILED(hr)) {
			wprintf(L"loadDataFromPdb failed - HRESULT = %08X\n", hr);

			return false;
		}
	}

	else {
		CCallback callback; // Receives callbacks from the DIA symbol locating procedure,
							// thus enabling a user interface to report on the progress of
							// the location attempt. The client application may optionally
							// provide a reference to its own implementation of this
							// virtual base class to the IDiaDataSource::loadDataForExe method.
		callback.AddRef();

		// Open and prepare the debug data associated with the executable

		hr = (*ppSource)->loadDataForExe(szFilename, wszSearchPath, &callback);

		if (FAILED(hr)) {
			wprintf(L"loadDataForExe failed - HRESULT = %08X\n", hr);

			return false;
		}
	}

	// Open a session for querying symbols

	hr = (*ppSource)->openSession(ppSession);

	if (FAILED(hr)) {
		wprintf(L"openSession failed - HRESULT = %08X\n", hr);

		return false;
	}

	// Retrieve a reference to the global scope

	hr = (*ppSession)->get_globalScope(ppGlobal);

	if (hr != S_OK) {
		wprintf(L"get_globalScope failed\n");

		return false;
	}

/* i guess i don't need this here

	// Set Machine type for getting correct register names
	if ((*ppGlobal)->get_machineType(&dwMachType) == S_OK) {
		switch (dwMachType) {
		case IMAGE_FILE_MACHINE_I386: g_dwMachineType = CV_CFL_80386; break;
		case IMAGE_FILE_MACHINE_IA64: g_dwMachineType = CV_CFL_IA64; break;
		case IMAGE_FILE_MACHINE_AMD64: g_dwMachineType = CV_CFL_AMD64; break;
		}
	}
*/

	return true;
}

//-------------------------------------------------------------------------

// Release DIA objects and CoUninitialize
//
void Cleanup(IDiaDataSource *ppSource,	IDiaSession *pDiaSession, IDiaSymbol *pGlobalSymbol)
{
	if (pGlobalSymbol) {
		pGlobalSymbol->Release();
		pGlobalSymbol = NULL;
	}

	if (pDiaSession) {
		pDiaSession->Release();
		pDiaSession = NULL;
	}

	CoUninitialize();
}

//-------------------------------------------------------------------------
SymbolInfoPDB::SymbolInfoPDB(const std::wstring& symbolfile) : tracepoints()
{
	this->g_filename = symbolfile;
	initializationOk = LoadDataFromPdb(symbolfile.c_str(), &this->g_pDiaDataSource, &this->g_pDiaSession, &this->g_pGlobalSymbol);
}


//-------------------------------------------------------------------------
SymbolInfoPDB::~SymbolInfoPDB()
{
	Cleanup(this->g_pDiaDataSource, this->g_pDiaSession, this->g_pGlobalSymbol);
}


MayBe<DWORD> SymbolInfoPDB::getRVAByNames(std::wstring classname, std::wstring methodname)
{
	IDiaEnumSymbols *pEnumSymbols = NULL;
	IDiaSymbol *pSymbolClass = NULL;
	IDiaEnumSymbols *pEnumSymbols2 = NULL;
	IDiaSymbol *pSymbolMethod = NULL;
	LONG noClassesFound;
	DWORD dwSymTagClass;
	DWORD dwLocationType;
	DWORD dwRVA;

	try
	{
		if (!this->initializationOk)
			throw 1;

		if (FAILED(this->g_pGlobalSymbol->findChildren(SymTagNull, classname.c_str(), nsRegularExpression, &pEnumSymbols))) {
			throw 2;
		}

		if (FAILED(pEnumSymbols->get_Count(&noClassesFound))) {
			throw 11;
		}

		if (noClassesFound != 1) {
			throw 12;
		}

		if (FAILED(pEnumSymbols->Item(0, &pSymbolClass))) {
			return 13;
		}

		if (FAILED(pSymbolClass->get_symTag(&dwSymTagClass))) {
			return 21;
		}
		if (dwSymTagClass != SymTagUDT) {
			return 22;
		}

		if (FAILED(pSymbolClass->findChildren(SymTagNull, methodname.c_str(), nsRegularExpression, &pEnumSymbols2))) {
			return 23;
		}

		if (FAILED(pEnumSymbols2->get_Count(&noClassesFound))) {
			throw 31;
		}

		if (noClassesFound != 1) {
			throw 32;
		}

		if (FAILED(pEnumSymbols2->Item(0, &pSymbolMethod))) {
			return 33;
		}

		if (FAILED(pSymbolMethod->get_symTag(&dwSymTagClass))) {
			return 41;
		}

		if (dwSymTagClass != SymTagFunction) {
			return 42;
		}

		if (FAILED(pSymbolMethod->get_locationType(&dwLocationType))) {
			return 43;
		}

		if (dwLocationType != LocIsStatic) {
			throw 44;
		}

		if (FAILED(pSymbolMethod->get_relativeVirtualAddress(&dwRVA))) {
			return 45;
		}

		pEnumSymbols->Release();
		pSymbolClass->Release();
		pEnumSymbols2->Release();
		pSymbolMethod->Release();

		return MayBe<DWORD>(dwRVA);  // indicate success
	}
	catch (int failureCode)
	{
		if ((failureCode >= 10) && (pEnumSymbols != NULL)) pEnumSymbols->Release();
		if ((failureCode >= 20) && (pSymbolClass != NULL)) pSymbolClass->Release();
		if ((failureCode >= 30) && (pEnumSymbols2 != NULL)) pEnumSymbols2->Release();
		if ((failureCode >= 40) && (pSymbolMethod != NULL)) pSymbolMethod->Release();

		return MayBe<DWORD>();  // indicate failure
	}
}

void SymbolInfoPDB::addTracepoint(std::wstring qualifier)
{
	size_t pos = qualifier.find(L"::");
	if (pos == std::wstring::npos) throw (L"Illegal symbol format.");
	std::wstring classname = qualifier.substr(0, pos);
	std::wstring methodname = qualifier.substr(pos+2);
	MayBe<DWORD> address = this->getRVAByNames(classname, methodname);
	if (!address) throw (L"Address not found.");
	this->tracepoints.emplace(address.get(), qualifier);
}
