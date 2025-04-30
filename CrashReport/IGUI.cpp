#define IMGUI_DEFINE_MATH_OPERATORS
#define _CRT_SECURE_NO_WARNINGS
#define CPPHTTPLIB_OPENSSL_SUPPORT
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "IGUI.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <SDL2/SDL.h>
#include <vector>
#include <httplib.h>
#include <Windows.h>
#include <DbgHelp.h>
#include <time.h>
#include <filesystem>
#include <sysinfoapi.h>
#include <tchar.h>
#include <locale>
#include <codecvt>



HImage TitleImage;
char user_message[2048] = {'\0'};
SDL_Window * windows = nullptr;
nlohmann::json main_info;

enum H_SymTagEnum {
	H_SymTagNull,
	H_SymTagExe,
	H_SymTagCompiland,
	H_SymTagCompilandDetails,
	H_SymTagCompilandEnv,
	H_SymTagFunction,
	H_SymTagBlock,
	H_SymTagData,
	H_SymTagAnnotation,
	H_SymTagLabel,
	H_SymTagPublicSymbol,
	H_SymTagUDT,
	H_SymTagEnum_RN,
	H_SymTagFunctionType,
	H_SymTagPointerType,
	H_SymTagArrayType,
	H_SymTagBaseType,
	H_SymTagTypedef,
	H_SymTagBaseClass,
	H_SymTagFriend,
	H_SymTagFunctionArgType,
	H_SymTagFuncDebugStart,
	H_SymTagFuncDebugEnd,
	H_SymTagUsingNamespace,
	H_SymTagVTableShape,
	H_SymTagVTable,
	H_SymTagCustom,
	H_SymTagThunk,
	H_SymTagCustomType,
	H_SymTagManagedType,
	H_SymTagDimension
};

struct Mod
{
	bool only_path = true;
	std::string path;
	std::vector<std::string> Characteristics;
	std::string DllCharacteristics;
	std::string Machine;
	std::string Magic;
	std::string LinkerVersion;
	std::string OperatingSystemVersion;
	std::string SubsystemVersion;
	std::string ImageVersion;
	std::string Subsystem;
	std::string time;
};
struct StackInfo
{
	std::string name;
	std::string address;
	std::vector<std::string> Flags;
	std::string tag;
	Mod mod;

	ULONG64 value =0;

	bool have_value = false;
	bool DebugTool_Stack = false;
};
struct sysinfo
{
	std::string ProcessorArchitecture = "";
	int NumberOfProcessors = 0;

	int MemoryTotalPhys = 0;
	int MemoryAvailPhys = 0;
	int MemoryLoad = 0;

	std::string OSVersion = "", OS_CSDVersion = "";
	long long OSBuildNumber = 0;
};
sysinfo H_SYSINFO;
std::vector<StackInfo> Stacks;

std::string longLongToHexString(long long value) {
	std::stringstream ss;
	ss << std::hex << std::uppercase << value; // 转换为十六进制并使用大写字母
	return ss.str();
}
std::string TimeTranslate(time_t time)
{
	std::string str_time;
	tm TM;
	localtime_s(&TM, &time);
	str_time.resize(80);
	size_t str_len = strftime(str_time.data(), 80, "%Y-%m-%d	%H:%M:%S", &TM);
	str_time.resize(str_len);
	return str_time;
}

