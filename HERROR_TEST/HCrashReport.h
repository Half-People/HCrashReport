#pragma once
#ifndef HCRASH_REPORT_H
#define HCRASH_REPORT_H

#define CrashReport_File_Path "CrashReport\\Crash\\CrashMessage.HCrash"
#define CrashDump_File_Path L"CrashReport\\Crash\\CrashDump.dmp"
#define CrashReport_Viewer_Directory  "CrashReport\\"
#define CrashReport_Viewer_Name "CrashReport.exe"
#define DumpJsonFile true	

namespace HCrashReport
{
	typedef void(*CrashCallback)(void*);
	void RegisterCrashReporter(CrashCallback callback = nullptr,void* user_data = nullptr);
}


#endif // !HCRASH_REPORT_H