#include "HCrashReport.h"
#define _NO_CVCONST_H
#define _CRT_SECURE_NO_WARNINGS
#include <csignal>
#include <Windows.h>
#include <DbgHelp.h>
#include <fstream>
#include <string>
#include <sstream>
#include "nlohmann/json.hpp"

#pragma comment(lib,"Dbghelp.lib")

std::string longLongToHexString(long long value) {
	std::stringstream ss;
	ss << std::hex << std::uppercase << value; // 转换为十六进制并使用大写字母
	return ss.str();
}


struct CrashReport_Context
{
	HCrashReport::CrashCallback callback = nullptr;
	void* callback_user_data = nullptr;

	void* stack[100];
	HANDLE process = GetCurrentProcess();
};
CrashReport_Context* context = nullptr;



nlohmann::json GetModInfo(ULONG64 ModBase)
{
	nlohmann::json j;
	char moduleName[MAX_PATH];
	if (!(GetModuleFileNameA((HMODULE)ModBase, moduleName, sizeof(moduleName)) > 0)) {
		sprintf_s(moduleName, "Could not retrieve module name: %lu\n", GetLastError());
	}	
	auto Image = ImageNtHeader((PVOID)ModBase);
	if (Image)
	{
		switch (Image->FileHeader.Machine)
		{
		case IMAGE_FILE_MACHINE_I386:
			j["Machine"] = "I386 (x86)";
			break;
		case IMAGE_FILE_MACHINE_IA64:
			j["Machine"] = "IA64 (Intel Itanium)";
			break;
		case IMAGE_FILE_MACHINE_AMD64:
			j["Machine"] = "AMD64 (x64)";
			break;
		default:
			break;
		}
	

		j["time"] = Image->FileHeader.TimeDateStamp;
		j["Characteristics"] = Image->FileHeader.Characteristics;
		j["Magic"] = Image->OptionalHeader.Magic;
		j["LinkerVersion"] = std::to_string(Image->OptionalHeader.MajorLinkerVersion).append(".").append(std::to_string(Image->OptionalHeader.MinorLinkerVersion));
		j["OperatingSystemVersion"] = std::to_string(Image->OptionalHeader.MajorOperatingSystemVersion).append(".").append(std::to_string(Image->OptionalHeader.MinorOperatingSystemVersion));
		j["ImageVersion"] = std::to_string(Image->OptionalHeader.MajorImageVersion).append(".").append(std::to_string(Image->OptionalHeader.MinorImageVersion));
		j["SubsystemVersion"] = std::to_string(Image->OptionalHeader.MajorSubsystemVersion).append(".").append(std::to_string(Image->OptionalHeader.MinorSubsystemVersion));
		j["Subsystem"] = Image->OptionalHeader.Subsystem;
		j["DllCharacteristics"] = Image->OptionalHeader.DllCharacteristics;
	}

	j["path"] = std::string(moduleName);

	return j;
}
nlohmann::json GetExceptionRecordInfo(PEXCEPTION_RECORD Record)
{
	nlohmann::json j;
	j["Address"] = longLongToHexString((long long)Record->ExceptionAddress);
	j["Code"] = Record->ExceptionCode;
	j["Flags"] = Record->ExceptionFlags;
	for (size_t i = 0; i < Record->NumberParameters; i++)
	{
		j["Information"].push_back(Record->ExceptionInformation[i]);
	}
	return j;
}
void StackJsonFileCreate(PEXCEPTION_RECORD ExceptionRecord)
{
	
	unsigned short frames = CaptureStackBackTrace(0, 100, context->stack, NULL);
	SYMBOL_INFO* symbol;
	symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);

	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	nlohmann::json j;
	j["ExceptionRecord"] = GetExceptionRecordInfo(ExceptionRecord);



	//std::vector<std::string> tag_tabal{
	//		"Null"						   ,
	//		"Exe"						   ,
	//		"Compiland"					   ,
	//		"CompilandDetails"			   ,
	//		"CompilandEnv"				   ,
	//		"Function"					   ,
	//		"Block"						   ,
	//		"Data"						   ,
	//		"Annotation"				   ,
	//		"Label"						   ,
	//		"PublicSymbol"				   ,
	//		"UDT"						   ,
	//		"Enum"						   ,
	//		"FunctionType"				   ,
	//		"PointerType"				   ,
	//		"ArrayType"					   ,
	//		"BaseType"					   ,
	//		"Typedef"					   ,
	//		"BaseClass"					   ,
	//		"Friend"					   ,
	//		"FunctionArgType"			   ,
	//		"FuncDebugStart"			   ,
	//		"FuncDebugEnd"				   ,
	//		"UsingNamespace"			   ,
	//		"VTableShape"				   ,
	//		"VTable"					   ,
	//		"Custom"					   ,
	//		"Thunk"						   ,
	//		"CustomType"				   ,
	//		"ManagedType"				   ,
	//		"Dimension"
	//};


	for (unsigned short i = 0; i < frames; i++) {
		SymFromAddr(context->process, (DWORD64)(context->stack[i]), 0, symbol);
		nlohmann::json info;
		info["name"] = std::string(symbol->Name);
		info["address"] = longLongToHexString(symbol->Address);
		info["flags"] = symbol->Flags;
		info["value"] = symbol->Value;
		info["tag"] = symbol->Tag;
		info["mod"] = GetModInfo(symbol->ModBase);
		
		j["stack"].push_back(info);
	}
	free(symbol);

	std::ofstream crash_file(CrashReport_File_Path);
	if(crash_file.good())
	{ 
#if DumpJsonFile
		crash_file << j.dump(4);
#else
		crash_file << j;
#endif
	}

	else
	{
		MessageBoxA(NULL,j.dump(4).c_str(), "Crash Report",MB_OK);
	}
	crash_file.close();
}
void OpenCrashReportViewer()
{
	// 定义进程信息
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	// 初始化结构体
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// 获取当前工作目录
	char currentPath[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, currentPath);


	// 合并当前路径和局部路径
	std::string fullPath = currentPath;
	fullPath.append("\\").append(CrashReport_Viewer_Directory);



	// 创建进程
	if (CreateProcessA(
		std::string(fullPath).append(CrashReport_Viewer_Name).c_str(),
		NULL,
		NULL,
		NULL,
		FALSE,
		DETACHED_PROCESS,
		NULL,
		fullPath.c_str(),
		&si,
		&pi)
		) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else {
		DWORD error = GetLastError();
		std::string errorMessage = "Failed to open CrashReportViewer\nError Code: " + std::to_string(error);
		MessageBoxA(NULL, errorMessage.c_str(), "Crash Report", MB_OK);
	}
}
LONG WINAPI UnhandledExceptionFilter_(EXCEPTION_POINTERS* pExceptionPointers) {
	HANDLE hDumpFile = CreateFile(CrashDump_File_Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hDumpFile != INVALID_HANDLE_VALUE) {
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ExceptionPointers = pExceptionPointers;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ClientPointers = TRUE;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithIndirectlyReferencedMemory, &dumpInfo, NULL, NULL);
		CloseHandle(hDumpFile);

		StackJsonFileCreate(pExceptionPointers->ExceptionRecord);


		OpenCrashReportViewer();

	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void HCrashReport::RegisterCrashReporter(CrashCallback callback, void* user_data)
{
	if (context == nullptr){context = new CrashReport_Context;	SymInitialize(context->process, NULL, TRUE);}
	context->callback = callback;
	context->callback_user_data = user_data;

	SetUnhandledExceptionFilter(UnhandledExceptionFilter_);

}