void ExceptionRecord_Code_Translate()
{
	std::string buffer;
	switch (main_info["ExceptionRecord"]["Code"].get<DWORD>())
	{
	case EXCEPTION_ACCESS_VIOLATION:
		buffer = u8"執行緒嘗試讀取或寫入其無權存取的虛擬位址\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access.";
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		buffer = u8"該執行緒嘗試存取超出範圍的數組元素，並且底層硬體支援邊界檢查。\nThe thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.";
		break;
	case EXCEPTION_BREAKPOINT:
		buffer = u8"遇到斷點。\nA breakpoint was encountered.";
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		buffer = u8"執行緒嘗試在不提供對齊的硬體上讀取或寫入未對齊的資料。例如，16 位元值必須與 2 位元組邊界對齊； 4 位元組邊界上的 32 位元值，等等。\nThe thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.";
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		buffer = u8"浮點運算中的一個操作數是非規範的。非規範值是由於太小而無法表示為標準浮點值的值。\nOne of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value.";
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		buffer = u8"該執行緒嘗試將浮點數值除以浮點除數零。\nThe thread tried to divide a floating-point value by a floating-point divisor of zero.";
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		buffer = u8"浮點運算的結果不能精確地表示為小數。\nThe result of a floating-point operation cannot be represented exactly as a decimal fraction.";
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		buffer = u8"此異常代表此清單中未包含的任何浮點異常。\nThis exception represents any floating-point exception not included in this list.";
		break;
	case EXCEPTION_FLT_OVERFLOW:
		buffer = u8"浮點運算的指數大於對應類型允許的幅度。\nThe exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.";
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		buffer = u8"浮點運算導致堆疊溢位或下溢。\nThe stack overflowed or underflowed as the result of a floating-point operation.";
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		buffer = u8"浮點運算的指數小於對應類型允許的幅度。\nThe exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.";
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		buffer = u8"該執行緒嘗試執行無效指令。\nThe thread tried to execute an invalid instruction.";
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		buffer = u8"該線程嘗試訪問不存在的頁面，系統無法載入該頁面。例如，如果在透過網路運行程式時網路連線遺失，則可能會發生此異常。\nThe thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.";
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		buffer = u8"該執行緒嘗試將整數值除以零整數除數。\nThe thread tried to divide an integer value by an integer divisor of zero.";
		break;
	case EXCEPTION_INT_OVERFLOW:
		buffer = u8"整數運算的結果導致結果的最高有效位元發生進位。\nThe result of an integer operation caused a carry out of the most significant bit of the result.";
		break;
	case EXCEPTION_INVALID_DISPOSITION:
		buffer = u8"異常處理程序向異常調度程序傳回了無效的配置。使用 C 等高階語言的程式設計師永遠不會遇到這種異常。\nAn exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception.";
		break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		buffer = u8"異常_不可繼續_異常\nThe thread tried to continue execution after a noncontinuable exception occurred.";
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		buffer = u8"執行緒嘗試執行目前機器模式下不允許操作的指令。\nThe thread tried to execute an instruction whose operation is not allowed in the current machine mode.";
		break;
	case EXCEPTION_SINGLE_STEP:
		buffer = u8"追蹤陷阱或其他單指令機製表示一條指令已被執行。\nA trace trap or other single-instruction mechanism signaled that one instruction has been executed.";
		break;
	case EXCEPTION_STACK_OVERFLOW:
		buffer = u8"該線程用完了其堆疊。\nThe thread used up its stack.";
		break;
	default:
		buffer = u8"未知异常 Unknown exception";
		break;
	}
	main_info["ExceptionRecord"]["Code"] = std::string("0x").append(longLongToHexString(main_info["ExceptionRecord"]["Code"].get<DWORD>())).append(" ) : ").append(buffer);
}
void ExceptionRecord_Flags_Translate()
{
	std::string buffer;
	switch (main_info["ExceptionRecord"]["Flags"].get<int>())
	{
	case EXCEPTION_NONCONTINUABLE:
		buffer = u8"不可繼續的異常 Noncontinuable exception";
		break;
	case EXCEPTION_UNWINDING:
		buffer = u8"放鬆正在進行中 Unwind is in progress";
		break;
	case EXCEPTION_EXIT_UNWIND:
		buffer = u8"退出展開正在進行中 Exit unwind is in progress";
		break;
	case EXCEPTION_STACK_INVALID:
		buffer = u8"堆疊超出限製或未對齊 Stack out of limits or unaligned";
		break;
	case EXCEPTION_NESTED_CALL:
		buffer = u8"巢狀異常處理程序調用 Nested exception handler call";
		break;
	case EXCEPTION_TARGET_UNWIND:
		buffer = u8"目標展開正在進行中  Target unwind in progress";
		break;
	case EXCEPTION_COLLIDED_UNWIND:
		buffer = u8"碰撞異常處理程序調用 Collided exception handler call";
		break;
	case EXCEPTION_SOFTWARE_ORIGINATE:
		buffer = u8"例外源自於軟體 Exception originated in software";
		break;

	default:
		buffer = u8"未知异常 Unknown exception";
		break;
	} 
	main_info["ExceptionRecord"]["Flags"] = buffer;
}
void StackRecords_Flags_Translate(size_t index, StackInfo& info)
{
	ULONG Flags = main_info["stack"][index]["flags"];
	if (Flags& SYMFLAG_CLR_TOKEN) {info.Flags.push_back(u8"符號是 CLR 令牌 (SYMFLAG_CLR_TOKEN)");}
	if (Flags & SYMFLAG_CONSTANT) { info.Flags.push_back(u8"符號是常數 (SYMFLAG_CONSTANT)"); }
	if (Flags & SYMFLAG_EXPORT) { info.Flags.push_back(u8"符號來自導出數據表 (SYMFLAG_EXPORT)"); }
	if (Flags & SYMFLAG_FORWARDER) { info.Flags.push_back(u8"符號是轉寄站。 (SYMFLAG_FORWARDER)"); }
	if (Flags & SYMFLAG_FRAMEREL) { info.Flags.push_back(u8"位移是相對的框架 (SYMFLAG_FRAMEREL)"); }
	if (Flags & SYMFLAG_FUNCTION) { info.Flags.push_back(u8"符號是已知的函式 (SYMFLAG_FUNCTION)"); }
	if (Flags & SYMFLAG_ILREL) { info.Flags.push_back(u8"符號位址是相對於中繼語言區塊開頭的位移。 這僅適用於Managed程式代碼。 (SYMFLAG_ILREL)"); }
	if (Flags & SYMFLAG_LOCAL) { info.Flags.push_back(u8"符號是局部變數 (SYMFLAG_LOCAL)"); }
	if (Flags & SYMFLAG_METADATA) { info.Flags.push_back(u8"符號是Managed元數據 (SYMFLAG_METADATA)"); }
	if (Flags & SYMFLAG_PARAMETER) { info.Flags.push_back(u8"符號是參數。 (SYMFLAG_PARAMETER)"); }
	if (Flags & SYMFLAG_REGISTER) { info.Flags.push_back(u8"符號是緩存器。 使用 Register 成員。 (SYMFLAG_REGISTER)"); }
	if (Flags & SYMFLAG_REGREL) { info.Flags.push_back(u8"位移是相對的緩存器。 (SYMFLAG_REGREL)"); }
	if (Flags & SYMFLAG_SLOT) { info.Flags.push_back(u8"符號是Managed程式代碼位置。 (SYMFLAG_SLOT)"); }
	if (Flags & SYMFLAG_THUNK) { info.Flags.push_back(u8"符號是 Thunk。 (SYMFLAG_THUNK)"); }
	if (Flags & SYMFLAG_TLSREL) { info.Flags.push_back(u8"符號是 TLS 資料區域的位移。 (SYMFLAG_TLSREL)"); }
	if (Flags & SYMFLAG_VALUEPRESENT) { info.Flags.push_back(u8"使用 Value 成員。 (SYMFLAG_VALUEPRESENT)"); info.have_value = true; }
	if (Flags & SYMFLAG_VIRTUAL) { info.Flags.push_back(u8"符號是由 SymAddSymbol 函式所建立的虛擬符號。 (SYMFLAG_VIRTUAL)"); info.DebugTool_Stack = true; }
}
void StackRecords_Tag_Translate(size_t index, StackInfo& info)
{

	switch (main_info["stack"][index]["tag"].get<long>())
	{
	case H_SymTagNull:
		info.tag = "null";
		break;
	case H_SymTagExe:
		info.tag = "exe";
		break;
	case H_SymTagCompiland:
		info.tag = "Compiland";
		break;
	case H_SymTagCompilandDetails:
		info.tag = "CompilandDetails";
		break;
	case H_SymTagCompilandEnv:
		info.tag = "CompilandEnv";
		break;
	case H_SymTagFunction:
		info.tag = "Function";
		break;
	case H_SymTagBlock:
		info.tag = "Block";
		break;
	case H_SymTagData:
		info.tag = "Data";
		break;
	case H_SymTagAnnotation:
		info.tag = "Annotation";
		break;
	case H_SymTagLabel:
		info.tag = "Label";
		break;
	case H_SymTagPublicSymbol:
		info.tag = "PublicSymbol";
		break;
	case H_SymTagUDT:
		info.tag = "UDT";
		break;
	case H_SymTagEnum_RN:
		info.tag = "Enum";
		break;
	case H_SymTagFunctionType:
		info.tag = "FunctionType";
		break;
	case H_SymTagPointerType:
		info.tag = "PointerType";
		break;
	case H_SymTagArrayType:
		info.tag = "ArrayType";
		break;
	case H_SymTagBaseType:
		info.tag = "BaseType";
		break;
	case H_SymTagTypedef:
		info.tag = "Typedef";
		break;
	case H_SymTagBaseClass:
		info.tag = "BaseClass";
		break;
	case H_SymTagFriend:
		info.tag = "Friend";
		break;
	case H_SymTagFunctionArgType:
		info.tag = "FunctionArgType";
		break;
	case H_SymTagFuncDebugStart:
		info.tag = "FuncDebugStart";
		break;
	case H_SymTagFuncDebugEnd:
		info.tag = "FuncDebugEnd";
		break;
	case H_SymTagUsingNamespace:
		info.tag = "UsingNamespace";
		break;
	case H_SymTagVTableShape:
		info.tag = "VTableShape";
		break;
	case H_SymTagVTable:
		info.tag = "VTable";
		break;
	case H_SymTagCustom:
		info.tag = "Custom";
		break;
	case H_SymTagThunk:
		info.tag = "Thunk";
		break;
	case H_SymTagCustomType:
		info.tag = "CustomType";
		break;
	case H_SymTagManagedType:
		info.tag = "ManagedType";
		break;
	case H_SymTagDimension:
		info.tag = "Dimension";
		break;
	default:
		break;
	}
}

