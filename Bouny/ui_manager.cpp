#include "ui_manager.h"
#include "globals.h"
#include "utils.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_stdlib.h>

#include <numeric>

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

    if (ImGui::Begin("Stuff"))
    {
        ImGui::Checkbox("Cheat damage", &g_config.cheat_damage);
        ImGui::Checkbox("Tailwinds", &g_config.tailwinds);
        if (ImGui::Button("Hook all bp_"))
        {
            for (int i = 0; i < 100000; i++)
            {
                CScript* script = nullptr;
                g_module_interface->GetScriptData(i, script);
                if (script == nullptr) continue;
                auto name = std::string(script->m_Name);
                if (name.starts_with("bp_"))
                {
                    utils::hook_and_log(name);
                }
            }
        }

        static std::string buffer;
        static std::map<std::string, int> script_map;
        ImGui::InputText("Script name", &buffer);
        if (ImGui::Button("Hook"))
        {
            auto script_name = std::string(buffer);
            if (!script_map.contains(script_name))
            {
                auto id = utils::hook_and_log(script_name);
                script_map[script_name] = id;
            }
        }

        if (ImGui::Button("Unhook"))
        {
            if (script_map.contains(buffer))
            {
                g_hooks->unhook_script(script_map[buffer]);
                script_map.erase(buffer);
            }
        }
    }

    ImGui::End();

    CRoom* room = nullptr;
    g_module_interface->GetCurrentRoomData(room);


    if (room != nullptr)
    {
        std::map<int, CLayer*> layer_map = {};
        std::map<int, CLayerElementBase*> element_map = {};
        std::map<int, std::list<CLayerElementBase*>> layer_element_map = {};
        static std::list<int> open_layers = {};
        static std::list<int> open_elements = {};

        auto layers = room->m_Layers;
        auto layer = layers.m_First;
        while (layer != nullptr)
        {
            layer_map[layer->m_Id] = layer;
            layer_element_map[layer->m_Id] = {};

            auto elements = layer->m_Elements;
            auto element = elements.m_First;
            while (element != nullptr)
            {
                element_map[element->m_ID] = element;
                layer_element_map[layer->m_Id].push_back(element);
                element = element->m_Flink;
            }

            layer = layer->m_Flink;
        }


        if (ImGui::Begin("Room"))
        {
            for (auto& [id, layer] : layer_map)
            {
                auto name = std::format("{} ({})##layer_toggle_{}",
                                        layer->m_Name == nullptr ? "Unnamed" : layer->m_Name, id, id);
                if (ImGui::Button(name.c_str()))
                {
                    if (std::ranges::find(open_layers, id) == open_layers.end())
                    {
                        open_layers.push_back(id);
                    }
                    else
                    {
                        open_layers.remove(id);
                    }
                }
            }
        }
        ImGui::End();

        std::list<int> layers_to_remove = {};
        for (auto id : open_layers)
        {
            if (!layer_map.contains(id))
            {
                layers_to_remove.push_back(id);
                continue;
            }

            auto layer = layer_map[id];
            auto name = std::format("Layer {} ({})##layer_{}", layer->m_Name == nullptr ? "Unnamed" : layer->m_Name, id,
                                    id);
            bool open = true;
            if (ImGui::Begin(name.c_str(), &open))
            {
                for (auto element : layer_element_map[id])
                {
                    auto name = std::format("{} ({})##element_toggle_{}",
                                            element->m_Name == nullptr ? "Unnamed" : element->m_Name, element->m_ID,
                                            element->m_ID);
                    if (ImGui::Button(name.c_str()))
                    {
                        if (std::ranges::find(open_elements, element->m_ID) == open_elements.end())
                        {
                            open_elements.push_back(element->m_ID);
                        }
                        else
                        {
                            open_elements.remove(element->m_ID);
                        }
                    }
                }
            }

            ImGui::End();
            if (!open) layers_to_remove.push_back(id);
        }
        for (auto id : layers_to_remove) open_layers.remove(id);

        std::list<int> elements_to_remove = {};
        for (auto id : open_elements)
        {
            if (!element_map.contains(id))
            {
                elements_to_remove.push_back(id);
                continue;
            }

            auto element = element_map[id];
            auto name = std::format("Element {} ({})##element_{}",
                                    element->m_Name == nullptr ? "Unnamed" : element->m_Name,
                                    element->m_ID, element->m_ID);
            bool open = true;
            if (ImGui::Begin(name.c_str(), &open))
            {
                if (element->m_Type == 2)
                {
                    auto instance = static_cast<CLayerInstanceElement*>(element);
                    g_module_interface->EnumInstanceMembers(instance->m_Instance,
                                                            [](const char* name, RValue* value) -> bool
                                                            {
                                                                ImGui::Text(
                                                                    "%s: %s", name,
                                                                    utils::rvalue_to_string(value).c_str());
                                                                return false;
                                                            });
                }
            }

            ImGui::End();
            if (!open) elements_to_remove.push_back(id);
        }
        for (auto id : elements_to_remove) open_elements.remove(id);
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

        auto io = ImGui::GetIO();
        if ((io.WantCaptureMouse || io.MouseDrawCursor) && !(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
        {
            static std::map<ImGuiMouseCursor, HCURSOR> Cursor = {
                {ImGuiMouseCursor_Arrow, LoadCursor(nullptr, IDC_ARROW)},
                {ImGuiMouseCursor_TextInput, LoadCursor(nullptr, IDC_IBEAM)},
                {ImGuiMouseCursor_ResizeAll, LoadCursor(nullptr, IDC_SIZEALL)},
                {ImGuiMouseCursor_ResizeNS, LoadCursor(nullptr, IDC_SIZENS)},
                {ImGuiMouseCursor_ResizeEW, LoadCursor(nullptr, IDC_SIZEWE)},
                {ImGuiMouseCursor_ResizeNESW, LoadCursor(nullptr, IDC_SIZENESW)},
                {ImGuiMouseCursor_ResizeNWSE, LoadCursor(nullptr, IDC_SIZENWSE)},
                {ImGuiMouseCursor_Hand, LoadCursor(nullptr, IDC_HAND)},
                {ImGuiMouseCursor_NotAllowed, LoadCursor(nullptr, IDC_NO)}
            };
            auto cursor = ImGui::GetMouseCursor();

            if (cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
            {
                SetCursor(nullptr);
            }
            else
            {
                SetCursor(Cursor[cursor]);
            }
        }

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
        vp.Width = static_cast<float>(std::get<2>(ResizeContext.Arguments()));
        vp.Height = static_cast<float>(std::get<3>(ResizeContext.Arguments()));
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
            WndProcContext.Override(1);
        }

        auto io = ImGui::GetIO();
        if (io.WantCaptureKeyboard || io.WantTextInput)
        {
            if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_CHAR)
            {
                WndProcContext.Override(1);
            }
        }

        if (io.WantCaptureMouse)
        {
            if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_LBUTTONDBLCLK ||
                msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP || msg == WM_RBUTTONDBLCLK ||
                msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP || msg == WM_MBUTTONDBLCLK ||
                msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL)
            {
                WndProcContext.Override(1);
            }
        }
    }
}
