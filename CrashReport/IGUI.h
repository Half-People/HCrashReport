#pragma once
#ifndef IGUI_H
#define IGUI_H
#include <string>

#define WEB_HOOK_URL "https://discord.com"
#define WEB_HOOK_URL_PATH "/api/webhooks/1345755590332252221/wpzveOBWoUO2Tan-jpjbsKx-3YCqb7dV47eFd_hOUg90ldodsBoDsmhrRpkt619EQLrg"
#define PORJECT_NAME "úy‘á"
#define PORJECT_Var "x.x"
typedef void* HTexture;
struct HImage
{
	int image_height = 0, image_width = 0,image_channel =0;
	HTexture texture = nullptr;
	void free_texture();
};
namespace H_GUI
{
	//static std::string current_exe_path;
	HImage LoadTexture(const char* path);
	void* GetSDL2_Window();
	void close_window();

	std::string current_exe_path();
	void init(std::string report_path);
	void update();
	void close();
	void GetCurrentExePath();
}
#endif // !IGUI_H