void StackRecords_Mod_Characteristics_Translate(Mod& mod,WORD Characteristics)
{
	if (Characteristics & IMAGE_FILE_RELOCS_STRIPPED) { mod.Characteristics.push_back(u8"圖像檔案被重新定位到 (IMAGE_FILE_RELOCS_STRIPPED)"); }
	if (Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) { mod.Characteristics.push_back(u8"該文件是可執行的（沒有未解析的外部引用）。 (IMAGE_FILE_EXECUTABLE_IMAGE)"); }
	if (Characteristics & IMAGE_FILE_LINE_NUMS_STRIPPED) { mod.Characteristics.push_back(u8"COFF 行號已從檔案中刪除。 (IMAGE_FILE_LINE_NUMS_STRIPPED)"); }
	if (Characteristics & IMAGE_FILE_LOCAL_SYMS_STRIPPED) { mod.Characteristics.push_back(u8"COFF 符號表條目已從檔案中刪除。 (IMAGE_FILE_LOCAL_SYMS_STRIPPED)"); }
	if (Characteristics & IMAGE_FILE_AGGRESIVE_WS_TRIM) { mod.Characteristics.push_back(u8"積極修剪工作集。此值已過時。 (IMAGE_FILE_AGGRESIVE_WS_TRIM)"); }
	if (Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) { mod.Characteristics.push_back(u8"該應用程式可以處理大於 2 GB 的位址。 (IMAGE_FILE_LARGE_ADDRESS_AWARE)"); }
	if (Characteristics & IMAGE_FILE_BYTES_REVERSED_LO) { mod.Characteristics.push_back(u8"該字的位元組被反轉。此標誌已經過時。 (IMAGE_FILE_BYTES_REVERSED_LO)"); }
	if (Characteristics & IMAGE_FILE_32BIT_MACHINE) { mod.Characteristics.push_back(u8"計算機支援32位元字。 (IMAGE_FILE_32BIT_MACHINE)"); }
	if (Characteristics & IMAGE_FILE_DEBUG_STRIPPED) { mod.Characteristics.push_back(u8"調試資訊已被刪除並單獨儲存在另一個檔案中。 (IMAGE_FILE_DEBUG_STRIPPED)"); }
	if (Characteristics & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP) { mod.Characteristics.push_back(u8"如果映像位於可移動媒體上，請將其複製到交換檔案並從交換檔案執行。 (IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP)"); }
	if (Characteristics & IMAGE_FILE_NET_RUN_FROM_SWAP) { mod.Characteristics.push_back(u8"如果映像在網路上，則將其複製到交換檔案並從交換檔案執行。 (IMAGE_FILE_NET_RUN_FROM_SWAP)"); }
	if (Characteristics & IMAGE_FILE_SYSTEM) { mod.Characteristics.push_back(u8"該圖像是一個系統檔案。 (IMAGE_FILE_SYSTEM)"); }
	if (Characteristics & IMAGE_FILE_DLL) { mod.Characteristics.push_back(u8"該圖像是一個 DLL 檔案。雖然它是一個可執行文件，但不能直接運行。 (IMAGE_FILE_DLL)"); }
	if (Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY) { mod.Characteristics.push_back(u8"該檔案只能在單處理器計算機上運作。 (IMAGE_FILE_UP_SYSTEM_ONLY)"); }
	if (Characteristics & IMAGE_FILE_BYTES_REVERSED_HI) { mod.Characteristics.push_back(u8"該字的位元組被反轉。此標誌已經過時。 (IMAGE_FILE_BYTES_REVERSED_HI)"); }

}
void StackRecords_Mod_DllCharacteristics_Translate(Mod& mod,WORD DllCharacteristics)
{
	switch (DllCharacteristics)
	{
	case 0x0020:
		mod.DllCharacteristics = u8"具有 64 位元位址空間的 ASLR。";
		break;
	case 0x0040:
		mod.DllCharacteristics = u8"DLL 可以在載入時重新定位。";
		break;
	case 0x0080:
		mod.DllCharacteristics = u8"強制進行程式碼完整性檢查。\n如果設定了此標誌且某個部分僅包含未初始化的數據，則將該部分IMAGE_SECTION_HEADER的 PointerToRawData成員設為零；\n否則，由於無法驗證數位簽名，影像將無法載入";
		break;
	case 0x0100:
		mod.DllCharacteristics = u8"此影像與資料執行保護（DEP）相容。";
		break;
	case 0x0200:
		mod.DllCharacteristics = u8"影像具有隔離意識，但不應被隔離。";
		break;
	case 0x0400:
		mod.DllCharacteristics = u8"此影像不使用結構化異常處理（SEH）。在此圖像中無法呼叫任何處理程序。";
		break;
	case 0x0800:
		mod.DllCharacteristics = u8"不綁定圖像。";
		break;
	case 0x1000:
		mod.DllCharacteristics = u8"映像應該在 AppContainer 中執行。";
		break;
	case 0x2000:
		mod.DllCharacteristics = u8"WDM 驅動程式。";
		break;
	case 0x4000:
		mod.DllCharacteristics = u8"影像支援控制流保護。";
		break;
	case 0x8000:
		mod.DllCharacteristics = u8"此圖像可辨識終端伺服器。";
		break;
	default:
		break;
	}
}
void StackRecords_Mod_Magic_Translate(Mod& mod,WORD Magic)
{
	switch (Magic)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		mod.Magic = u8"該檔案是一個32位可執行映像";
		break;
	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		mod.Magic = u8"該檔案是一個64位可執行映像";
		break;
	case IMAGE_ROM_OPTIONAL_HDR_MAGIC:
		mod.Magic = u8"該檔案是 ROM 映像";
		break;
	default:
		mod.Magic = "null";
		break;
	}
}
void StackRecords_Mod_Subsystem_Translate(Mod& mod,WORD Subsystem)
{
	switch (Subsystem)
	{
	case IMAGE_SUBSYSTEM_UNKNOWN:
		mod.Subsystem = u8"未知子系統。";
		break;
	case IMAGE_SUBSYSTEM_NATIVE:
		mod.Subsystem = u8"無需子系統（裝置驅動程式和本機系統進程）。";
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_GUI:
		mod.Subsystem = u8"Windows 圖形使用者介面 (GUI) 子系統。";
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_CUI:
		mod.Subsystem = u8"Windows 字元模式使用者介面 (CUI) 子系統。";
		break;
	case IMAGE_SUBSYSTEM_OS2_CUI:
		mod.Subsystem = u8"OS/2 CUI 子系統。";
		break;
	case IMAGE_SUBSYSTEM_POSIX_CUI:
		mod.Subsystem = u8"POSIX CUI 子系統。";
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
		mod.Subsystem = u8"Windows CE 系統。";
		break;
	case IMAGE_SUBSYSTEM_EFI_APPLICATION:
		mod.Subsystem = u8"可擴充韌體介面 (EFI) 應用程式。";
		break;
	case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
		mod.Subsystem = u8"具有啟動服務的 EFI 驅動程式。";
		break;
	case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
		mod.Subsystem = u8"具有運行時服務的 EFI 驅動程式。";
		break;
	case IMAGE_SUBSYSTEM_EFI_ROM:
		mod.Subsystem = u8"EFI ROM 映像。";
		break;
	case IMAGE_SUBSYSTEM_XBOX:
		mod.Subsystem = u8"Xbox 系統。";
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION:
		mod.Subsystem = u8"啟動應用程式。";
		break;
	default:
		break;
	}
}
void StackRecords_Mod_Time_Translate(Mod& mod, DWORD TimeDateStamp)
{
	mod.time = TimeTranslate(TimeDateStamp);
	//time_t time = TimeDateStamp;
	//tm TM;
	//localtime_s(&TM, &time);
	//mod.time.resize(80);
	//size_t str_len = strftime(mod.time.data(),80,"%Y-%m-%d	%H:%M:%S", &TM);
	//mod.time.resize(str_len);
}


