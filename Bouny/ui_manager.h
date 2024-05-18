#pragma once
#include <imgui.h>
#include <YYToolkit/Shared.hpp>

enum ImGuiInitState
{
    FetchingSwapchain = 0,
    CreatingContext = 1,
    CreatingContext2 = 2,
    Done = 3,
    Failed = -1
};

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class ui_manager
{
public:
    ui_manager();

private:
    inline static ImGuiInitState g_init_state;
    inline static ID3D11Device* g_device;
    inline static ID3D11DeviceContext* g_context;
    inline static IDXGISwapChain* g_swapchain;
    inline static ID3D11RenderTargetView* g_render_target_view;
    
    static void draw();
    static void frame_callback(YYTK::FWFrame& FrameContext);
    static void resize_callback(YYTK::FWResize& ResizeContext);
    static void wnd_proc_callback(YYTK::FWWndProc& WndProcContext);
};

