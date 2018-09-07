#include "StartInfo.hpp"
#include "CppProfilerRunner.hpp"
#include "SymbolInfoPdb.h"
#include <string>

void bal014a()
{
	std::wstring workingDirectory = L"I:\\work\\OBCU_ATP\\OBCU_ATP_2.3.1-LAB_2.3.2R\\TGMT_OBCU_V\\tests\\TestCases\\bal\\bal014a";
	StartInfo startInfo(L"I:\\work\\OBCU_ATP\\OBCU_ATP_2.3.1-LAB_2.3.2R\\TGMT_OBCU_V\\tests\\TestCases\\bal\\bal014a\\bal014a.exe");
	startInfo.SetWorkingDirectory(L"I:\\work\\OBCU_ATP\\OBCU_ATP_2.3.1-LAB_2.3.2R\\TGMT_OBCU_V\\tests\\TestCases\\bal\\bal014a");
	startInfo.AddArgument(L"--all");

	SymbolInfoPDB symtab(startInfo.GetPath());
	MayBe<unsigned long> loc_locating_c_Start = symtab.getRVAByNames(L"LOC_Locating_C", L"Start");
	MayBe<unsigned long> loc_locating_c_setForwardDirection = symtab.getRVAByNames(L"LOC_Locating_C", L"SetForwardDirection");
	MayBe<unsigned long> loc_movementauthority_c_Start = symtab.getRVAByNames(L"MA_MovementAuthority_C", L"Start");

	symtab.addTracepoint(L"LOC_Locating_C::Start");

	CppProfilerRunner r(symtab, workingDirectory + L"\\CppProfiler.lst");
	r.Run(startInfo);
}

void ieb223a()
{
	std::wstring workingDirectory = L"D:\\work\\OBCU_ATP\\OBCU_ATP_2.3.1-LAB_2.3.2R\\TGMT_OBCU_V\\tests\\TestCases\\ieb\\ieb223a";
	StartInfo startInfo(workingDirectory + L"\\ieb223a.exe");
	startInfo.SetWorkingDirectory(workingDirectory);
	startInfo.AddArgument(L"--all");

	SymbolInfoPDB symtab(startInfo.GetPath());
	MayBe<unsigned long> loc_locating_c_Start = symtab.getRVAByNames(L"LOC_Locating_C", L"Start");
	MayBe<unsigned long> loc_locating_c_setForwardDirection = symtab.getRVAByNames(L"LOC_Locating_C", L"SetForwardDirection");
	MayBe<unsigned long> loc_movementauthority_c_Start = symtab.getRVAByNames(L"MA_MovementAuthority_C", L"Start");

	symtab.addTracepoint(L"LOC_Locating_C::Start");

	CppProfilerRunner r(symtab, workingDirectory + L"\\CppProfiler.lst");
	r.Run(startInfo);
}

void tt901()
{
	std::wstring workingDirectory = L"B:\\TGMT_WCU_SW\\TRA\\SRC";
	StartInfo startInfo(workingDirectory + L"\\tt901.exe");
	startInfo.SetWorkingDirectory(workingDirectory);
	startInfo.AddArgument(L"--all");

	SymbolInfoPDB symtab(startInfo.GetPath());
	symtab.addTracepoint(L"CLte::update_atp_vacancy");
	symtab.addTracepoint(L"CTvs::update_all_subordinate_lte");

	CppProfilerRunner r(symtab, workingDirectory + L"\\CppProfiler.lst");
	r.Run(startInfo);
}

void main()
{
	tt901();
}