int FlagWidget(const char* label,ImVec2 size_arg = ImVec2())
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return 0;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(size, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return 0;

	bool hovered = ImGui::ItemHoverable(bb, id, 0);

	// Render
	const ImU32 col = ImGui::GetColorU32(hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	ImGui::RenderNavHighlight(bb, id);
	ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

	ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);



	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return size.x;
}
void FlagsWidgetList(std::vector<std::string>& flags,const char* title)
{
	if (!flags.empty())
	{
		if (ImGui::BeginChild((int)&flags, ImVec2(), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_MenuBar))
		{
			if (ImGui::BeginMenuBar())
			{
				ImGui::Text(title);
				ImGui::EndMenuBar();
			}

			ImGuiStyle& style = ImGui::GetStyle();
			float window_visible_x2 = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
			for (int n = 0; n < flags.size(); n++)
			{
				ImGui::PushID(n);
				int size_buff = FlagWidget(flags[n].c_str());
				float last_button_x2 = ImGui::GetItemRectMax().x;
				float next_button_x2 = last_button_x2 + style.ItemSpacing.x + size_buff; // Expected position if next button was on same line
				if (n + 1 < flags.size() && next_button_x2 < window_visible_x2)
					ImGui::SameLine();
				ImGui::PopID();
			}
		}
		ImGui::EndChild();
	}
}

