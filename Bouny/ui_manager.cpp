#include "ui_manager.h"
#include "globals.h"
#include "utils.h"

#include <fstream>
#include <numeric>

void dump_for_ida()
{
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

    std::ofstream file("dump.txt");

    for (int id = 0; id < 100000; id++)
    {
        CScript* script = nullptr;
        auto success = g_module_interface->GetScriptData(id, script);
        if (!AurieSuccess(success)) continue;

        if (script == nullptr) continue;

        auto functions = script->m_Functions;
        if (functions == nullptr)continue;

        auto func = functions->m_ScriptFunction;
        if (func == nullptr)continue;

        auto rva = reinterpret_cast<uintptr_t>(func) - base;
        file << script->m_Name << " " << std::hex << rva << '\n';
    }

    file.close();
}

ui_manager::ui_manager()
{
    g_init_state = FetchingSwapchain;

    auto status = g_module_interface->CreateCallback(
        g_module,
        EVENT_FRAME,
        frame_callback,
        0
    );

    if (!AurieSuccess(status))
    {
        g_module_interface->Print(CM_LIGHTRED, "Failed to register EVENT_FRAME");
        return;
    }

    status = g_module_interface->CreateCallback(
        g_module,
        EVENT_RESIZE,
        resize_callback,
        0
    );

    if (!AurieSuccess(status))
    {
        g_module_interface->Print(CM_LIGHTRED, "Failed to register EVENT_RESIZE");
        return;
    }

    status = g_module_interface->CreateCallback(
        g_module,
        EVENT_WNDPROC,
        wnd_proc_callback,
        0
    );

    if (!AurieSuccess(status))
    {
        g_module_interface->Print(CM_LIGHTRED, "Failed to register EVENT_WNDPROC");
        return;
    }

    g_module_interface->Print(CM_LIGHTGREEN, "Registered all UI callbacks!");
}


void ui_manager::draw()
{
    ImGui::ShowDemoWindow();
    ImGui::Checkbox("Cheat damage", &g_config.cheat_damage);
    if (ImGui::Button("Dump for IDA"))
    {
        dump_for_ida();
    }

    static std::string buffer;
    ImGui::InputText("Script name", &buffer);
    if (ImGui::Button("Hook"))
    {
        auto script_name = std::string(buffer);
        g_hooks->hook_script_runtime(buffer, [script_name](YYTK::CInstance* self, YYTK::CInstance* other,
                                                           YYTK::RValue& return_value, int num_args,
                                                           YYTK::RValue** args,
                                                           ScriptFunction* trampoline)
        {
            trampoline(self, other, return_value, num_args, args);

            if (num_args == 0)
            {
                g_module_interface->Print(CM_GRAY, "%s() -> %s", script_name.c_str(),
                                          utils::rvalue_to_string(&return_value).c_str());
            }
            else
            {
                std::string args_str = std::accumulate(args, args + num_args, std::string{},
                                                       [](std::string acc, YYTK::RValue* arg)
                                                       {
                                                           return acc + utils::rvalue_to_string(arg) + ", ";
                                                       });

                // remove last ,
                args_str.pop_back();
                args_str.pop_back();

                g_module_interface->Print(CM_GRAY, "%s(%s) -> %s", script_name.c_str(), args_str.c_str(),
                                          utils::rvalue_to_string(&return_value).c_str());
            }
        });
    }

    if (ImGui::Button("Unhook"))
    {
        g_hooks->unhook_script(buffer);
    }
}

void ui_manager::frame_callback(YYTK::FWFrame& FrameContext)
{
    auto swapchain = std::get<0>(FrameContext.Arguments());
    if (g_init_state == FetchingSwapchain)
    {
        g_swapchain = swapchain;
        g_init_state = CreatingContext;
    }

    if (g_init_state == Done)
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
        draw();
        ImGui::Render();
        g_context->OMSetRenderTargets(1, &g_render_target_view, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}

void ui_manager::resize_callback(YYTK::FWResize& ResizeContext)
{
    g_module_interface->Print(CM_LIGHTGREEN, "Resize event!");

    if (g_init_state == Done)
    {
        if (g_render_target_view != nullptr)
        {
            g_context->OMSetRenderTargets(0, nullptr, nullptr);
            g_render_target_view->Release();
        }

        ResizeContext.Call();

        ID3D11Texture2D* backBuffer = nullptr;
        g_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        g_device->CreateRenderTargetView(backBuffer, nullptr, &g_render_target_view);
        backBuffer->Release();

        g_context->OMSetRenderTargets(1, &g_render_target_view, nullptr);

        D3D11_VIEWPORT vp;
        vp.Width = std::get<2>(ResizeContext.Arguments());
        vp.Height = std::get<3>(ResizeContext.Arguments());
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        g_context->RSSetViewports(1, &vp);
    }
}

void ui_manager::wnd_proc_callback(YYTK::FWWndProc& WndProcContext)
{
    auto [hWnd, msg, wParam, lParam] = WndProcContext.Arguments();

    if (g_init_state == CreatingContext2)
    {
        if (g_swapchain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_device)) == S_OK)
        {
            g_device->GetImmediateContext(&g_context);

            ID3D11Texture2D* backBuffer = nullptr;
            g_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
            g_device->CreateRenderTargetView(backBuffer, nullptr, &g_render_target_view);
            backBuffer->Release();
            g_context->OMSetRenderTargets(1, &g_render_target_view, nullptr);

            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImGui_ImplWin32_Init(hWnd);
            ImGui_ImplDX11_Init(g_device, g_context);

            g_init_state = Done;
            g_module_interface->Print(CM_LIGHTGREEN, "UI initialized!");
        }
        else
        {
            g_module_interface->Print(CM_LIGHTRED, "Failed to get device!");
            g_init_state = Failed;
        }
    }

    if (g_init_state == CreatingContext)
    {
        g_init_state = CreatingContext2;
    }

    if (g_init_state == Done)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        {
            WndProcContext.Call();
            WndProcContext.Override(1);
        }
    }
}
