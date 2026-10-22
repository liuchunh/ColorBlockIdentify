#pragma once

#include <imgui.h>

namespace glass {
    // ---- 调色板 (想换色只改这几个常量) ----
    static const ImVec4 kAccent = ImVec4(0.45f, 0.52f, 1.00f, 1.00f); // 蓝紫强调色
    static const ImVec4 kAccentDim = ImVec4(0.30f, 0.36f, 0.72f, 1.00f);
    static const ImVec4 kText = ImVec4(0.91f, 0.92f, 0.96f, 1.00f); // 主文字
    static const ImVec4 kTextDim = ImVec4(0.55f, 0.58f, 0.68f, 1.00f); // 次要文字
    static const ImVec4 kBorderGlow = ImVec4(0.60f, 0.66f, 0.92f, 0.32f); // 玻璃反光描边

    // 应用全局样式(圆角 / 间距 / 半透明配色)
    void ApplyTheme()
    {
        ImGuiStyle& s = ImGui::GetStyle();
        ImVec4* c = s.Colors;

        // ---- 圆角 (现代感第一来源) ----
        s.WindowRounding = 12.0f;
        s.ChildRounding = 10.0f;
        s.FrameRounding = 8.0f;
        s.PopupRounding = 10.0f;
        s.ScrollbarRounding = 9.0f;
        s.GrabRounding = 6.0f;
        s.TabRounding = 8.0f;

        // ---- 间距 / 留白 ----
        s.WindowPadding = ImVec2(18, 18);
        s.FramePadding = ImVec2(10, 6);
        s.ItemSpacing = ImVec2(10, 8);
        s.ItemInnerSpacing = ImVec2(8, 6);
        s.CellPadding = ImVec2(8, 6);
        s.IndentSpacing = 22.0f;
        s.ScrollbarSize = 14.0f;
        s.GrabMinSize = 12.0f;

        // ---- 边框 (玻璃的亮边就靠 Border 这一条) ----
        s.WindowBorderSize = 1.0f;
        s.ChildBorderSize = 1.0f;
        s.PopupBorderSize = 1.0f;
        s.FrameBorderSize = 1.0f;   // 输入框细边框, 更现代
        s.TabBorderSize = 1.0f;

        // ---- 抗锯齿 ----
        s.AntiAliasedFill = true;
        s.AntiAliasedLines = true;

        // ---- 颜色 (深色玻璃拟态) ----
        c[ImGuiCol_Text] = kText;
        c[ImGuiCol_TextDisabled] = kTextDim;

        // 窗口/子窗口/弹窗背景 —— 半透明是关键
        c[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.07f, 0.11f, 1.00f);
        c[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.13f, 0.19f, 0.42f); // 玻璃面板底
        c[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.11f, 0.17f, 0.96f);

        // 描边
        c[ImGuiCol_Border] = kBorderGlow;
        c[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        // 输入框
        c[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.17f, 0.24f, 0.75f);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.20f, 0.29f, 0.85f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.32f, 1.00f);

        // 按钮
        c[ImGuiCol_Button] = ImVec4(0.38f, 0.45f, 0.90f, 0.90f);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.47f, 0.54f, 1.00f, 1.00f);
        c[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.36f, 0.75f, 1.00f);

        // 选中项 / 表头 (列表、Selectable、菜单高亮)
        c[ImGuiCol_Header] = ImVec4(0.40f, 0.47f, 0.90f, 0.45f);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.53f, 0.95f, 0.60f);
        c[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.58f, 1.00f, 0.80f);

        // 勾选 / 滑杆
        c[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.96f, 1.00f, 1.00f);
        c[ImGuiCol_SliderGrab] = ImVec4(0.55f, 0.62f, 1.00f, 1.00f);
        c[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.72f, 1.00f, 1.00f);

        // 分隔线
        c[ImGuiCol_Separator] = ImVec4(0.45f, 0.50f, 0.70f, 0.30f);
        c[ImGuiCol_SeparatorHovered] = ImVec4(0.55f, 0.60f, 0.85f, 0.60f);
        c[ImGuiCol_SeparatorActive] = ImVec4(0.60f, 0.66f, 0.95f, 1.00f);

        // 滚动条
        c[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.06f, 0.09f, 0.30f);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.30f, 0.42f, 0.60f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.32f, 0.38f, 0.52f, 0.80f);
        c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.46f, 0.62f, 1.00f);

        // 文本选中 / 拖放目标
        c[ImGuiCol_TextSelectedBg] = ImVec4(0.45f, 0.52f, 1.00f, 0.35f);
        c[ImGuiCol_DragDropTarget] = ImVec4(0.50f, 0.58f, 1.00f, 0.90f);

        // 表格
        c[ImGuiCol_TableHeaderBg] = ImVec4(0.15f, 0.17f, 0.24f, 0.60f);
        c[ImGuiCol_TableBorderStrong] = ImVec4(0.30f, 0.34f, 0.46f, 0.50f);
        c[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.22f, 0.30f, 0.40f);
        c[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        c[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);

        // Tab / 导航
        c[ImGuiCol_Tab] = ImVec4(0.14f, 0.16f, 0.22f, 0.80f);
        c[ImGuiCol_TabHovered] = ImVec4(0.32f, 0.40f, 0.75f, 0.70f);
        c[ImGuiCol_TabActive] = ImVec4(0.26f, 0.32f, 0.55f, 1.00f);
        c[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.13f, 0.18f, 0.80f);
        c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.22f, 0.36f, 1.00f);
    }

} // namespace glass