void InitMod(Mod& mod,nlohmann::json j)
{
	if (!j["Machine"].is_null())
	{
		mod.only_path = false;
		StackRecords_Mod_Characteristics_Translate(mod, j["Characteristics"]);
		StackRecords_Mod_DllCharacteristics_Translate(mod, j["DllCharacteristics"]);
		StackRecords_Mod_Magic_Translate(mod, j["Magic"]);
		StackRecords_Mod_Subsystem_Translate(mod, j["Subsystem"]);
		StackRecords_Mod_Time_Translate(mod, j["time"]);

		mod.ImageVersion = j["ImageVersion"];
		mod.LinkerVersion = j["LinkerVersion"];
		mod.Machine = j["Machine"];
		mod.OperatingSystemVersion = j["OperatingSystemVersion"];
		mod.SubsystemVersion = j["SubsystemVersion"];
	}
	mod.path = j["path"];
}

std::string current_exe_path_var;
std::string H_GUI::current_exe_path()
{
	return current_exe_path_var;
}

void H_GUI::init(std::string report_path)
{
	TitleImage = LoadTexture("asset\\TitleImage.png");
	windows =(SDL_Window*) GetSDL2_Window();
	
	std::ifstream report_file(report_path);
	if (report_file.good())
	{
		try
		{
			main_info = nlohmann::json::parse(report_file);
		}
		catch (const nlohmann::json::exception& err)
		{
			main_info = nlohmann::json();
		}

		report_file.close();
	}
	else
	{
		report_file.close();
		std::ifstream base_file("Crash\\CrashMessage.HCrash");
		if (base_file.good()) {
			try
			{
				main_info = nlohmann::json::parse(base_file);
			}
			catch (const nlohmann::json::exception& err)
			{
				main_info = nlohmann::json();
			}
		}
		else
		{
			main_info = nlohmann::json();
		}
		base_file.close();
	}

	if (!main_info.is_null())
	{
		ExceptionRecord_Code_Translate();
		ExceptionRecord_Flags_Translate();


		for (size_t i = 0; i < main_info["stack"].size(); i++)
		{
			StackInfo info;
			StackRecords_Flags_Translate(i, info);
			StackRecords_Tag_Translate(i, info);
			info.address = main_info["stack"][i]["address"];
			info.name = main_info["stack"][i]["name"];
			info.value = main_info["stack"][i]["value"];
			InitMod(info.mod, main_info["stack"][i]["mod"]);
			Stacks.push_back(info);
		}
	}
}

