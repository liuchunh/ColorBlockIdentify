// Dear ImGui: standalone example application for Windows API + OpenGL

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// This is provided for completeness, however it is strongly recommended you use OpenGL with SDL or GLFW.

#include <opencv2/opencv.hpp>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>
#include <tchar.h>
#include <stdio.h>

#include "CCL.h"
#include "Theme.h"

// Data stored per platform window
struct WGL_WindowData { HDC hDC; };

// Data
static HGLRC            g_hRC;          // Rendering context
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;

// Forward declarations of helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
void ResetDeviceWGL();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Support function for multi-viewports
// Unlike most other backend combination, we need specific hooks to combine Win32+OpenGL.
// We could in theory decide to support Win32-specific code in OpenGL backend via e.g. an hypothetical ImGui_ImplOpenGL3_InitForRawWin32().
static void Hook_Renderer_CreateWindow(ImGuiViewport* viewport)
{
    assert(viewport->RendererUserData == NULL);

    WGL_WindowData* data = IM_NEW(WGL_WindowData);
    CreateDeviceWGL((HWND)viewport->PlatformHandle, data);
    viewport->RendererUserData = data;
}

static void Hook_Renderer_DestroyWindow(ImGuiViewport* viewport)
{
    if (viewport->RendererUserData != NULL)
    {
        WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData;
        CleanupDeviceWGL((HWND)viewport->PlatformHandle, data);
        IM_DELETE(data);
        viewport->RendererUserData = NULL;
    }
}

static void Hook_Platform_RenderWindow(ImGuiViewport* viewport, void*)
{
    // Activate the platform window DC in the OpenGL rendering context
    if (WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData)
        wglMakeCurrent(data->hDC, g_hRC);
}

static void Hook_Renderer_SwapBuffers(ImGuiViewport* viewport, void*)
{
    if (WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData)
        ::SwapBuffers(data->hDC);
}

//ImVec2 operator+(const ImVec2& First, const ImVec2& Second) {
//    return ImVec2(First.x + Second.x, First.y + Second.y);
//}

#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

// ---------------------- CCL Code ----------------------
const int MinRunLength = 8; // 最小游程长度, 小于该长度的游程不分配标签, 直接忽略

static cv::Scalar HSVPixelEps = DefaultPixelEps; // HSV 像素误差范围, 用于判断像素是否在指定范围内
cv::VideoCapture cap(0); // 打开默认摄像头

static Run LastRowRun[FindComponent::MaxRunsPerRow], CurrentRowRun[FindComponent::MaxRunsPerRow];
static size_t LastRowRunCount = 0;

static int RunsCount[MaskHeight];
static Component components[FindComponent::MaxComponents];
// 最大连通块的两个角. 用于画出外接矩形
Coordinate LeftTop, RightBottom;

static bool ColorKnown;
static Color TargetColor;
// ---------------------- CCL Code ----------------------