void ExceptionRecordViewer()
{
	static nlohmann::json RecordViewer = main_info["ExceptionRecord"];
	
	if (ImGui::BeginChild("ExceptionRecordViewer",ImVec2(),ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY,ImGuiWindowFlags_MenuBar))
	{
		if (ImGui::BeginMenuBar())
		{
			ImGui::Text(u8"異常記錄檢視器 ExceptionRecordViewer");
			ImGui::EndMenuBar();
		}
		

		ImGui::Text(u8"錯誤地址 (Address) : 0x%s", RecordViewer["Address"].get<std::string>().c_str());
		ImGui::Text(u8"錯誤信息 (Code : %s", RecordViewer["Code"].get<std::string>().c_str());
		ImGui::Text(u8"錯誤標誌 : %s", RecordViewer["Flags"].get<std::string>().c_str());
		//if (ImGui::BeginListBox(u8"信息 :"))
		//{
		//	int i = 1;
		//	for (std::string j : RecordViewer["Information"].get<std::vector<std::string>>())
		//	{
		//		ImGui::Text("%d. %s", i++, j.c_str());
		//	}
		//}
		//ImGui::EndListBox();
	}
	ImGui::EndChild();
}

void ModViewer(Mod& mod)
{
	if (ImGui::BeginChild((int)&mod,ImVec2(), ImGuiChildFlags_Border| ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_MenuBar))
	{
		if (ImGui::BeginMenuBar())
		{
			ImGui::Text(u8"模組信息 mod info");
			ImGui::EndMenuBar();
		}

		ImGui::Text(u8"路徑 Path : %s", mod.path.c_str()); 
		ImGui::SameLine();
		if (ImGui::Button(u8"開啓路徑 OpenFolder"))
		{
			try
			{
				std::filesystem::path path(mod.path);
				path = path.parent_path();
	#ifdef _WIN32
				std::string command = "explorer \"" + path.string() + "\"";
				std::system(command.c_str());
	#else
				// 在其他操作系统上，可以使用xdg-open（Linux）或其他命令
				std::string command = "xdg-open \"" + path.string() + "\""; // Linux
				std::system(command.c_str());
	#endif
			}
			catch (const std::filesystem::filesystem_error& err)
			{
				MessageBoxA(0, err.what(), "filesystem error", 0);
			}
		}

		if (!mod.only_path)
		{
			FlagsWidgetList(mod.Characteristics, u8"特徵 Characteristics");
			ImGui::Text(u8"DLL 特徵 DllCharacteristics : %s", mod.DllCharacteristics.c_str());
			ImGui::Text(u8"機器 Machine : %s", mod.Machine.c_str());
			ImGui::Text(u8"映像類型 Magic : %s", mod.Magic.c_str());
			ImGui::Text(u8"作業系統版本 OperatingSystemVersion : %s", mod.OperatingSystemVersion.c_str());
			ImGui::Text(u8"映像版本 ImageVersion : %s", mod.ImageVersion.c_str());
			ImGui::Text(u8"連結器版本 LinkerVersion : %s", mod.LinkerVersion.c_str());
			ImGui::Text(u8"子系統 Subsystem : %s", mod.Subsystem.c_str());
			ImGui::Text(u8"子系統版本 SubsystemVersion : %s", mod.SubsystemVersion.c_str());
			ImGui::Text(u8"時間 Time : %s", mod.time.c_str());
		}

	}
	ImGui::EndChild();
}

void StackRecordsViewer()
{

	static bool AllMessage = false;
	if (ImGui::BeginChild("StackRecordViewer", ImVec2(), ImGuiChildFlags_Border, ImGuiWindowFlags_MenuBar))
	{
		if (ImGui::BeginMenuBar())
		{
			ImGui::Text(u8"堆疊記錄查看器 StackRecordViewer");
			ImGui::SetCursorPosX(ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - 115);
			ImGui::Checkbox(u8"所有信息", &AllMessage);
			ImGui::EndMenuBar();
		}


		size_t index = 0;
		for (auto& info : Stacks)
		{
			std::string title = std::to_string(++index).append(". 0x").append(info.address).append("	").append(info.name);
			bool buff = false;
			if (AllMessage)
				buff = ImGui::BeginChild((long long) & info, ImVec2(), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_MenuBar);
			else
				buff = ImGui::TreeNode(title.c_str());
			if (buff)
			{
				if (AllMessage)
				{
					if (ImGui::BeginMenuBar())
					{
						ImGui::Text(title.c_str());
						ImGui::EndMenuBar();
					}
				}
				ImGui::Text(u8"堆疊名稱 StackName : %s",info.name.c_str());
				ImGui::Text(u8"地址     Address : 0x%s", info.address.c_str());
				ImGui::Text(u8"Tag : %s", info.tag.c_str());
				FlagsWidgetList(info.Flags, "Flags ");
				if (info.have_value)
					ImGui::Text(u8"Value : %d", info.value);

				ModViewer(info.mod);
			}
			if (AllMessage)
				ImGui::EndChild();
			else if(buff)
				ImGui::TreePop();
		}

	}
	ImGui::EndChild();
}


nlohmann::json IPInfo()
{
	// 获取 IP 地址和地区信息的 API
	const std::string ip_api_url = "https://ipinfo.io/json";

	// 创建 httplib 客户端
	httplib::Client cli("ipinfo.io");

	// 发送 GET 请求
	auto res = cli.Get("/json");

	if (res && res->status == 200) {
		// 解析 JSON 响应
		nlohmann::json jsonData = nlohmann::json::parse(res->body);
		jsonData["ipinfo"] = true;
		return jsonData;
	}
	else {
		nlohmann::json j;
		std::stringstream msg;
		j["ipinfo"] = false;
		msg << "请求失败. 状态码: " << (res ? std::to_string(res->status) : "无响应");
		j["message"] = msg.str();
		return j;
	}
}

void HGetSystemInfo(sysinfo & info)
{
	// 获取系统信息
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	switch (sysInfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		info.ProcessorArchitecture = u8"x64 (AMD 或 Intel)";
		break;
	case PROCESSOR_ARCHITECTURE_ARM:
		info.ProcessorArchitecture = u8"ARM";
		break;
	case PROCESSOR_ARCHITECTURE_ARM64:
		info.ProcessorArchitecture = u8"ARM64";
		break;
	case PROCESSOR_ARCHITECTURE_IA64:
		info.ProcessorArchitecture = u8"Intel Itanium 型";
		break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		info.ProcessorArchitecture = u8"x86";
		break;
	case PROCESSOR_ARCHITECTURE_UNKNOWN:
		info.ProcessorArchitecture = u8"未知的架構。";
		break;
	default:
		break;
	}
	info.NumberOfProcessors =  sysInfo.dwNumberOfProcessors;

	// 获取内存信息
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);

	info.MemoryTotalPhys = memInfo.ullTotalPhys / (1024 * 1024) ;
	info.MemoryAvailPhys =  memInfo.ullAvailPhys / (1024 * 1024) ;
	info.MemoryLoad = memInfo.dwMemoryLoad;

	// 获取操作系统版本
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionExW(&osvi);
	
	info.OSBuildNumber = osvi.dwBuildNumber;
	info.OSVersion = std::to_string(osvi.dwMajorVersion).append(".").append(std::to_string(osvi.dwMinorVersion));
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	info.OS_CSDVersion = std::string(converter.to_bytes(osvi.szCSDVersion).c_str());
}

std::string cuthead_cutfoot(std::string str)
{
	// 去掉首尾的引号
	if (!str.empty() && str.front() == '"') {
		str.erase(str.begin()); // 去掉开头的引号
	}
	if (!str.empty() && str.back() == '"') {
		str.erase(str.end() - 1); // 去掉结尾的引号
	}
	return str;
}

httplib::MultipartFormData HLoadFile(std::string path,std::string name,bool* is_work=0)
{
	// 读取文件内容
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		if (is_work)
			*is_work = false;
		std::cerr << "无法打开文件: " << path << std::endl;
		MessageBoxA(0, std::string("无法打开文件: ").append(path).c_str(), "LoadFile Error", 0);
		return httplib::MultipartFormData();
	}
	// 读取文件内容到字符串中
	std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	if (is_work)
		*is_work = true;
	return httplib::MultipartFormData{ name, fileContent, name, "application/octet-stream" };
}