// Main code
int main(int, char**)
//int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui Win32 + OpenGL3 Debug Panel", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize OpenGL
    if (!CreateDeviceWGL(hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
    io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();

    // Win32+GL needs specific hooks for viewport, as there are specific things needed to tie Win32 and GL api.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        IM_ASSERT(platform_io.Renderer_CreateWindow == NULL);
        IM_ASSERT(platform_io.Renderer_DestroyWindow == NULL);
        IM_ASSERT(platform_io.Renderer_SwapBuffers == NULL);
        IM_ASSERT(platform_io.Platform_RenderWindow == NULL);
        platform_io.Renderer_CreateWindow = Hook_Renderer_CreateWindow;
        platform_io.Renderer_DestroyWindow = Hook_Renderer_DestroyWindow;
        platform_io.Renderer_SwapBuffers = Hook_Renderer_SwapBuffers;
        platform_io.Platform_RenderWindow = Hook_Platform_RenderWindow;
    }

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    const float EnglishFontSize = 14.0f;
    const float ChineseFontSize = 15.0f;

    ImFont* fontConsolas = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Consola.ttf", EnglishFontSize, nullptr);
    IM_ASSERT(fontConsolas != nullptr);
    ImFontConfig fontConfig;
    fontConfig.MergeMode = true;
    // 将 Consolas 和 微软雅黑合并为一个字体
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", ChineseFontSize, &fontConfig, io.Fonts->GetGlyphRangesChineseFull());

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    //ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f); // Default Background Color
    ImVec4 clear_color = ImVec4(0.14f, 0.16f, 0.24f, 1.00f);

    // --------------- CCL Code ---------------
    ColorKnown = true;
	TargetColor = Blue;
    // --------------- CCL Code ---------------

    glass::ApplyTheme();

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        if (::IsIconic(hwnd))
        {
            ::Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ----------------  CCL Code  ---------------
        constexpr int TestImgCount = 4;
        static double SumDuration = 0.0;

        //cv::Mat src = cv::imread("C:\\Users\\Lenovo\\Downloads\\output-image.png");
        static std::string message, res, target;
        cv::Mat src, dst;
        cap >> src; // 从摄像头读取一帧图像
        if (src.empty()) {
            std::cerr << "Error: Could not open or find the image!" << std::endl;
            return 1;
        }

        LeftTop.x = LeftTop.y = RightBottom.x = RightBottom.y = 0;
        std::memset(RunsCount, 0, sizeof(RunsCount));
        std::memset(components, 0, sizeof(components));
        LastRowRunCount = 0;

        int Label = 0;

        int width = src.cols;

        for (int y = 0; y < src.rows; y++) {
            int RunIndex = 0;
            int x = 0;

            while (x < width) {
                while (x < width && !IsPixelInRange(ImgHandle::ConvertPixelBGR2HSV(src.at<cv::Vec3b>(y, x)), TargetColor.HSVLower, TargetColor.HSVUpper, HSVPixelEps, ColorKnown)) {
                    x++;
                }

                if (x >= width) {
                    break;
                }

                CurrentRowRun[RunIndex].StartX = x;

                while (x < width && IsPixelInRange(ImgHandle::ConvertPixelBGR2HSV(src.at<cv::Vec3b>(y, x)), TargetColor.HSVLower, TargetColor.HSVUpper, HSVPixelEps, ColorKnown)) {
                    x++;
                }

                CurrentRowRun[RunIndex].EndX = x - 1;
                CurrentRowRun[RunIndex].Label = 0; // 初始化为未标记
                RunIndex++;
            }

            if (y == 0) {
                for (int i = 0; i < RunIndex; i++) {
                    Run* curr = &CurrentRowRun[i];
                    Label++;
                    curr->Label = FindComponent::AllocateLabel(components, Label);
                    FindComponent::UpdateComponent(components, *curr, y);
                }
            }
            else {
                for (int index = 0; index < RunIndex; index++) {
                    Run* curr = &CurrentRowRun[index];
                    bool AllocatedLabel = false;

                    for (int LastRowRunIndex = 0; LastRowRunIndex < static_cast<int>(LastRowRunCount); LastRowRunIndex++) {
                        Run* prev = &LastRowRun[LastRowRunIndex];
                        if (prev->EndX < curr->StartX) continue;
                        if (curr->EndX < prev->StartX) continue;
                        if (curr->Label == 0) {
                            curr->Label = prev->Label;
                        }
                        else {
                            FindComponent::Union(components, curr->Label, prev->Label);
                        }
                        AllocatedLabel = true;
                    }

                    if (!AllocatedLabel) {
                        Label++;
                        curr->Label = FindComponent::AllocateLabel(components, Label);
                    }
                }

                for (int index = 0; index < RunIndex; index++) {
                    Run* curr = &CurrentRowRun[index];
                    if (curr->Label != 0) {
                        curr->Label = FindComponent::Find(components, curr->Label);
                        FindComponent::UpdateComponent(components, *curr, y);
                    }
                }
            }
            std::memcpy(LastRowRun, CurrentRowRun, sizeof(CurrentRowRun));
            LastRowRunCount = RunIndex;
            std::memset(CurrentRowRun, 0, sizeof(CurrentRowRun));
        }

        // FindComponent::PrintComponents(components, Label);

		const cv::Vec3b TargetHSVLower = TargetColor.HSVLower;
		const cv::Vec3b TargetHSVUpper = TargetColor.HSVUpper;

        if (ColorKnown) target = std::format("Target Color HSV Range: [{}, {}, {}] -> [{}, {}, {}]", TargetHSVLower[0], TargetHSVLower[1], TargetHSVLower[2], TargetHSVUpper[0], TargetHSVUpper[1], TargetHSVUpper[2]);
        else target = std::format("Target Color HSV Range: [{}, {}, {}]", TargetHSVLower[0], TargetHSVLower[1], TargetHSVLower[2]);

        size_t maxArea = 0;
        for (int i = 1; i <= Label; i++) {
            if (components[i].area >= maxArea && components[i].parent == i) {
                maxArea = components[i].area;

                LeftTop.x = components[i].minX;
                LeftTop.y = components[i].minY;
                RightBottom.x = components[i].maxX;
                RightBottom.y = components[i].maxY;
            }
        }
        // std::cout << std::format("Current FPS: {}", cap.get(cv::CAP_PROP_FPS)) << std::endl;
        dst = src.clone();

        if (LeftTop.x == 0 && LeftTop.y == 0 && RightBottom.x == 0 && RightBottom.y == 0) {
            // 没有找到任何连通块
            // std::cout << "No connected component found!" << std::endl;
            res = "No connected component found!";
        }
        else {
            // std::cout << std::format("Final Component: Area: {}, Bounding Box: [({}, {}) -> ({}, {})]", maxArea, LeftTop.x, LeftTop.y, RightBottom.x, RightBottom.y) << std::endl;
            res = std::format("Final Component: Area: {}, Bounding Box: [({}, {}) -> ({}, {})]", maxArea, LeftTop.x, LeftTop.y, RightBottom.x, RightBottom.y);
            cv::rectangle(dst, cv::Point(LeftTop.x, LeftTop.y), cv::Point(RightBottom.x, RightBottom.y), cv::Scalar(0, 0, 255), 2);
        }
        cv::imshow("Result Image", dst);
        cv::waitKey(2);
        // ----------------  CCL Code  ---------------

        // ------------ Debug Panel Code ----------------
        ImGuiTabBarFlags TabBarFlags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("MyTabBar", TabBarFlags)){
            if (ImGui::BeginTabItem("Debug Info Panel")){
                static cv::Vec3b centerPixelBGR, centerPixelHSV;

                if (ImGui::Button("Refresh Target")) {
                    ImGui::SameLine();
                    ImGui::Text("Target Refreshed!");
                    centerPixelBGR = src.at<cv::Vec3b>(src.rows / 2, src.cols / 2);
                    centerPixelHSV = ImgHandle::ConvertPixelBGR2HSV(centerPixelBGR);
                    ColorKnown = true;
        
                    if (IsPixelInRange(centerPixelHSV, GreenHSVLower, GreenHSVUpper, HSVConfirmThreshold, true)) {
                        TargetColor = Green;
                    }
                    else if (IsPixelInRange(centerPixelHSV, YellowHSVLower, YellowHSVUpper, HSVConfirmThreshold, true)) {
                        TargetColor = Yellow;
                    }
                    else if (IsPixelInRange(centerPixelHSV, BlueHSVLower, BlueHSVUpper, HSVConfirmThreshold, true)) {
                        TargetColor = Blue;
                    }
                    else {
                        ColorKnown = false;
                        TargetColor.BGR = centerPixelBGR;
                        TargetColor.HSVLower = centerPixelHSV;
                        TargetColor.colorName = "Unknown";
                        TargetColor.HSVUpper = cv::Vec3b(0, 0, 0);
                    }
        
                    // Just for test.
                    // cv::Vec3b centerPixelBGR = {255, 0, 0}; 
                    // TargetHSVLower = BlueHSVLower;
                    // TargetHSVUpper = BlueHSVUpper;
                    // ColorKnown = true;
        
                    // memset(components, 0, sizeof(components));
                    // memset(LastRowRun, 0, sizeof(LastRowRun));
                    // memset(CurrentRowRun, 0, sizeof(CurrentRowRun));
        
                    message = std::format("Center Pixel BGR: [{}, {}, {}]", centerPixelBGR[0], centerPixelBGR[1], centerPixelBGR[2]);
                    if (ColorKnown) {
                        message += std::format(", Refreshed Color HSV: [{}, {}, {}] -> [{}, {}, {}]", TargetHSVLower[0], TargetHSVLower[1], TargetHSVLower[2], TargetHSVUpper[0], TargetHSVUpper[1], TargetHSVUpper[2]);
                    }
                    else {
                        message += std::format(", Refreshed Color HSV: [{}, {}, {}]", TargetHSVLower[0], TargetHSVLower[1], TargetHSVLower[2]);
                    }
                }
                ImGui::Text(target.c_str());
                ImGui::Text(message.c_str());
                ImGui::Text(res.c_str());
                ImGui::Text(std::format("Current FPS: {}", cap.get(cv::CAP_PROP_FPS)).c_str());
        
                ImGui::Text("Tracing Color: ");
                ImGui::SameLine();
                ImU32 color = IM_COL32(TargetColor.BGR[2], TargetColor.BGR[1], TargetColor.BGR[0], 255);
                const ImVec2 size = ImVec2(15, 15);
                ImVec2 CursorPosition = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(CursorPosition, ImVec2(CursorPosition.x + size.x, CursorPosition.y + size.y), color);
                ImGui::Dummy(size);
                if (ColorKnown) {
                    ImGui::Text(std::format("Tracing Color: {}", TargetColor.colorName).c_str());
                }
                else {
                    ImGui::Text("Tracing Unknown Color Type.");
                }
                ImGui::Text(std::format("HSV Pixel Threshold: [{}, {}, {}]", HSVPixelEps[0], HSVPixelEps[1], HSVPixelEps[2]).c_str());

                ImGui::EndTabItem();
            }
            // ------------ Debug Panel Code ----------------

            // ------------ Modify Panel Code ----------------
            if (ImGui::BeginTabItem("Modify Parameters Panel")){
                ImGui::Text("Modify Target Color RGB: ");
                static int R, G, B;
                static int ConfirmR, ConfirmG, ConfirmB;
                static ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;
                static std::string ModifyMsg;
                static bool btnPreviewPushed = false;
                static bool btnConfirmPushed = false;
        
                const float SliderLength = 400;
                ImGui::SetNextItemWidth(SliderLength);
                ImGui::SliderInt("R Channel", &R, 0, 255, "%d", flags);
                ImGui::SetNextItemWidth(SliderLength);
                ImGui::SliderInt("G Channel", &G, 0, 255, "%d", flags);
                ImGui::SetNextItemWidth(SliderLength);
                ImGui::SliderInt("B Channel", &B, 0, 255, "%d", flags);
        
                if (ImGui::Button("Preview Color")) {
                    btnPreviewPushed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Confirm Color")) {
                    btnConfirmPushed = true;
                }
        
                if (btnPreviewPushed){
                    btnPreviewPushed = false;
                    ModifyMsg = std::format("Selected RGB Value: [{}, {}, {}]", R, G, B);
                    ConfirmB = B, ConfirmG = G, ConfirmR = R;
                }
        
                ImGui::Text("%s", ModifyMsg.c_str());
                if (!ModifyMsg.empty()) {
                    ImGui::Text("Color Preview: ");
                    ImGui::SameLine();
                    ImU32 Color = IM_COL32(ConfirmR, ConfirmG, ConfirmB, 255);
                    const ImVec2 Size = ImVec2(20, 20);
                    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + Size.x, ImGui::GetCursorScreenPos().y + Size.y), Color);
                    ImGui::Dummy(Size);
                }
        
                if (btnConfirmPushed) {
                    btnConfirmPushed = false;
                    ConfirmB = B, ConfirmG = G, ConfirmR = R;
                    const cv::Vec3b ConfirmBGR = cv::Vec3b(ConfirmB, ConfirmG, ConfirmR);
                    const cv::Vec3b ConfirmHSV = ImgHandle::ConvertPixelBGR2HSV(ConfirmBGR);
                    TargetColor.BGR = ConfirmBGR;
                    TargetColor.HSVLower = ConfirmHSV;
        
                    ColorKnown = true;
        
                    // 这里的判定范围应从小到大, 不然会出现颜色被覆盖的情况, 例如绿色和黄色的范围有重叠, 如果先判定绿色, 那么黄色就会被判定为绿色
                    if (IsPixelInRange(ConfirmHSV, GreenHSVLower, GreenHSVUpper, HSVConfirmThreshold, true)) {
                        TargetColor = Green;
                    }
                    else if (IsPixelInRange(ConfirmHSV, YellowHSVLower, YellowHSVUpper, HSVConfirmThreshold, true)) {
                        TargetColor = Yellow;
                    }
                    else if (IsPixelInRange(ConfirmHSV, BlueHSVLower, BlueHSVUpper, HSVConfirmThreshold, true)) {
                        TargetColor = Blue;
                    }
                    else {
                        ColorKnown = false;
                        TargetColor.colorName = "Unknown";
                        TargetColor.HSVUpper = cv::Vec3b(0, 0, 0);
                    }
                    ModifyMsg = std::format("Confirmed Color: BGR: [{}, {}, {}], HSV: [{}, {}, {}]", ConfirmBGR[0], ConfirmBGR[1], ConfirmBGR[2], ConfirmHSV[0], ConfirmHSV[1], ConfirmHSV[2]);
                }
        
                ImGui::Text("Preview Settings below, please adjust as needed.");
                
                if (ImGui::Button("Yellow")){
                    TargetColor = Yellow;
                    ColorKnown = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Green")){
                    TargetColor = Green;
                    ColorKnown = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Blue")){
                    TargetColor = Blue;
                    ColorKnown = true;
                }
        
                static int ChannelH = HSVPixelEps[0];
                static int ChannelS = HSVPixelEps[1];
                static int ChannelV = HSVPixelEps[2];
                static std::string EpsMsg;
        
                ImGui::Text("");
                ImGui::Text("Modify Target HSV Pixel Threshold: ");
                ImGui::SetNextItemWidth(SliderLength);
                ImGui::SliderInt("Hue Channel Threshold", &ChannelH, 0, 255, "%d", flags);
                ImGui::SetNextItemWidth(SliderLength);
                ImGui::SliderInt("Saturation Channel Threshold", &ChannelS, 0, 255, "%d", flags);
                ImGui::SetNextItemWidth(SliderLength);
                ImGui::SliderInt("Value Channel Threshold", &ChannelV, 0, 255, "%d", flags);
        
                if (ImGui::Button("Set to Default")){
                    HSVPixelEps[0] = DefaultPixelEps[0];
                    HSVPixelEps[1] = DefaultPixelEps[1];
                    HSVPixelEps[2] = DefaultPixelEps[2];
                    
                    ChannelH = HSVPixelEps[0];
                    ChannelS = HSVPixelEps[1];
                    ChannelV = HSVPixelEps[2];
                }
        
                // 修改后更新 HSVPixelEps
                // 如果不是全部相等, 则更新 HSVPixelEps, 否则不更新
                if ( !(cv::Scalar(ChannelH, ChannelS, ChannelV) == HSVPixelEps) ) {
                    HSVPixelEps = cv::Scalar(ChannelH, ChannelS, ChannelV);
                    EpsMsg = std::format("Updated HSV Pixel Threshold: [{}, {}, {}]", HSVPixelEps[0], HSVPixelEps[1], HSVPixelEps[2]);
                }
                ImGui::Text(EpsMsg.c_str());
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // ------------ Modify Panel Code ----------------

        //// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        //if (show_demo_window)
        //    ImGui::ShowDemoWindow(&show_demo_window);

        //// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        //{
        //    static float f = 0.0f;
        //    static int counter = 0;

        //    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        //    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        //    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        //    ImGui::Checkbox("Another Window", &show_another_window);

        //    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        //    ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        //    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        //        counter++;
        //    ImGui::SameLine();
        //    ImGui::Text("counter = %d", counter);

        //    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        //    ImGui::End();
        //}

        //// 3. Show another simple window.
        //if (show_another_window)
        //{
        //    ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        //    ImGui::Text("Hello from another window!");
        //    if (ImGui::Button("Close Me"))
        //        show_another_window = false;
        //    ImGui::End();
        //}

        // Rendering
        ImGui::Render();
        glViewport(0, 0, g_Width, g_Height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            // Restore the OpenGL rendering context to the main window DC, since platform windows might have changed it.
            wglMakeCurrent(g_MainWindow.hDC, g_hRC);
        }

        // Present
        ::SwapBuffers(g_MainWindow.hDC);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceWGL(hwnd, &g_MainWindow);
    wglDeleteContext(g_hRC);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (g_hRC)
        return true;

    // Create legacy context
    HGLRC tempRC = wglCreateContext(data->hDC);
    if (!tempRC) { ::ReleaseDC(hWnd, data->hDC); return false; }
    if (!wglMakeCurrent(data->hDC, tempRC))
    {
        wglDeleteContext(tempRC);
        ::ReleaseDC(hWnd, data->hDC);
        return false;
    }

    // Get context version
    GLint major = 0, minor = 0;
    glGetIntegerv(0x821B, &major); // GL_MAJOR_VERSION
    glGetIntegerv(0x821C, &minor); // GL_MINOR_VERSION
    const char* gl_version_str = (const char*)glGetString(GL_VERSION);
    if (major == 0 && minor == 0)
        sscanf_s(gl_version_str, "%d.%d", &major, &minor); // Query GL_VERSION in desktop GL 2.x, the string will start with "<major>.<minor>"
    const GLuint gl_version = (GLuint)(major * 100 + minor * 10);

    // Keep temporary context: already OpenGL 3.0+.
    g_hRC = tempRC;
    if (gl_version >= 300)
        return true;

    typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
    const PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

    // GL 3.0
    const int attribs[] =
    {
        0x2091, 3,      // WGL_CONTEXT_MAJOR_VERSION_ARB
        0x2092, 0,      // WGL_CONTEXT_MINOR_VERSION_ARB
        0x9126, 0x0001, // WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB
        0
    };
    HGLRC newRC = nullptr;
    if (wglCreateContextAttribsARB)
        newRC = wglCreateContextAttribsARB(data->hDC, 0, attribs);

    // If we managed to create 3.0+ context: use that one and destroy the temporary OpenGL 2.x compatibility context.
    if (newRC)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(tempRC);
        g_hRC = newRC;
        wglMakeCurrent(data->hDC, g_hRC);
    }

    return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_Width = LOWORD(lParam);
            g_Height = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