void HSendMessage()
{
	nlohmann::json message;
	std::stringstream content;
	time_t current_time;
	std::time(&current_time);


	nlohmann::json embed,ip_embed,sys_embed;
	if (!std::string(user_message).empty())
	{
		embed["description"] = std::string(user_message);
		embed["color"] = 16711748;
		embed["author"]["name"] = u8"崩潰時用戶反饋 :";
	}

	
	try
	{
		nlohmann::json ip = IPInfo();
		if (ip["ipinfo"])
		{
			content << u8"國家/地區 Country : [" << cuthead_cutfoot(ip["country"]) << "](https://www.google.com/search?q=country+" << cuthead_cutfoot(ip["country"]) << ")"
				<< u8"\n城市 City : [" << cuthead_cutfoot(ip["city"]) << "](https://www.google.com/search?q=city+" << cuthead_cutfoot(ip["city"]) << ")"
				<< u8"\n地區 Region : [" << cuthead_cutfoot(ip["region"]) << "](https://www.google.com/search?q=region+" << cuthead_cutfoot(ip["region"]) << ")"
				<< u8"\n時區 TimeZone : [" << cuthead_cutfoot(ip["timezone"]) << "](https://www.google.com/search?q=timezone+" << cuthead_cutfoot(ip["timezone"]) <<")"
				<< u8"\n----------------------------\n主機名稱 hostname : " << cuthead_cutfoot(ip["hostname"])
				<< u8"\nIP 地址 :" << cuthead_cutfoot(ip["ip"])
				<< u8"\n組織 org : " << cuthead_cutfoot(ip["org"])
				<< u8"\n坐標 : [" << cuthead_cutfoot(ip["loc"]) << "](https://www.google.com/maps/place/"<< cuthead_cutfoot(ip["loc"]) <<")"
				<< u8"\n[IP檢測器的讀我](" << cuthead_cutfoot(ip["readme"]) << ")";
		}
		else
		{
			content << ip["message"];
		}
	}
	catch (const nlohmann::json::exception& err)
	{
		content << u8"解析json時出現了錯誤 : " << err.what();
	}


	ip_embed["description"] = content.str();
	ip_embed["color"] = 16753408;
	ip_embed["author"]["name"] = u8"IP信息 :";
	content.clear();
	content = std::stringstream();

	HGetSystemInfo(H_SYSINFO);
	content
		<< u8"處理器架構 : " << H_SYSINFO.ProcessorArchitecture
		<< u8"\n處理器數量 : " << H_SYSINFO.NumberOfProcessors;
		
	if (H_SYSINFO.MemoryTotalPhys > 1024)
		content<< u8"\n----------------------------\n總記憶體 : " << H_SYSINFO.MemoryTotalPhys/1024 << " GB";
	else
		content << u8"\n----------------------------\n總記憶體 : " << H_SYSINFO.MemoryTotalPhys << " MB";

	if (H_SYSINFO.MemoryAvailPhys > 1024)
		content << u8"\n可用記憶體 : " << H_SYSINFO.MemoryAvailPhys/1024 << " GB";
	else
		content << u8"\n可用記憶體 : " << H_SYSINFO.MemoryAvailPhys << " MB";

	content << u8"\n記憶體使用百分比 : " << H_SYSINFO.MemoryLoad << "%"
		<< u8"\n----------------------------\n 系統版本 : " << H_SYSINFO.OSVersion
		<< u8"\n系統構建號 : " << H_SYSINFO.OSBuildNumber
		<<u8"\n系統CSD版本 : " << H_SYSINFO.OS_CSDVersion;

	sys_embed["description"] = content.str();
	sys_embed["color"] = 65467;
	sys_embed["author"]["name"] = u8"系統信息 :";

	if(!embed.is_null())
		message["embeds"].push_back(embed);
	message["embeds"].push_back(ip_embed);
	message["embeds"].push_back(sys_embed);
	//message["tts"] = true;
	
	bool File_A, File_B;
	std::vector<httplib::MultipartFormData> items;
	items.emplace_back(HLoadFile("Crash\\CrashMessage.HCrash","CrashMessage.HCrash", &File_A));
	items.emplace_back(HLoadFile("Crash\\CrashDump.dmp", "CrashDump.dmp", &File_B));
	content = std::stringstream();
	content << u8"<@680032080138469394>\n日期時間 : "
		<< TimeTranslate(current_time)
		<< u8"\n項目 : " PORJECT_NAME
		<< u8"\n版本 :"  PORJECT_Var
		<< u8"\n----------------------------\n 文件 `CrashDump.dmp ` : " << (File_B?u8"用戶端讀取成功!":u8"用戶端讀取失敗!")
		<< u8"\n文件 `CrashMessage.HCrash ` : " << (File_A ?u8"用戶端讀取成功!":u8"用戶端讀取失敗!");
	message["content"] = content.str();
	items.emplace_back(httplib::MultipartFormData{ "payload_json", message.dump(), "", "application/json" });


	httplib::Client cli(WEB_HOOK_URL);
	auto res = cli.Post(WEB_HOOK_URL_PATH, items);


	if (res && res->status == 204) {
		std::cout << "消息已成功发送到 Discord!" << std::endl;
	}
	else {
		std::cerr << "发送失败. 状态码: " << (res ? std::to_string(res->status) : "无响应") << std::endl;
		std::cout << res->body;
	}
}



void H_GUI::update()
{
	ImGui::Image(TitleImage.texture, ImVec2(600, 200)); 
	ImGui::Text(u8"堆疊資訊 :");
	if (ImGui::BeginChild("stack_info",ImVec2(-1,ImGui::GetWindowHeight() - 450),ImGuiChildFlags_Border |ImGuiChildFlags_FrameStyle))
	{
		if (main_info.is_null())
		{
			ImGui::Text(u8"詳細錯誤信息讀取失敗 \nFailed to read detailed error information");
		}
		else
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5);
			ImGui::PushStyleColor(ImGuiCol_ChildBg,ImVec4(0.07, 0.07, 0.07,1));
			ExceptionRecordViewer();
			StackRecordsViewer();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}

	}
	ImGui::EndChild();
	ImGui::Text(u8"錯誤信息 :");
	ImGui::InputTextEx("##user_message", u8"請在這裏描述錯誤發生的情況和現像 :)", user_message, (int)2048, ImVec2(-1, 150),  ImGuiInputTextFlags_Multiline);

	if (ImGui::Button(u8"關閉"))
	{
		close_window();
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"發送并關閉"))
	{
		HSendMessage();
		close_window();
	}
}

void H_GUI::close()
{
	TitleImage.free_texture();
}

void H_GUI::GetCurrentExePath()
{
	char exePath[MAX_PATH];

	// 获取当前可执行文件的完整路径
	HMODULE hModule = GetModuleHandle(NULL);
	if (hModule != NULL) {
		GetModuleFileNameA(hModule, exePath, sizeof(exePath));
		std::filesystem::path path(exePath);
		path = path.parent_path();
		current_exe_path_var = std::string(path.u8string().c_str());
		std::cout << "当前 exe 文件路径: " << current_exe_path_var << std::endl;
	}
	else {
		std::cerr << "获取模块句柄失败" << std::endl;
	}
}
