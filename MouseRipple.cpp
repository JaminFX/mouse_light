#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <timeapi.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include "resource.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "winmm.lib")

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace Gdiplus;

namespace {

// Message IDs
constexpr UINT_PTR kFrameTimerId = 1;
constexpr UINT kFrameMs = 12;               // ~83 FPS (极速丝滑高刷新，配合 timeBeginPeriod(1))
constexpr UINT WM_RIPPLE_CLICK = WM_APP + 1;
constexpr UINT WM_TRAYICON     = WM_APP + 2;

// Tray menu IDs
constexpr UINT ID_TRAY_SETTINGS  = 1001;
constexpr UINT ID_TRAY_AUTOSTART = 1002;
constexpr UINT ID_TRAY_EXIT      = 1003;

// Settings control IDs
#ifndef CB_SETMINVISIBLE
#define CB_SETMINVISIBLE 0x1701
#endif
constexpr int IDC_PRESET_COMBO        = 2001;
constexpr int IDC_BTN_COLOR_LEFT      = 2002;
constexpr int IDC_BTN_COLOR_RIGHT     = 2003;
constexpr int IDC_BTN_COLOR_MID       = 2004;
constexpr int IDC_TRK_RADIUS          = 2005;
constexpr int IDC_LBL_RADIUS          = 2006;
constexpr int IDC_TRK_DURATION        = 2007;
constexpr int IDC_LBL_DURATION        = 2008;
constexpr int IDC_TRK_THICKNESS       = 2009;
constexpr int IDC_LBL_THICKNESS       = 2010;
constexpr int IDC_TRK_RINGS           = 2011;
constexpr int IDC_LBL_RINGS           = 2012;
constexpr int IDC_CHK_CENTERDOT       = 2013;
constexpr int IDC_BTN_SAVE            = 2014;
constexpr int IDC_BTN_RESET           = 2015;
constexpr int IDC_CHK_AMBIENT         = 2016;
constexpr int IDC_TRK_AMBIENT_RADIUS  = 2017;
constexpr int IDC_LBL_AMBIENT_RADIUS  = 2018;
constexpr int IDC_TRK_AMBIENT_ALPHA   = 2019;
constexpr int IDC_LBL_AMBIENT_ALPHA   = 2020;
constexpr int IDC_BTN_COLOR_AMBIENT   = 2021;
constexpr int IDC_TRK_AMBIENT_BAND    = 2022;
constexpr int IDC_LBL_AMBIENT_BAND    = 2023;
constexpr int IDC_TRK_ALPHA           = 2024;
constexpr int IDC_LBL_ALPHA           = 2025;
constexpr int IDC_CHK_TRAIL           = 2026;
constexpr int IDC_BTN_COLOR_TRAIL     = 2027;
constexpr int IDC_TRK_TRAIL_DURATION  = 2028;
constexpr int IDC_LBL_TRAIL_DURATION  = 2029;
constexpr int IDC_TRK_TRAIL_WIDTH     = 2030;
constexpr int IDC_LBL_TRAIL_WIDTH     = 2031;
constexpr int IDC_TRK_TRAIL_ALPHA     = 2032;
constexpr int IDC_LBL_TRAIL_ALPHA     = 2033;
constexpr int IDC_CHK_ANNOTATION      = 2034;
constexpr int IDC_BTN_COLOR_INK       = 2035;
constexpr int IDC_BTN_COLOR_ARROW     = 2036;
constexpr int IDC_TRK_INK_WIDTH       = 2037;
constexpr int IDC_LBL_INK_WIDTH       = 2038;
constexpr int IDC_TRK_ANNOTATION_HOLD = 2039;
constexpr int IDC_LBL_ANNOTATION_HOLD = 2040;

enum StylePreset {
    PRESET_WACOM = 0,
    PRESET_WATER_RIPPLE = 1,
    PRESET_CUSTOM = 2
};

// 点击特效类型：0 = 原来的水波纹，其余是新增的各种特效
enum ClickEffect {
    EFFECT_RIPPLE = 0,
    EFFECT_STARS = 1,        // 小星星闪烁
    EFFECT_LIGHTNING = 2,    // 闪电盘旋
    EFFECT_PETALS = 3,       // 花瓣
    EFFECT_INK = 4,          // 水墨
    EFFECT_FIREWORK = 5,     // 烟花
    EFFECT_BUBBLES = 6,      // 泡泡
    EFFECT_HEARTS = 7,       // 爱心
    EFFECT_SNOW = 8,         // 雪花
    EFFECT_LEAVES = 9,       // 落叶
    EFFECT_PAWS = 10,        // 猫爪印
    EFFECT_COINS = 11,       // 金币
    EFFECT_CONFETTI = 12,    // 彩带礼花
    EFFECT_METEOR = 13,      // 流星
    EFFECT_BUTTERFLY = 14,   // 蝴蝶
    EFFECT_FIREFLY = 15,     // 萤火虫
    EFFECT_NOTES = 16,       // 音符
    EFFECT_STARBURST = 17,   // 星环爆裂
    EFFECT_RAIN = 18,        // 雨滴溅起
    EFFECT_MAGIC = 19,       // 魔法阵
    EFFECT_COMIC = 20,       // 漫画爆炸
    EFFECT_RAINBOW = 21,     // 彩虹光环
    EFFECT_BALLOONS = 22,    // 气球
    EFFECT_FLAMES = 23       // 火焰
};

// 鼠标拖尾样式：0 = 原来的墨迹彩带，其余是新增的“散落粒子 / 彩虹带”
enum TrailStyle {
    TRAIL_RIBBON = 0,
    TRAIL_SPARKLE = 1,       // 星屑
    TRAIL_PETAL = 2,         // 花瓣
    TRAIL_SNOW = 3,          // 雪花
    TRAIL_HEART = 4,         // 爱心
    TRAIL_BUBBLE = 5,        // 泡泡
    TRAIL_FIREFLY = 6,       // 萤火虫
    TRAIL_NOTE = 7,          // 音符
    TRAIL_EMBER = 8,         // 火星
    TRAIL_LEAF = 9,          // 落叶
    TRAIL_RAINBOW = 10,      // 彩虹带
    TRAIL_GLITTER = 11       // 彩屑
};

struct AppConfig {
    int stylePreset = PRESET_WACOM;
    COLORREF leftColor = RGB(232, 65, 82);     // Wacom coral red
    COLORREF rightColor = RGB(41, 128, 245);   // Wacom crisp blue
    COLORREF middleColor = RGB(245, 166, 35);  // Warm amber
    int maxRadius = 28;                        // px (Wacom tight radius)
    int durationMs = 300;                      // ms (Fast & snappy)
    int ringThickness = 2;                     // px
    int ringCount = 1;                         // 1: pure Wacom ring, 2-3: ripples
    bool centerDot = true;                     // Subtle center contact flash
    int maxAlpha = 180;                        // Alpha opacity (默认 180 ≈ 70% 透明度，鲜明柔和)
    int clickEffect = 0;                       // 点击特效类型，见 enum ClickEffect (0=水波纹)
    int trailStyle = 0;                        // 拖尾样式，见 enum TrailStyle (0=墨迹彩带)

    // Ambient continuous ripple settings (常驻动态纯净线条波纹)
    bool ambientRipple = true;                 // 始终开启
    int ambientRadius = 20;                    // 常驻半径 (默认 20px)
    int ambientThickness = 2;                  // 线条粗细 (默认 2px)
    int ambientBand = 2;                       // 兼容旧 ini
    int ambientAlphaPercent = 30;              // 透明度 0 - 100% (默认 30%)
    int ambientAlpha = 76;                     // 255 * 30% ≈ 76
    COLORREF ambientColor = RGB(145, 145, 145);// 柔和自然灰色 (可自定义颜色)

    // Mouse movement ink ribbon trail settings (鼠标移动水墨流线拖尾)
    bool trailEnabled = true;                  // 默认开启（可随时在设置界面关闭）
    COLORREF trailColor = RGB(41, 128, 245);   // 水墨优雅蓝（支持自定义任意调色）
    int trailDurationMs = 380;                 // 留存时长 150 - 800ms
    int trailWidth = 8;                        // 笔触最大粗细 3 - 18px
    int trailAlphaPercent = 80;                // 透明度 10% - 100%
    int trailAlpha = 204;                      // 255 * 80% ≈ 204

    // Teaching annotation settings (教学演示标注: 阅后即焚画笔与快捷箭头)
    bool annotationEnabled = true;             // 默认开启（按住 Ctrl+Alt 唤起）
    COLORREF inkColor = RGB(255, 68, 68);      // 醒目教学红橙色
    int inkWidth = 6;                          // 画笔粗细 (3 - 16px)
    COLORREF arrowColor = RGB(255, 140, 0);    // 亮橙金箭头
    int arrowWidth = 6;                        // 箭头粗细 (3 - 16px)
    int annotationHoldMs = 2200;               // 停留展示时长 (500 - 5000ms)
};

AppConfig g_config;
COLORREF g_customColors[16] = {0};

struct Ripple {
    POINT screenPt{};
    int buttonType = 0; // 0=Left, 1=Right, 2=Middle
    ULONGLONG startedAt = 0;
    unsigned seed = 1;  // 随机种子：每次点击花样都不同
};

struct TrailPoint {
    POINT screenPt{};
    double timeMs = 0.0;
    unsigned seed = 0;      // 粒子随机种子
    bool emit = false;      // 这个点是否“放出”一颗拖尾粒子
};

enum AnnotationType {
    ANNOTATION_INK = 0,
    ANNOTATION_ARROW = 1
};

struct AnnotationStroke {
    AnnotationType type = ANNOTATION_INK;
    std::vector<POINT> points;  // Path points (for ink)
    POINT startPt{};            // For arrow
    POINT endPt{};              // For arrow
    bool isDrawing = true;      // actively drawing with button down
    double startedAt = 0.0;
    double releasedAt = 0.0;
    COLORREF color = RGB(255, 68, 68);
    float width = 6.0f;
    float holdMs = 2200.0f;
    float fadeMs = 800.0f;
};

std::vector<AnnotationStroke> g_annotations;
bool g_isDrawingInk = false;
bool g_isDrawingArrow = false;
HHOOK g_kbdHook = nullptr;

inline double getHighPrecisionMs() {
    static const double invFreq = []() {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return 1000.0 / static_cast<double>(f.QuadPart);
    }();
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) * invFreq;
}

HINSTANCE g_instance = nullptr;
HWND g_overlay = nullptr;
HWND g_settingsWnd = nullptr;
HHOOK g_mouseHook = nullptr;
ULONG_PTR g_gdiplusToken = 0;
HFONT g_uiFont = nullptr;
std::vector<Ripple> g_ripples;
std::vector<TrailPoint> g_trailPoints;
int g_virtualX = 0;
int g_virtualY = 0;
int g_virtualW = 0;
int g_virtualH = 0;

// Utility functions
float clamp01(float x) {
    if (!(x > 0.0f)) return 0.0f;       // 同时挡住负数和 NaN
    if (x > 1.0f) return 1.0f;
    return x;
}

BYTE alphaByte(float a) {
    if (!(a > 0.0f)) return 0;          // 同时挡住负数和 NaN（NaN 以前会被算成 255=全不透明，造成结尾闪烁）
    if (a > 255.0f) a = 255.0f;
    return static_cast<BYTE>(a + 0.5f);
}

std::wstring getIniPath() {
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash) {
        *(lastSlash + 1) = L'\0';
        return std::wstring(path) + L"MouseRipple.ini";
    }
    return L"MouseRipple.ini";
}

void loadConfig() {
    std::wstring ini = getIniPath();
    g_config.stylePreset = GetPrivateProfileIntW(L"Config", L"StylePreset", PRESET_WACOM, ini.c_str());
    g_config.leftColor = (COLORREF)GetPrivateProfileIntW(L"Config", L"LeftColor", RGB(232, 65, 82), ini.c_str());
    g_config.rightColor = (COLORREF)GetPrivateProfileIntW(L"Config", L"RightColor", RGB(41, 128, 245), ini.c_str());
    g_config.middleColor = (COLORREF)GetPrivateProfileIntW(L"Config", L"MiddleColor", RGB(245, 166, 35), ini.c_str());
    g_config.maxRadius = GetPrivateProfileIntW(L"Config", L"MaxRadius", 28, ini.c_str());
    g_config.durationMs = GetPrivateProfileIntW(L"Config", L"DurationMs", 300, ini.c_str());
    g_config.ringThickness = GetPrivateProfileIntW(L"Config", L"RingThickness", 2, ini.c_str());
    g_config.ringCount = GetPrivateProfileIntW(L"Config", L"RingCount", 1, ini.c_str());
    g_config.centerDot = GetPrivateProfileIntW(L"Config", L"CenterDot", 1, ini.c_str()) != 0;
    g_config.maxAlpha = GetPrivateProfileIntW(L"Config", L"MaxAlpha", 180, ini.c_str());
    g_config.clickEffect = GetPrivateProfileIntW(L"Config", L"ClickEffect", 0, ini.c_str());
    if (g_config.clickEffect < 0 || g_config.clickEffect > 23) g_config.clickEffect = 0;
    g_config.trailStyle = GetPrivateProfileIntW(L"Config", L"TrailStyle", 0, ini.c_str());
    if (g_config.trailStyle < 0 || g_config.trailStyle > 11) g_config.trailStyle = 0;

    // Ambient ripple configuration (纯净线条圈, 0-100% 透明度)
    g_config.ambientRipple = GetPrivateProfileIntW(L"Config", L"AmbientRipple", 1, ini.c_str()) != 0;
    g_config.ambientRadius = GetPrivateProfileIntW(L"Config", L"AmbientRadius", 20, ini.c_str());
    g_config.ambientThickness = GetPrivateProfileIntW(L"Config", L"AmbientThickness", 0, ini.c_str());
    if (g_config.ambientThickness <= 0) {
        g_config.ambientThickness = (std::max)(1, (std::min)(6, (int)GetPrivateProfileIntW(L"Config", L"AmbientBand", 2, ini.c_str())));
    }
    g_config.ambientBand = g_config.ambientThickness;
    g_config.ambientAlphaPercent = (std::max)(0, (std::min)(100, (int)GetPrivateProfileIntW(L"Config", L"AmbientAlphaPercent", 30, ini.c_str())));
    g_config.ambientAlpha = static_cast<int>(255.0f * (g_config.ambientAlphaPercent / 100.0f));
    g_config.ambientColor = (COLORREF)GetPrivateProfileIntW(L"Config", L"AmbientColor", RGB(145, 145, 145), ini.c_str());

    // Ink Trail configuration (水墨流线拖尾)
    g_config.trailEnabled = GetPrivateProfileIntW(L"Config", L"TrailEnabled", 1, ini.c_str()) != 0;
    g_config.trailColor = (COLORREF)GetPrivateProfileIntW(L"Config", L"TrailColor", RGB(41, 128, 245), ini.c_str());
    g_config.trailDurationMs = GetPrivateProfileIntW(L"Config", L"TrailDurationMs", 380, ini.c_str());
    g_config.trailWidth = GetPrivateProfileIntW(L"Config", L"TrailWidth", 8, ini.c_str());
    g_config.trailAlphaPercent = (std::max)(10, (std::min)(100, (int)GetPrivateProfileIntW(L"Config", L"TrailAlphaPercent", 80, ini.c_str())));
    g_config.trailAlpha = static_cast<int>(255.0f * (g_config.trailAlphaPercent / 100.0f));

    // Teaching Annotations configuration (教学演示标注)
    g_config.annotationEnabled = GetPrivateProfileIntW(L"Config", L"AnnotationEnabled", 1, ini.c_str()) != 0;
    g_config.inkColor = (COLORREF)GetPrivateProfileIntW(L"Config", L"InkColor", RGB(255, 68, 68), ini.c_str());
    g_config.inkWidth = (std::max)(3, (std::min)(16, (int)GetPrivateProfileIntW(L"Config", L"InkWidth", 6, ini.c_str())));
    g_config.arrowColor = (COLORREF)GetPrivateProfileIntW(L"Config", L"ArrowColor", RGB(255, 140, 0), ini.c_str());
    g_config.arrowWidth = (std::max)(3, (std::min)(16, (int)GetPrivateProfileIntW(L"Config", L"ArrowWidth", 6, ini.c_str())));
    g_config.annotationHoldMs = (std::max)(500, (std::min)(6000, (int)GetPrivateProfileIntW(L"Config", L"AnnotationHoldMs", 2200, ini.c_str())));
}

void saveConfig() {
    std::wstring ini = getIniPath();
    WritePrivateProfileStringW(L"Config", L"StylePreset", std::to_wstring(g_config.stylePreset).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"LeftColor", std::to_wstring(g_config.leftColor).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"RightColor", std::to_wstring(g_config.rightColor).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"MiddleColor", std::to_wstring(g_config.middleColor).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"MaxRadius", std::to_wstring(g_config.maxRadius).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"DurationMs", std::to_wstring(g_config.durationMs).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"RingThickness", std::to_wstring(g_config.ringThickness).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"RingCount", std::to_wstring(g_config.ringCount).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"CenterDot", g_config.centerDot ? L"1" : L"0", ini.c_str());
    WritePrivateProfileStringW(L"Config", L"MaxAlpha", std::to_wstring(g_config.maxAlpha).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"ClickEffect", std::to_wstring(g_config.clickEffect).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"TrailStyle", std::to_wstring(g_config.trailStyle).c_str(), ini.c_str());

    // Ambient ripple save
    WritePrivateProfileStringW(L"Config", L"AmbientRipple", g_config.ambientRipple ? L"1" : L"0", ini.c_str());
    WritePrivateProfileStringW(L"Config", L"AmbientRadius", std::to_wstring(g_config.ambientRadius).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"AmbientThickness", std::to_wstring(g_config.ambientThickness).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"AmbientBand", std::to_wstring(g_config.ambientThickness).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"AmbientAlphaPercent", std::to_wstring(g_config.ambientAlphaPercent).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"AmbientColor", std::to_wstring(g_config.ambientColor).c_str(), ini.c_str());

    // Ink Trail save
    WritePrivateProfileStringW(L"Config", L"TrailEnabled", g_config.trailEnabled ? L"1" : L"0", ini.c_str());
    WritePrivateProfileStringW(L"Config", L"TrailColor", std::to_wstring(g_config.trailColor).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"TrailDurationMs", std::to_wstring(g_config.trailDurationMs).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"TrailWidth", std::to_wstring(g_config.trailWidth).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"TrailAlphaPercent", std::to_wstring(g_config.trailAlphaPercent).c_str(), ini.c_str());

    // Teaching Annotations save
    WritePrivateProfileStringW(L"Config", L"AnnotationEnabled", g_config.annotationEnabled ? L"1" : L"0", ini.c_str());
    WritePrivateProfileStringW(L"Config", L"InkColor", std::to_wstring(g_config.inkColor).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"InkWidth", std::to_wstring(g_config.inkWidth).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"ArrowColor", std::to_wstring(g_config.arrowColor).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"ArrowWidth", std::to_wstring(g_config.arrowWidth).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Config", L"AnnotationHoldMs", std::to_wstring(g_config.annotationHoldMs).c_str(), ini.c_str());
}

// ------------------------------------------------------------------
// 更多预设主题表：每个主题一行数据，想再加新预设，只需照着复制一行即可。
// 字段顺序见下方 ThemePreset 结构体。颜色用 RGB(红, 绿, 蓝)，范围 0~255。
// ------------------------------------------------------------------
struct ThemePreset {
    const wchar_t* name;                     // 下拉框里显示的名字
    int radius;                              // 点击波纹最大半径 15~90
    int duration;                            // 点击波纹时长 150~750 ms
    int thickness;                           // 圈线粗细 1~5
    int rings;                               // 圈数 1~3
    int alpha;                               // 点击波纹不透明度 0~255
    bool centerDot;                          // 中心触点闪光
    COLORREF left, right, middle;            // 左键 / 右键 / 中键 颜色
    bool ambient;                            // 常驻呼吸圈开关
    int ambRadius;                           // 常驻圈半径 10~45
    int ambThickness;                        // 常驻圈粗细 1~6
    int ambPercent;                          // 常驻圈透明度 0~100 %
    COLORREF ambColor;                       // 常驻圈颜色
    bool trail;                              // 拖尾开关
    COLORREF trailColor;                     // 拖尾颜色
    int trailDuration;                       // 拖尾留存 150~800 ms
    int trailWidth;                          // 拖尾粗细 3~18 px
    int trailPercent;                        // 拖尾透明度 10~100 %
    int effect;                              // 点击特效类型 (0=水波纹，1~23 见 enum ClickEffect)
                                             // 特效类主题里：“圈数”滑块 = 数量档位，“半径”= 特效大小
    int trailStyle;                          // 拖尾样式 (0=墨迹彩带，1~11 见 enum TrailStyle)；可省略 = 0
};

const ThemePreset kThemes[] = {
    { L"🌸 樱花粉 (柔和/淡雅/少女心)",
      40, 480, 2, 2, 170, true,  RGB(255,126,170), RGB(190,140,255), RGB(255,190,120),
      true, 24, 2, 35, RGB(255,170,200),
      true, RGB(255,120,170), 420, 9, 75 },

    { L"🔥 烈焰火花 (炽热/高对比)",
      46, 420, 3, 3, 210, true,  RGB(255,87,34),   RGB(255,193,7),   RGB(255,235,59),
      true, 22, 2, 35, RGB(255,120,50),
      true, RGB(255,94,20),   360, 11, 85 },

    { L"💜 霓虹赛博 (青紫洋红/科技感)",
      36, 380, 2, 2, 220, true,  RGB(0,229,255),   RGB(224,64,251),  RGB(118,255,3),
      true, 20, 2, 45, RGB(0,229,255),
      true, RGB(224,64,251),  400, 8, 85 },

    { L"🌿 森林薄荷 (清新/自然)",
      38, 500, 2, 2, 170, true,  RGB(46,204,113),  RGB(26,188,156),  RGB(241,196,15),
      true, 22, 2, 30, RGB(39,174,96),
      true, RGB(46,204,113),  420, 8, 75 },

    { L"🌅 日落晚霞 (暖橙/紫红)",
      48, 560, 3, 3, 185, true,  RGB(255,138,76),  RGB(171,71,188),  RGB(255,213,79),
      true, 26, 2, 30, RGB(240,98,146),
      true, RGB(255,112,67),  460, 10, 78 },

    { L"🧊 冰蓝晶莹 (清冷/通透)",
      44, 520, 2, 3, 175, true,  RGB(79,195,247),  RGB(100,120,255), RGB(38,198,218),
      true, 24, 2, 30, RGB(120,190,230),
      true, RGB(66,165,245),  440, 8, 75 },

    { L"⚡ 闪电极速 (超快/干脆利落)",
      24, 160, 2, 1, 230, true,  RGB(255,214,0),   RGB(0,200,255),   RGB(255,64,129),
      true, 16, 2, 25, RGB(255,214,0),
      true, RGB(255,214,0),   200, 6, 85 },

    { L"🎬 录屏教学 (大圈/醒目/观众看得清)",
      62, 650, 4, 2, 235, true,  RGB(255,59,48),   RGB(0,122,255),   RGB(255,204,0),
      true, 30, 3, 55, RGB(255,204,0),
      true, RGB(255,149,0),   500, 14, 90 },

    { L"⚪ 极简黑白 (克制/低干扰，无拖尾无常驻圈)",
      22, 240, 1, 1, 150, false, RGB(60,60,60),    RGB(110,110,110), RGB(160,160,160),
      false, 18, 1, 20, RGB(120,120,120),
      false, RGB(80,80,80),   300, 4, 50 },

    // ================= 以下是“点击特效”类主题（不再是水波纹）=================
    { L"✨ 星星闪烁 (点一下群星闪现又消失)",
      44, 700, 2, 2, 240, false, RGB(255,196,40),  RGB(110,190,255), RGB(255,120,190),
      false, 18, 2, 25, RGB(255,196,40),
      true, RGB(255,196,40),  350, 6, 60, EFFECT_STARS },

    { L"⚡ 闪电盘旋 (落雷+电弧环绕)",
      54, 650, 2, 2, 245, false, RGB(70,170,255),  RGB(170,100,255), RGB(255,200,40),
      false, 18, 2, 25, RGB(90,180,255),
      true, RGB(90,180,255),  300, 5, 55, EFFECT_LIGHTNING },

    { L"🌸 樱花飘落 (花瓣四散旋转飘落)",
      58, 1100, 2, 2, 240, false, RGB(255,140,180), RGB(255,190,205), RGB(250,110,150),
      false, 18, 2, 25, RGB(255,150,185),
      true, RGB(255,150,185), 380, 6, 55, EFFECT_PETALS },

    { L"🌺 缤纷花瓣 (多彩花瓣绽放)",
      58, 1100, 2, 3, 240, false, RGB(255,105,150), RGB(150,110,255), RGB(255,170,50),
      false, 18, 2, 25, RGB(255,130,170),
      true, RGB(255,130,170), 380, 6, 55, EFFECT_PETALS },

    { L"🖌️ 水墨闪现 (墨点泼溅后晕散)",
      50, 850, 2, 2, 235, false, RGB(26,26,32),    RGB(36,66,128),   RGB(190,40,45),
      false, 18, 2, 25, RGB(60,60,70),
      true, RGB(40,40,48),    420, 8, 55, EFFECT_INK },

    { L"🎆 烟花绽放 (火花四射下坠)",
      64, 850, 2, 2, 240, false, RGB(255,140,30),  RGB(255,60,90),   RGB(60,200,255),
      false, 18, 2, 25, RGB(255,160,60),
      true, RGB(255,160,60),  300, 5, 55, EFFECT_FIREWORK },

    { L"🫧 泡泡升空 (透明泡泡飘起破裂)",
      52, 1200, 2, 2, 235, false, RGB(80,185,245), RGB(130,215,200), RGB(240,150,220),
      false, 18, 2, 25, RGB(120,200,240),
      false, RGB(90,190,240), 300, 5, 50, EFFECT_BUBBLES },

    { L"💖 爱心飘升 (爱心弹出飘起)",
      54, 1100, 2, 2, 240, false, RGB(255,75,120), RGB(255,130,170), RGB(190,90,240),
      false, 18, 2, 25, RGB(255,110,150),
      true, RGB(255,110,150), 350, 5, 55, EFFECT_HEARTS },

    { L"❄️ 雪花飘落 (六角雪花旋转飘落)",
      56, 1300, 2, 2, 240, false, RGB(120,190,255), RGB(170,150,255), RGB(100,220,230),
      false, 18, 2, 25, RGB(140,200,255),
      true, RGB(140,200,255), 350, 5, 55, EFFECT_SNOW },

    { L"🍁 落叶纷飞 (秋叶摇摆飘落)",
      58, 1300, 2, 2, 240, false, RGB(224,96,28),  RGB(214,160,30),  RGB(168,44,36),
      false, 18, 2, 25, RGB(224,120,40),
      true, RGB(224,120,40),  400, 6, 55, EFFECT_LEAVES },

    { L"🐾 猫爪印 (一串小爪印依次踩出)",
      52, 1100, 2, 2, 240, false, RGB(255,140,160), RGB(120,84,70),  RGB(70,70,80),
      false, 18, 2, 25, RGB(255,150,170),
      false, RGB(255,150,170), 300, 5, 50, EFFECT_PAWS },

    { L"💰 金币飞溅 (金币上抛翻转落下)",
      60, 1000, 2, 2, 240, false, RGB(255,200,40), RGB(190,200,215), RGB(210,120,60),
      false, 18, 2, 25, RGB(255,200,40),
      true, RGB(255,200,40),  300, 5, 55, EFFECT_COINS },

    { L"🎉 彩带礼花 (彩色纸片喷射飘落)",
      66, 1200, 2, 2, 240, false, RGB(255,80,120), RGB(60,160,255), RGB(255,200,40),
      false, 18, 2, 25, RGB(255,120,150),
      true, RGB(255,120,150), 300, 5, 55, EFFECT_CONFETTI },

    { L"☄️ 流星坠落 (流星划过撞出星光)",
      58, 800, 2, 2, 240, false, RGB(255,200,90),  RGB(120,190,255), RGB(255,130,210),
      false, 18, 2, 25, RGB(255,210,120),
      false, RGB(255,210,120), 300, 5, 50, EFFECT_METEOR },

    { L"🦋 蝴蝶翩翩 (蝴蝶扇动翅膀飞走)",
      56, 1500, 2, 2, 240, false, RGB(80,160,255), RGB(255,140,200), RGB(255,190,60),
      false, 18, 2, 25, RGB(120,180,255),
      false, RGB(120,180,255), 300, 5, 50, EFFECT_BUTTERFLY },

    { L"🌟 萤火虫 (荧光点点飘散闪烁)",
      56, 1400, 2, 2, 240, false, RGB(150,225,50), RGB(255,225,90), RGB(90,235,190),
      false, 18, 2, 25, RGB(170,235,80),
      true, RGB(170,235,80),  300, 5, 45, EFFECT_FIREFLY },

    { L"🎵 音符飘升 (音符弹出飘起)",
      54, 1200, 2, 2, 240, false, RGB(60,130,255), RGB(255,90,150), RGB(40,200,160),
      false, 18, 2, 25, RGB(100,150,255),
      true, RGB(100,150,255), 300, 5, 50, EFFECT_NOTES },
    { L"💫 星环爆裂 (光环扩散+十字星芒)",
      56, 800, 2, 2, 240, false, RGB(255,215,90), RGB(120,200,255), RGB(255,140,220),
      false, 18, 2, 25, RGB(255,215,90),
      false, RGB(255,215,90), 300, 5, 50, EFFECT_STARBURST, TRAIL_RIBBON },
    { L"☔ 雨滴溅起 (雨点砸落溅出水花)",
      54, 1000, 2, 2, 240, false, RGB(90,160,235), RGB(120,130,240), RGB(70,190,200),
      false, 18, 2, 25, RGB(90,160,235),
      false, RGB(90,160,235), 300, 5, 50, EFFECT_RAIN, TRAIL_RIBBON },
    { L"🔮 魔法阵 (旋转符文法阵浮现)",
      60, 1200, 2, 2, 240, false, RGB(170,110,255), RGB(255,110,200), RGB(80,210,255),
      false, 18, 2, 25, RGB(170,110,255),
      true, RGB(170,110,255), 350, 6, 55, EFFECT_MAGIC, TRAIL_RIBBON },
    { L"💥 漫画爆炸 (爆炸尖角啪地弹出)",
      52, 800, 2, 2, 240, false, RGB(255,205,40), RGB(255,90,60), RGB(70,200,255),
      false, 18, 2, 25, RGB(255,205,40),
      false, RGB(255,205,40), 300, 5, 50, EFFECT_COMIC, TRAIL_RIBBON },
    { L"🎈 气球升空 (彩色气球摇摆升起)",
      56, 1500, 2, 2, 240, false, RGB(255,80,100), RGB(80,160,255), RGB(255,200,50),
      false, 18, 2, 25, RGB(255,120,140),
      false, RGB(255,120,140), 300, 5, 50, EFFECT_BALLOONS, TRAIL_RIBBON },
    { L"🌈 彩虹光环 + 彩虹拖尾",
      60, 900, 2, 2, 240, false, RGB(255,90,90), RGB(90,160,255), RGB(255,200,50),
      false, 18, 2, 25, RGB(255,120,120),
      true, RGB(255,120,120), 600, 8, 85, EFFECT_RAINBOW, TRAIL_RAINBOW },
    { L"🔥 烈焰升腾 + 火星跟随",
      54, 1000, 2, 2, 240, false, RGB(255,110,20), RGB(255,60,40), RGB(255,190,40),
      false, 18, 2, 25, RGB(255,130,30),
      true, RGB(255,130,30), 800, 9, 90, EFFECT_FLAMES, TRAIL_EMBER },
    { L"✨ 星星闪烁 + 星屑跟随",
      44, 700, 2, 2, 240, false, RGB(255,200,60), RGB(110,190,255), RGB(255,120,190),
      false, 18, 2, 25, RGB(255,205,70),
      true, RGB(255,205,70), 700, 9, 90, EFFECT_STARS, TRAIL_SPARKLE },
    { L"🌸 樱花飘落 + 花瓣跟随",
      58, 1100, 2, 2, 240, false, RGB(255,140,180), RGB(255,190,205), RGB(250,110,150),
      false, 18, 2, 25, RGB(255,150,185),
      true, RGB(255,150,185), 900, 9, 90, EFFECT_PETALS, TRAIL_PETAL },
    { L"❄️ 雪花飘落 + 雪花跟随",
      56, 1300, 2, 2, 240, false, RGB(120,190,255), RGB(170,150,255), RGB(100,220,230),
      false, 18, 2, 25, RGB(140,205,255),
      true, RGB(140,205,255), 900, 9, 90, EFFECT_SNOW, TRAIL_SNOW },
    { L"💖 爱心飘升 + 爱心跟随",
      54, 1100, 2, 2, 240, false, RGB(255,75,120), RGB(255,130,170), RGB(190,90,240),
      false, 18, 2, 25, RGB(255,95,135),
      true, RGB(255,95,135), 800, 9, 90, EFFECT_HEARTS, TRAIL_HEART },
    { L"🫧 泡泡升空 + 泡泡跟随",
      52, 1200, 2, 2, 240, false, RGB(80,185,245), RGB(130,215,200), RGB(240,150,220),
      false, 18, 2, 25, RGB(100,195,245),
      true, RGB(100,195,245), 900, 9, 90, EFFECT_BUBBLES, TRAIL_BUBBLE },
    { L"🌟 萤火虫 + 萤火跟随",
      56, 1400, 2, 2, 240, false, RGB(150,225,50), RGB(255,225,90), RGB(90,235,190),
      false, 18, 2, 25, RGB(170,235,80),
      true, RGB(170,235,80), 1000, 9, 90, EFFECT_FIREFLY, TRAIL_FIREFLY },
    { L"🎵 音符飘升 + 音符跟随",
      54, 1200, 2, 2, 240, false, RGB(60,130,255), RGB(255,90,150), RGB(40,200,160),
      false, 18, 2, 25, RGB(90,140,255),
      true, RGB(90,140,255), 900, 9, 90, EFFECT_NOTES, TRAIL_NOTE },
    { L"🍁 落叶纷飞 + 落叶跟随",
      58, 1300, 2, 2, 240, false, RGB(224,96,28), RGB(214,160,30), RGB(168,44,36),
      false, 18, 2, 25, RGB(224,110,35),
      true, RGB(224,110,35), 1000, 9, 90, EFFECT_LEAVES, TRAIL_LEAF },
    { L"🎊 彩带礼花 + 彩屑跟随",
      66, 1200, 2, 2, 240, false, RGB(255,80,120), RGB(60,160,255), RGB(255,200,40),
      false, 18, 2, 25, RGB(255,100,140),
      true, RGB(255,100,140), 900, 9, 90, EFFECT_CONFETTI, TRAIL_GLITTER },
};

constexpr int kThemeCount = static_cast<int>(sizeof(kThemes) / sizeof(kThemes[0]));
constexpr int PRESET_THEME_FIRST = 3;        // 旧的 0/1/2 保持不变，新主题从 3 开始，老用户的 ini 不受影响

// 下拉框顺序：Wacom、水波涟漪、各新主题……、最后是“自定义”
int comboIndexToPreset(int idx) {
    if (idx == 0) return PRESET_WACOM;
    if (idx == 1) return PRESET_WATER_RIPPLE;
    if (idx >= 2 && idx < 2 + kThemeCount) return PRESET_THEME_FIRST + (idx - 2);
    return PRESET_CUSTOM;
}

int presetToComboIndex(int preset) {
    if (preset == PRESET_WACOM) return 0;
    if (preset == PRESET_WATER_RIPPLE) return 1;
    if (preset >= PRESET_THEME_FIRST && preset < PRESET_THEME_FIRST + kThemeCount)
        return 2 + (preset - PRESET_THEME_FIRST);
    return 2 + kThemeCount;                  // 自定义排最后
}

void applyPreset(int preset) {
    g_config.stylePreset = preset;
    g_config.clickEffect = EFFECT_RIPPLE;   // 默认水波纹；下面的新特效主题会改写它
    g_config.trailStyle = TRAIL_RIBBON;     // 默认墨迹彩带；下面的新主题会改写它
    if (preset == PRESET_WACOM) {
        g_config.maxRadius = 28;
        g_config.durationMs = 300;
        g_config.ringThickness = 2;
        g_config.ringCount = 1;
        g_config.centerDot = true;
        g_config.maxAlpha = 180;
        g_config.ambientRipple = true;
        g_config.ambientRadius = 20;
        g_config.ambientThickness = 2;
        g_config.ambientBand = 2;
        g_config.ambientAlphaPercent = 30;
        g_config.ambientAlpha = 76;
        g_config.ambientColor = RGB(145, 145, 145);
        g_config.trailEnabled = true;
        g_config.trailColor = RGB(41, 128, 245);
        g_config.trailDurationMs = 380;
        g_config.trailWidth = 8;
        g_config.trailAlphaPercent = 80;
        g_config.trailAlpha = 204;
    } else if (preset == PRESET_WATER_RIPPLE) {
        g_config.maxRadius = 52;
        g_config.durationMs = 540;
        g_config.ringThickness = 3;
        g_config.ringCount = 3;
        g_config.centerDot = true;
        g_config.maxAlpha = 190;
        g_config.trailEnabled = true;
        g_config.trailColor = RGB(0, 160, 233);
        g_config.trailDurationMs = 450;
        g_config.trailWidth = 10;
        g_config.trailAlphaPercent = 85;
        g_config.trailAlpha = 216;
    } else if (preset >= PRESET_THEME_FIRST && preset < PRESET_THEME_FIRST + kThemeCount) {
        const ThemePreset& t = kThemes[preset - PRESET_THEME_FIRST];
        g_config.maxRadius = t.radius;
        g_config.durationMs = t.duration;
        g_config.ringThickness = t.thickness;
        g_config.ringCount = t.rings;
        g_config.centerDot = t.centerDot;
        g_config.maxAlpha = t.alpha;
        g_config.leftColor = t.left;
        g_config.rightColor = t.right;
        g_config.middleColor = t.middle;
        g_config.ambientRipple = t.ambient;
        g_config.ambientRadius = t.ambRadius;
        g_config.ambientThickness = t.ambThickness;
        g_config.ambientBand = t.ambThickness;
        g_config.ambientAlphaPercent = t.ambPercent;
        g_config.ambientAlpha = static_cast<int>(255.0f * (t.ambPercent / 100.0f));
        g_config.ambientColor = t.ambColor;
        g_config.trailEnabled = t.trail;
        g_config.trailColor = t.trailColor;
        g_config.trailDurationMs = t.trailDuration;
        g_config.trailWidth = t.trailWidth;
        g_config.trailAlphaPercent = t.trailPercent;
        g_config.trailAlpha = static_cast<int>(255.0f * (t.trailPercent / 100.0f));
        g_config.clickEffect = t.effect;
        g_config.trailStyle = t.trailStyle;
    }
}

// Registry Autostart Helper
constexpr const wchar_t* kAutoStartKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr const wchar_t* kAutoStartValue = L"MouseRipple";

bool isAutoStartEnabled() {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kAutoStartKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type = 0;
        DWORD size = 0;
        LONG res = RegQueryValueExW(hKey, kAutoStartValue, nullptr, &type, nullptr, &size);
        RegCloseKey(hKey);
        return (res == ERROR_SUCCESS && size > 0);
    }
    return false;
}

void setAutoStart(bool enable) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kAutoStartKey, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH] = {0};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            std::wstring quoted = L"\"" + std::wstring(exePath) + L"\" --autostart";
            RegSetValueExW(hKey, kAutoStartValue, 0, REG_SZ,
                           reinterpret_cast<const BYTE*>(quoted.c_str()),
                           static_cast<DWORD>((quoted.size() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, kAutoStartValue);
        }
        RegCloseKey(hKey);
    }
}

void updateVirtualDesktopMetrics() {
    g_virtualX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    g_virtualY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    g_virtualW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    g_virtualH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
}

// -------------------------------------------------------------
// Soft Toroidal Wave Profile (基于余弦窗采样插值的超柔和水波，晶莹通透，零硬边，零摩尔纹)
// -------------------------------------------------------------
void drawSoftToroidalWave(Graphics& g, float cx, float cy, float rCrest, float halfBand,
                          float peakAlpha, BYTE r, BYTE gg, BYTE b) {
    if (rCrest <= 0.5f || peakAlpha <= 1.0f || halfBand <= 1.0f) return;

    const float rOuter = rCrest + halfBand;
    const float rInner = (std::max)(0.0f, rCrest - halfBand);
    if (rOuter <= 2.0f) return;

    GraphicsPath path;
    path.AddEllipse(cx - rOuter, cy - rOuter, rOuter * 2.0f, rOuter * 2.0f);
    PathGradientBrush pgb(&path);
    pgb.SetCenterPoint(PointF(cx, cy));

    float posCrest = clamp01(1.0f - (rCrest / rOuter));
    float posInner = clamp01(1.0f - (rInner / rOuter));

    constexpr int kStops = 11;
    REAL positions[kStops];
    Color colors[kStops];

    // 从外边缘 pos=0 (r=rOuter, 透明度=0) 到 波峰 posCrest (r=rCrest, 透明度=peakAlpha)
    for (int i = 0; i <= 5; i++) {
        float t = static_cast<float>(i) / 5.0f;
        positions[i] = posCrest * t;
        float factor = 0.5f * (1.0f - std::cos(3.14159265f * t));
        colors[i] = Color(alphaByte(peakAlpha * factor), r, gg, b);
    }
    // 从波峰 posCrest 到 内边缘 posInner (r=rInner, 透明度=0)
    for (int i = 1; i <= 4; i++) {
        float t = static_cast<float>(i) / 4.0f;
        positions[5 + i] = posCrest + (posInner - posCrest) * t;
        float factor = 0.5f * (1.0f + std::cos(3.14159265f * t));
        colors[5 + i] = Color(alphaByte(peakAlpha * factor), r, gg, b);
    }
    // 内边缘到中心 (全透明)
    positions[10] = 1.0f;
    colors[10] = Color(0, r, gg, b);

    // 单调性保证
    for (int i = 1; i < kStops; ++i) {
        if (positions[i] <= positions[i - 1]) positions[i] = positions[i - 1] + 0.0001f;
    }
    if (positions[kStops - 1] > 1.0f) positions[kStops - 1] = 1.0f;

    pgb.SetInterpolationColors(colors, positions, kStops);
    g.FillEllipse(&pgb, cx - rOuter, cy - rOuter, rOuter * 2.0f, rOuter * 2.0f);
}

// -------------------------------------------------------------
// Calligraphic Ink Ribbon Trail (水墨流线拖尾：向心样条 + 物理弧长锥度 + Alpha渐隐 + 转角防自交)
// -------------------------------------------------------------
void drawSmoothInkRibbon(Graphics& g, const std::vector<TrailPoint>& points, float totalDurationMs,
                         float baseWidth, int baseAlpha, COLORREF color, int vx, int vy, double nowMs) {
    const int nRaw = static_cast<int>(points.size());
    if (nRaw < 2) return;

    const BYTE r = GetRValue(color);
    const BYTE gg = GetGValue(color);
    const BYTE b = GetBValue(color);

    // 1. 提取样本并计算高精度寿命 (life: 0.0 ~ 1.0)
    struct SamplePt {
        float x, y;
        float life;
    };
    std::vector<SamplePt> rawPts;
    rawPts.reserve(nRaw);

    for (int i = 0; i < nRaw; ++i) {
        float elapsed = static_cast<float>(nowMs > points[i].timeMs ? nowMs - points[i].timeMs : 0.0);
        float life = clamp01(1.0f - elapsed / totalDurationMs);
        if (life <= 0.001f && i + 2 < nRaw) {
            continue; // 已完全过期的末尾历史点提前丢弃
        }
        SamplePt pt;
        pt.x = static_cast<float>(points[i].screenPt.x - vx);
        pt.y = static_cast<float>(points[i].screenPt.y - vy);
        pt.life = life;
        rawPts.push_back(pt);
    }

    const int n = static_cast<int>(rawPts.size());
    if (n < 2) return;

    // 2. 轨迹坐标轻度滤波（滤除鼠标硬件整数坐标产生的 1px 阶梯锯齿，同时保持笔头零延迟精准）
    std::vector<SamplePt> ctrlPts = rawPts;
    if (n >= 3) {
        for (int i = 1; i < n - 1; ++i) {
            ctrlPts[i].x = 0.2f * rawPts[i - 1].x + 0.6f * rawPts[i].x + 0.2f * rawPts[i + 1].x;
            ctrlPts[i].y = 0.2f * rawPts[i - 1].y + 0.6f * rawPts[i].y + 0.2f * rawPts[i + 1].y;
        }
    }

    // 3. 严格向心 Catmull-Rom 样条插值（Centripetal Spline: alpha = 0.5）
    // 数学上保证无局部自交环绕、无超调起伏、无波浪形凸起
    std::vector<SamplePt> spline;
    spline.reserve(n * 35);

    auto getKnot = [](const SamplePt& pA, const SamplePt& pB, float tPrev) -> float {
        float dx = pB.x - pA.x;
        float dy = pB.y - pA.y;
        float dist = std::hypot(dx, dy);
        return tPrev + std::pow((std::max)(dist, 0.001f), 0.5f); // alpha = 0.5 向心参数化
    };

    for (int i = 0; i < n - 1; ++i) {
        SamplePt p0, p1, p2, p3;
        p1 = ctrlPts[i];
        p2 = ctrlPts[i + 1];

        // 端点平滑自然镜像
        if (i == 0) {
            p0.x = p1.x - (p2.x - p1.x);
            p0.y = p1.y - (p2.y - p1.y);
            p0.life = clamp01(p1.life - (p2.life - p1.life));
        } else {
            p0 = ctrlPts[i - 1];
        }

        if (i + 2 < n) {
            p3 = ctrlPts[i + 2];
        } else {
            p3.x = p2.x + (p2.x - p1.x);
            p3.y = p2.y + (p2.y - p1.y);
            p3.life = clamp01(p2.life + (p2.life - p1.life));
        }

        float t0 = 0.0f;
        float t1 = getKnot(p0, p1, t0);
        float t2 = getKnot(p1, p2, t1);
        float t3 = getKnot(p2, p3, t2);

        float d10 = (std::max)(t1 - t0, 0.0001f);
        float d21 = (std::max)(t2 - t1, 0.0001f);
        float d32 = (std::max)(t3 - t2, 0.0001f);
        float d20 = (std::max)(t2 - t0, 0.0001f);
        float d31 = (std::max)(t3 - t1, 0.0001f);

        const float segDist = std::hypot(p2.x - p1.x, p2.y - p1.y);
        const int steps = (std::max)(3, (std::min)(35, static_cast<int>(std::ceil(segDist / 1.8f))));

        for (int s = 0; s < steps; ++s) {
            const float u = static_cast<float>(s) / static_cast<float>(steps);
            const float t = t1 + u * (t2 - t1);

            float a1_x = ((t1 - t) * p0.x + (t - t0) * p1.x) / d10;
            float a1_y = ((t1 - t) * p0.y + (t - t0) * p1.y) / d10;
            float a1_l = ((t1 - t) * p0.life + (t - t0) * p1.life) / d10;

            float a2_x = ((t2 - t) * p1.x + (t - t1) * p2.x) / d21;
            float a2_y = ((t2 - t) * p1.y + (t - t1) * p2.y) / d21;
            float a2_l = ((t2 - t) * p1.life + (t - t1) * p2.life) / d21;

            float a3_x = ((t3 - t) * p2.x + (t - t2) * p3.x) / d32;
            float a3_y = ((t3 - t) * p2.y + (t - t2) * p3.y) / d32;
            float a3_l = ((t3 - t) * p2.life + (t - t2) * p3.life) / d32;

            float b1_x = ((t2 - t) * a1_x + (t - t0) * a2_x) / d20;
            float b1_y = ((t2 - t) * a1_y + (t - t0) * a2_y) / d20;
            float b1_l = ((t2 - t) * a1_l + (t - t0) * a2_l) / d20;

            float b2_x = ((t3 - t) * a2_x + (t - t1) * a3_x) / d31;
            float b2_y = ((t3 - t) * a2_y + (t - t1) * a3_y) / d31;
            float b2_l = ((t3 - t) * a2_l + (t - t1) * a3_l) / d31;

            float cx = ((t2 - t) * b1_x + (t - t1) * b2_x) / d21;
            float cy = ((t2 - t) * b1_y + (t - t1) * b2_y) / d21;
            float cl = ((t2 - t) * b1_l + (t - t1) * b2_l) / d21;

            spline.push_back({cx, cy, clamp01(cl)});
        }
    }
    spline.push_back(ctrlPts.back());

    const int m = static_cast<int>(spline.size());
    if (m < 2) return;

    // 4. 计算沿轨迹物理累积弧长（Cumulative Arc Length）
    std::vector<float> arcLen(m, 0.0f);
    float totalLen = 0.0f;
    for (int i = 1; i < m; ++i) {
        float d = std::hypot(spline[i].x - spline[i - 1].x, spline[i].y - spline[i - 1].y);
        totalLen += d;
        arcLen[i] = totalLen;
    }
    if (totalLen < 1.0f) return;

    // 5. 计算切线方向与法线向量（带 3 点平滑，彻底消除法线微扰抖动）
    std::vector<PointF> tangents(m);
    for (int i = 0; i < m; ++i) {
        float tx = 0.0f, ty = 0.0f;
        if (i == 0) {
            tx = spline[1].x - spline[0].x;
            ty = spline[1].y - spline[0].y;
        } else if (i == m - 1) {
            tx = spline[m - 1].x - spline[m - 2].x;
            ty = spline[m - 1].y - spline[m - 2].y;
        } else {
            tx = spline[i + 1].x - spline[i - 1].x;
            ty = spline[i + 1].y - spline[i - 1].y;
        }
        float len = std::hypot(tx, ty);
        if (len > 0.0001f) {
            tangents[i] = PointF(tx / len, ty / len);
        } else {
            tangents[i] = (i > 0) ? tangents[i - 1] : PointF(1.0f, 0.0f);
        }
    }

    std::vector<PointF> rawNormals(m);
    for (int i = 0; i < m; ++i) {
        rawNormals[i] = PointF(-tangents[i].Y, tangents[i].X);
    }

    std::vector<PointF> normals = rawNormals;
    if (m >= 3) {
        for (int i = 1; i < m - 1; ++i) {
            float nx = 0.25f * rawNormals[i - 1].X + 0.5f * rawNormals[i].X + 0.25f * rawNormals[i + 1].X;
            float ny = 0.25f * rawNormals[i - 1].Y + 0.5f * rawNormals[i].Y + 0.25f * rawNormals[i + 1].Y;
            float len = std::hypot(nx, ny);
            if (len > 0.0001f) {
                normals[i] = PointF(nx / len, ny / len);
            }
        }
    }

    // 6. 物理弧长锥度与转角防自交折痕保护（生成左右边界）
    std::vector<PointF> leftEdge(m);
    std::vector<PointF> rightEdge(m);
    std::vector<float> segmentAlpha(m, 0.0f);

    for (int i = 0; i < m; ++i) {
        const float u = arcLen[i] / totalLen; // 0.0 (尾端) ~ 1.0 (笔尖)
        const float life = spline[i].life;

        // 弧长空间平滑过渡 (sin 缓动使笔尖饱满、尾梢纯数学平滑收窄)
        const float geomTaper = std::sin(u * 1.5707963f);
        float hw = (baseWidth * 0.5f) * geomTaper * std::sqrt(life);
        if (hw < 0.35f) hw = 0.35f;

        // 转角曲率自适应保护：锐角折返时平滑收敛线宽，杜绝内侧自交突起与外侧鸟嘴刺角
        if (i > 0 && i < m - 1) {
            float dot = tangents[i - 1].X * tangents[i + 1].X + tangents[i - 1].Y * tangents[i + 1].Y;
            if (dot < 0.7f) {
                float cornerScale = 0.45f + 0.55f * (0.5f * (dot + 1.0f));
                hw *= (std::max)(0.4f, cornerScale);
            }
        }

        leftEdge[i]  = PointF(spline[i].x + normals[i].X * hw, spline[i].y + normals[i].Y * hw);
        rightEdge[i] = PointF(spline[i].x - normals[i].X * hw, spline[i].y - normals[i].Y * hw);

        // Alpha 沿线平滑衰减：尾部归零羽化淡出，头部饱满
        float localA = static_cast<float>(baseAlpha) * std::pow(u, 1.25f) * std::pow(life, 0.8f);
        segmentAlpha[i] = localA;
    }

    // 7. 微元带状网格渲染（Micro-Quads Ribbon Strips，实现由实到虚的渐隐与无缝抗锯齿融合）
    for (int i = 0; i < m - 1; ++i) {
        float avgA = 0.5f * (segmentAlpha[i] + segmentAlpha[i + 1]);
        if (avgA < 1.0f) continue;

        BYTE a = alphaByte(avgA);
        SolidBrush brush(Color(a, r, gg, b));

        // 沿切线方向微重叠 0.6px，完全消除 GDI+ 邻接多边形亚像素缝隙
        const float overlap = 0.6f;
        const float ox = tangents[i + 1].X * overlap;
        const float oy = tangents[i + 1].Y * overlap;

        PointF quad[4] = {
            leftEdge[i],
            PointF(leftEdge[i + 1].X + ox, leftEdge[i + 1].Y + oy),
            PointF(rightEdge[i + 1].X + ox, rightEdge[i + 1].Y + oy),
            rightEdge[i]
        };

        g.FillPolygon(&brush, quad, 4);
    }

    // 8. 笔尖顺滑圆帽（Head Cap，消除 180° 反向翻转倒钩）
    const auto& headPt = spline.back();
    float headHw = (baseWidth * 0.5f) * std::sqrt(headPt.life);
    float headA = segmentAlpha.back();
    if (headHw > 0.5f && headA > 1.0f) {
        SolidBrush headBrush(Color(alphaByte(headA), r, gg, b));
        g.FillEllipse(&headBrush, headPt.x - headHw, headPt.y - headHw, headHw * 2.0f, headHw * 2.0f);
    }
}

// -------------------------------------------------------------
// Teaching Annotations Drawing (阅后即焚流光画笔与快捷箭头)
// -------------------------------------------------------------
void drawAnnotationInk(Graphics& g, const AnnotationStroke& stroke, double nowMs, int originX, int originY) {
    if (stroke.points.empty()) return;

    float alpha = 1.0f;
    if (!stroke.isDrawing) {
        const double elapsed = nowMs - stroke.releasedAt;
        if (elapsed < stroke.holdMs) {
            alpha = 1.0f;
        } else {
            const double fadeElapsed = elapsed - stroke.holdMs;
            if (fadeElapsed >= stroke.fadeMs) {
                return;
            }
            float t = static_cast<float>(fadeElapsed / stroke.fadeMs);
            alpha = 1.0f - std::pow(t, 1.4f);
        }
    }

    if (alpha <= 0.005f) return;

    const BYTE r = GetRValue(stroke.color);
    const BYTE gg = GetGValue(stroke.color);
    const BYTE b = GetBValue(stroke.color);

    std::vector<PointF> pts;
    pts.reserve(stroke.points.size());
    for (const auto& p : stroke.points) {
        pts.emplace_back(static_cast<float>(p.x - originX), static_cast<float>(p.y - originY));
    }

    if (pts.size() == 1) {
        float rad = stroke.width * 0.5f;
        SolidBrush brush(Color(alphaByte(alpha * 240.0f), r, gg, b));
        g.FillEllipse(&brush, pts[0].X - rad, pts[0].Y - rad, rad * 2.0f, rad * 2.0f);
        return;
    }

    // 3-point Gaussian smoothing to filter mouse coordinate quantization
    std::vector<PointF> smoothPts;
    smoothPts.reserve(pts.size());
    smoothPts.push_back(pts.front());
    for (size_t i = 1; i + 1 < pts.size(); ++i) {
        float sx = 0.25f * pts[i - 1].X + 0.5f * pts[i].X + 0.25f * pts[i + 1].X;
        float sy = 0.25f * pts[i - 1].Y + 0.5f * pts[i].Y + 0.25f * pts[i + 1].Y;
        smoothPts.emplace_back(sx, sy);
    }
    smoothPts.push_back(pts.back());

    GraphicsPath path;
    path.AddLines(smoothPts.data(), static_cast<INT>(smoothPts.size()));

    // 1. Soft dark drop shadow pass (ensures high contrast on any background)
    Pen shadowPen(Color(alphaByte(alpha * 85.0f), 0, 0, 0), stroke.width + 3.0f);
    shadowPen.SetStartCap(LineCapRound);
    shadowPen.SetEndCap(LineCapRound);
    shadowPen.SetLineJoin(LineJoinRound);
    g.DrawPath(&shadowPen, &path);

    // 2. Vibrant core ink pass
    Pen corePen(Color(alphaByte(alpha * 250.0f), r, gg, b), stroke.width);
    corePen.SetStartCap(LineCapRound);
    corePen.SetEndCap(LineCapRound);
    corePen.SetLineJoin(LineJoinRound);
    g.DrawPath(&corePen, &path);
}

void drawQuickArrow(Graphics& g, const AnnotationStroke& stroke, double nowMs, int originX, int originY) {
    float alpha = 1.0f;
    if (!stroke.isDrawing) {
        const double elapsed = nowMs - stroke.releasedAt;
        if (elapsed < stroke.holdMs) {
            alpha = 1.0f;
        } else {
            const double fadeElapsed = elapsed - stroke.holdMs;
            if (fadeElapsed >= stroke.fadeMs) {
                return;
            }
            float t = static_cast<float>(fadeElapsed / stroke.fadeMs);
            alpha = 1.0f - std::pow(t, 1.4f);
        }
    }

    if (alpha <= 0.005f) return;

    const float x1 = static_cast<float>(stroke.startPt.x - originX);
    const float y1 = static_cast<float>(stroke.startPt.y - originY);
    const float x2 = static_cast<float>(stroke.endPt.x - originX);
    const float y2 = static_cast<float>(stroke.endPt.y - originY);

    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float dist = std::hypot(dx, dy);
    if (dist < 4.0f) {
        const BYTE r = GetRValue(stroke.color);
        const BYTE gg = GetGValue(stroke.color);
        const BYTE b = GetBValue(stroke.color);
        SolidBrush brush(Color(alphaByte(alpha * 240.0f), r, gg, b));
        float rad = stroke.width * 0.6f;
        g.FillEllipse(&brush, x1 - rad, y1 - rad, rad * 2.0f, rad * 2.0f);
        return;
    }

    const float ux = dx / dist;
    const float uy = dy / dist;
    const float nx = -uy;
    const float ny = ux;

    float headLen = (std::min)(50.0f, (std::max)(22.0f, stroke.width * 4.0f));
    if (dist < headLen * 1.3f) {
        headLen = dist * 0.72f;
    }
    const float headWidth = headLen * 0.80f;

    PointF tip(x2, y2);
    PointF left(x2 - ux * headLen + nx * (headWidth * 0.5f),
                y2 - uy * headLen + ny * (headWidth * 0.5f));
    PointF right(x2 - ux * headLen - nx * (headWidth * 0.5f),
                 y2 - uy * headLen - ny * (headWidth * 0.5f));
    PointF notch(x2 - ux * (headLen * 0.72f),
                 y2 - uy * (headLen * 0.72f));

    PointF headPoly[4] = { tip, left, notch, right };

    const BYTE r = GetRValue(stroke.color);
    const BYTE gg = GetGValue(stroke.color);
    const BYTE b = GetBValue(stroke.color);

    // 1. Soft dark shadow pass
    Pen shadowShaft(Color(alphaByte(alpha * 85.0f), 0, 0, 0), stroke.width + 3.0f);
    shadowShaft.SetStartCap(LineCapRound);
    shadowShaft.SetEndCap(LineCapRound);
    g.DrawLine(&shadowShaft, x1, y1, notch.X, notch.Y);

    Pen shadowHeadBorder(Color(alphaByte(alpha * 85.0f), 0, 0, 0), 3.0f);
    shadowHeadBorder.SetLineJoin(LineJoinRound);
    SolidBrush shadowBrush(Color(alphaByte(alpha * 85.0f), 0, 0, 0));
    g.FillPolygon(&shadowBrush, headPoly, 4);
    g.DrawPolygon(&shadowHeadBorder, headPoly, 4);

    // 2. Vibrant core pass
    Pen coreShaft(Color(alphaByte(alpha * 250.0f), r, gg, b), stroke.width);
    coreShaft.SetStartCap(LineCapRound);
    coreShaft.SetEndCap(LineCapRound);
    g.DrawLine(&coreShaft, x1, y1, notch.X, notch.Y);

    SolidBrush coreBrush(Color(alphaByte(alpha * 250.0f), r, gg, b));
    g.FillPolygon(&coreBrush, headPoly, 4);
    Pen coreHeadBorder(Color(alphaByte(alpha * 250.0f), r, gg, b), 1.5f);
    coreHeadBorder.SetLineJoin(LineJoinRound);
    g.DrawPolygon(&coreHeadBorder, headPoly, 4);
}

// -------------------------------------------------------------
// Ambient Continuous Ripple Animation (常驻单圈纯净线条呼吸圈，无渐变过度)
// -------------------------------------------------------------
void drawAmbientRipple(Graphics& g, ULONGLONG now) {
    if (!g_config.ambientRipple || g_config.ambientAlpha <= 0) return;

    POINT pt{};
    if (!GetCursorPos(&pt)) return;

    const float cx = static_cast<float>(pt.x - g_virtualX);
    const float cy = static_cast<float>(pt.y - g_virtualY);

    const float maxR = static_cast<float>((std::max)(10, g_config.ambientRadius));
    const float maxAlpha = static_cast<float>(g_config.ambientAlpha); // 0 ~ 255 (0% - 100%)

    const BYTE r = GetRValue(g_config.ambientColor);
    const BYTE gg = GetGValue(g_config.ambientColor);
    const BYTE b = GetBValue(g_config.ambientColor);

    // 单圈从容呼吸循环 (1.8s，单圈舒缓，绝不密集喧闹)
    constexpr ULONGLONG kCycleMs = 1800;
    const float t = static_cast<float>(now % kCycleMs) / static_cast<float>(kCycleMs);

    // 舒缓缓动扩张
    const float e = 1.0f - std::pow(1.0f - t, 2.2f);
    const float startR = 3.5f;
    const float curR = startR + (maxR - startR) * e;

    // 钟形透明度淡入淡出 (前 18% 柔和淡入，随后舒缓淡出)
    float a = 0.0f;
    if (t < 0.18f) {
        a = maxAlpha * (t / 0.18f);
    } else {
        a = maxAlpha * std::pow(1.0f - (t - 0.18f) / 0.82f, 1.4f);
    }

    if (a > 1.0f) {
        const float thickness = static_cast<float>((std::max)(1, (std::min)(6, g_config.ambientThickness)));
        if (thickness <= 1.5f) {
            Pen pen(Color(alphaByte(a), r, gg, b), thickness);
            pen.SetAlignment(PenAlignmentCenter);
            g.DrawEllipse(&pen, cx - curR, cy - curR, curR * 2.0f, curR * 2.0f);
        } else {
            // 呼吸圈柔边羽化：核心圈 + 柔和晕光
            Pen penCore(Color(alphaByte(a * 0.75f), r, gg, b), thickness * 0.75f);
            penCore.SetAlignment(PenAlignmentCenter);
            g.DrawEllipse(&penCore, cx - curR, cy - curR, curR * 2.0f, curR * 2.0f);

            Pen penGlow(Color(alphaByte(a * 0.35f), r, gg, b), thickness * 1.5f);
            penGlow.SetAlignment(PenAlignmentCenter);
            g.DrawEllipse(&penGlow, cx - curR, cy - curR, curR * 2.0f, curR * 2.0f);
        }
    }
}

// =============================================================
// 点击特效合集：小星星 / 闪电盘旋 / 花瓣 / 水墨 / 烟花 / 泡泡 / 爱心
// 每个特效都是一个“纯函数”：给定动画进度 t (0~1)，画出这一帧，不保存任何状态。
// 参数含义：cx,cy=点击位置  t=进度  R=大小(对应“波纹半径”滑块)
//           A=不透明度(0~255)  col=颜色  seed=这一次点击的随机种子  density=数量档位(1~3)
// =============================================================
constexpr float kPi = 3.14159265f;

inline float frand(unsigned& s) {
    s = s * 1664525u + 1013904223u;
    return static_cast<float>((s >> 8) & 0xFFFF) / 65536.0f;
}

inline float lerpf(float a, float b, float k) { return a + (b - a) * k; }

// 安全幂函数：底数 <= 0 时当作 0，永远不会产生 NaN（NaN 会被当成“完全不透明”，造成结尾闪烁）
inline float powSafe(float x, float p) { return std::pow((x > 0.0f) ? x : 0.0f, p); }

inline float smoothstep01(float x) { x = clamp01(x); return x * x * (3.0f - 2.0f * x); }

inline float easeOutPow(float t, float p) { return 1.0f - powSafe(1.0f - clamp01(t), p); }

// 颜色工具：shade>0 向白色靠近(变亮)，shade<0 向黑色靠近(变暗)
Color makeColor(COLORREF c, float alpha, float shade = 0.0f) {
    float r = static_cast<float>(GetRValue(c));
    float gg = static_cast<float>(GetGValue(c));
    float b = static_cast<float>(GetBValue(c));
    if (shade >= 0.0f) {
        r += (255.0f - r) * shade;
        gg += (255.0f - gg) * shade;
        b += (255.0f - b) * shade;
    } else {
        const float k = 1.0f + shade;
        r *= k; gg *= k; b *= k;
    }
    return Color(alphaByte(alpha), alphaByte(r), alphaByte(gg), alphaByte(b));
}

// 柔和光晕：多层由大到小的半透明圆叠加，边缘自然羽化，没有硬边
void softGlow(Graphics& g, float x, float y, float r, COLORREF col, float alpha, float shade = 0.0f) {
    const float scale[7] = { 1.00f, 0.85f, 0.70f, 0.56f, 0.42f, 0.30f, 0.18f };
    for (int i = 0; i < 7; ++i) {
        const float rr = r * scale[i];
        SolidBrush b(makeColor(col, alpha * 0.17f, shade));
        g.FillEllipse(&b, x - rr, y - rr, rr * 2.0f, rr * 2.0f);
    }
}

void addSparklePath(GraphicsPath& path, float cx, float cy, float outer, float innerRatio, int points, float rot) {
    PointF pts[16];
    const int n = points * 2;
    for (int i = 0; i < n; ++i) {
        const float a = rot + static_cast<float>(i) * kPi / static_cast<float>(points);
        const float r = (i % 2 == 0) ? outer : outer * innerRatio;
        pts[i] = PointF(cx + std::cos(a) * r, cy + std::sin(a) * r);
    }
    path.AddPolygon(pts, n);
}

// ---------- 1. 小星星闪烁 ----------
void drawFxStars(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 6 + density * 3;
    const float sc = R / 40.0f;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float ang = frand(s) * 2.0f * kPi;
        const float dist = R * (0.35f + 1.05f * frand(s));
        const float delay = 0.22f * frand(s);
        const float size = (4.5f + 8.0f * frand(s)) * sc;
        const float rot0 = frand(s) * kPi;
        const float spin = (frand(s) - 0.5f) * kPi * 0.9f;
        const float whiteMix = 0.55f * frand(s);
        const bool fivePt = (i % 3 == 2);

        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float pulse = std::sin(u * kPi);
        const float twinkle = 0.80f + 0.20f * std::sin(u * kPi * 5.0f + static_cast<float>(i));
        const float e = easeOutPow(u, 2.2f);
        const float px = cx + std::cos(ang) * dist * e;
        const float py = cy + std::sin(ang) * dist * e - 8.0f * sc * u;
        const float sz = size * (0.25f + 0.75f * powSafe(pulse, 0.7f));
        const float a = A * powSafe(pulse, 0.6f) * twinkle;

        softGlow(g, px, py, sz * 1.9f, col, a * 0.55f, whiteMix);

        GraphicsPath path;
        if (fivePt) addSparklePath(path, px, py, sz * 0.85f, 0.45f, 5, rot0 + spin * u - kPi * 0.5f);
        else addSparklePath(path, px, py, sz * 1.15f, 0.24f, 4, rot0 + spin * u);
        SolidBrush body(makeColor(col, a, whiteMix));
        g.FillPath(&body, &path);
        Pen edge(makeColor(col, a * 0.65f, -0.30f), 0.9f);   // 细描边：浅色背景上也看得清
        edge.SetLineJoin(LineJoinRound);
        g.DrawPath(&edge, &path);

        const float cr = sz * 0.20f;
        SolidBrush core(Color(alphaByte(a * 0.9f), 255, 255, 255));
        g.FillEllipse(&core, px - cr, py - cr, cr * 2.0f, cr * 2.0f);
    }
    if (t < 0.18f) {
        const float k = 1.0f - t / 0.18f;
        const float fr = (4.0f + 8.0f * (1.0f - k)) * sc;
        softGlow(g, cx, cy, fr * 1.6f, col, A * 0.8f * k, 0.5f);
    }
}

// ---------- 2. 闪电盘旋 ----------
void drawBolt(Graphics& g, const PointF* pts, int n, float width, float A, COLORREF col) {
    if (n < 2 || A < 1.0f) return;
    Pen under(makeColor(col, A * 0.35f, -0.55f), width * 2.6f);   // 深色底边：浅色背景上托出轮廓
    under.SetLineJoin(LineJoinRound); under.SetStartCap(LineCapRound); under.SetEndCap(LineCapRound);
    g.DrawLines(&under, pts, n);
    Pen glow(makeColor(col, A * 0.22f), width * 4.2f);
    glow.SetLineJoin(LineJoinRound); glow.SetStartCap(LineCapRound); glow.SetEndCap(LineCapRound);
    g.DrawLines(&glow, pts, n);
    Pen mid(makeColor(col, A * 0.95f), width * 1.7f);
    mid.SetLineJoin(LineJoinRound); mid.SetStartCap(LineCapRound); mid.SetEndCap(LineCapRound);
    g.DrawLines(&mid, pts, n);
    Pen core(Color(alphaByte(A * 0.95f), 255, 255, 255), (std::max)(0.8f, width * 0.6f));
    core.SetLineJoin(LineJoinRound); core.SetStartCap(LineCapRound); core.SetEndCap(LineCapRound);
    g.DrawLines(&core, pts, n);
}

void drawFxLightning(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const float sc = R / 50.0f;
    unsigned base = seed;
    const float rotBase = frand(base) * 2.0f * kPi;
    const float strikeX = (frand(base) - 0.5f) * R * 0.5f;
    // 闪烁：每约 1/16 的生命周期换一次随机抖动，像真实电弧一样噼啪跳动
    unsigned fs = seed ^ (static_cast<unsigned>(t * 16.0f) * 2654435761u);
    const float flick = 0.65f + 0.35f * frand(fs);
    const float fade = powSafe(1.0f - t, 0.7f);
    const float a = A * fade * flick;

    // 1) 开场一道落雷，劈到点击位置
    if (t < 0.32f) {
        const float k = 1.0f - t / 0.32f;
        const int segs = 8;
        PointF pts[segs + 1];
        const float topX = cx + strikeX;
        const float topY = cy - R * 2.2f;
        for (int i = 0; i <= segs; ++i) {
            const float f = static_cast<float>(i) / segs;
            float x = lerpf(topX, cx, f);
            const float y = lerpf(topY, cy, f);
            if (i > 0 && i < segs) x += (frand(fs) - 0.5f) * R * 0.50f * (1.0f - f * 0.5f);
            pts[i] = PointF(x, y);
        }
        drawBolt(g, pts, segs + 1, 2.4f * sc, A * k, col);
    }

    // 2) 围绕点击位置盘旋的电弧
    const int arcs = 2 + density;
    for (int k = 0; k < arcs; ++k) {
        const float rot = rotBase + static_cast<float>(k) * 2.0f * kPi / arcs + t * 2.6f * kPi;
        const float rad = R * (0.30f + 0.70f * easeOutPow(t, 1.8f));
        const float span = 0.9f + 0.5f * frand(fs);
        const int M = 9;
        PointF pts[M];
        for (int i = 0; i < M; ++i) {
            const float f = static_cast<float>(i) / (M - 1);
            const float th = rot + span * f;
            const float rr = rad + (frand(fs) - 0.5f) * R * 0.18f;
            pts[i] = PointF(cx + std::cos(th) * rr, cy + std::sin(th) * rr);
        }
        drawBolt(g, pts, M, 1.9f * sc, a, col);
        // 一根小分叉
        const float bAng = rot + span * 0.5f;
        const PointF from = pts[M / 2];
        const float blen = R * (0.16f + 0.14f * frand(fs));
        const float dir = bAng + (frand(fs) > 0.5f ? 0.9f : -0.9f) + 1.5708f;
        PointF br[3] = { from,
                         PointF(from.X + std::cos(dir) * blen * 0.5f + (frand(fs) - 0.5f) * 3.0f * sc,
                                from.Y + std::sin(dir) * blen * 0.5f),
                         PointF(from.X + std::cos(dir) * blen, from.Y + std::sin(dir) * blen) };
        drawBolt(g, br, 3, 1.3f * sc, a * 0.85f, col);
    }

    // 3) 中心闪光
    if (t < 0.25f) {
        const float k = 1.0f - t / 0.25f;
        const float fr = R * (0.18f + 0.22f * (1.0f - k));
        softGlow(g, cx, cy, fr * 1.8f, col, A * 0.9f * k, 0.5f);
    }
}

// ---------- 3. 花瓣闪现 / 飘落 ----------
void drawFxPetals(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 7 + density * 3;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float ang = frand(s) * 2.0f * kPi;
        const float speed = R * (0.8f + 0.9f * frand(s));
        const float delay = 0.08f * frand(s);
        const float L = R * (0.17f + 0.12f * frand(s));
        const float phase = frand(s) * 2.0f * kPi;
        const float spin = (frand(s) - 0.5f) * 2.0f * kPi * 2.2f;
        const float rot0 = frand(s) * 2.0f * kPi;
        const float shade = 0.5f * frand(s) - 0.15f;

        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float e = easeOutPow(u, 2.4f);
        const float px = cx + std::cos(ang) * speed * e + std::sin(u * kPi * 2.5f + phase) * R * 0.12f * u;
        const float py = cy + std::sin(ang) * speed * e * 0.8f + R * 1.0f * u * u;
        const float rot = rot0 + spin * u;
        const float flip = 0.30f + 0.70f * std::fabs(std::cos(u * kPi * 3.2f + phase));
        const float a = A * clamp01(u * 10.0f) * (1.0f - powSafe(u, 2.5f));

        const float w = L * 0.55f;
        GraphicsPath petal;
        petal.AddBezier(PointF(0, 0), PointF(-w, -L * 0.35f), PointF(-w * 0.7f, -L * 0.9f), PointF(0, -L));
        petal.AddBezier(PointF(0, -L), PointF(w * 0.7f, -L * 0.9f), PointF(w, -L * 0.35f), PointF(0, 0));
        petal.CloseFigure();

        Matrix m;
        m.Translate(px, py);
        m.Rotate(rot * 180.0f / kPi);
        m.Scale(flip, 1.0f);
        g.SetTransform(&m);
        SolidBrush body(makeColor(col, a, shade));
        g.FillPath(&body, &petal);
        Pen vein(makeColor(col, a * 0.45f, shade + 0.25f), 0.8f);
        g.DrawLine(&vein, PointF(0, -L * 0.08f), PointF(0, -L * 0.78f));
        g.ResetTransform();
    }
}

// ---------- 4. 水墨闪现 ----------
void addBlobPath(GraphicsPath& path, float cx, float cy, float r, unsigned seed, int n, float jitter) {
    std::vector<PointF> pts;
    pts.reserve(n);
    unsigned s = seed;
    for (int k = 0; k < n; ++k) {
        const float a = 2.0f * kPi * static_cast<float>(k) / static_cast<float>(n);
        const float rr = r * (1.0f - jitter + 2.0f * jitter * frand(s));
        pts.emplace_back(cx + std::cos(a) * rr, cy + std::sin(a) * rr);
    }
    path.AddClosedCurve(pts.data(), n, 0.5f);
}

void drawFxInk(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const float open = easeOutPow((std::min)(1.0f, t / 0.30f), 3.0f);
    const float fade = (t < 0.5f) ? 1.0f : powSafe(1.0f - (t - 0.5f) / 0.5f, 1.4f);
    const float a = A * fade;
    if (a < 1.0f) return;
    const float r0 = R * 0.40f * (0.20f + 0.80f * open) * (1.0f + 0.10f * t);

    // 晕染：由外到内三层叠加，边缘像宣纸上洇开的墨
    { GraphicsPath p; addBlobPath(p, cx, cy, r0 * 1.30f, seed + 11u, 14, 0.20f);
      SolidBrush b(makeColor(col, a * 0.10f)); g.FillPath(&b, &p); }
    { GraphicsPath p; addBlobPath(p, cx, cy, r0 * 1.15f, seed + 7u, 14, 0.20f);
      SolidBrush b(makeColor(col, a * 0.18f)); g.FillPath(&b, &p); }
    { GraphicsPath p; addBlobPath(p, cx, cy, r0, seed, 14, 0.22f);
      SolidBrush b(makeColor(col, a * 0.90f)); g.FillPath(&b, &p); }
    { GraphicsPath p; addBlobPath(p, cx, cy, r0 * 0.55f, seed + 3u, 10, 0.15f);
      SolidBrush b(makeColor(col, a * 0.45f, -0.35f)); g.FillPath(&b, &p); }

    unsigned s = seed ^ 0x9E3779B9u;
    // 飞溅的墨条：根部粗、尖端细
    const int ns = 6 + density * 2;
    for (int i = 0; i < ns; ++i) {
        const float ang = 2.0f * kPi * static_cast<float>(i) / ns + (frand(s) - 0.5f) * 0.5f;
        const float len = R * (0.45f + 0.65f * frand(s)) * open;
        const float w0 = R * (0.045f + 0.055f * frand(s));
        const float bd = r0 * 0.7f;
        const float ux = std::cos(ang), uy = std::sin(ang);
        const float nx = -uy, ny = ux;
        PointF poly[5] = {
            PointF(cx + ux * bd + nx * w0,            cy + uy * bd + ny * w0),
            PointF(cx + ux * (bd + len * 0.55f) + nx * w0 * 0.45f, cy + uy * (bd + len * 0.55f) + ny * w0 * 0.45f),
            PointF(cx + ux * (bd + len),              cy + uy * (bd + len)),
            PointF(cx + ux * (bd + len * 0.55f) - nx * w0 * 0.45f, cy + uy * (bd + len * 0.55f) - ny * w0 * 0.45f),
            PointF(cx + ux * bd - nx * w0,            cy + uy * bd - ny * w0)
        };
        SolidBrush b(makeColor(col, a * 0.90f));
        g.FillPolygon(&b, poly, 5);
        const float dr = w0 * (0.5f + 0.4f * frand(s));
        const float dd = bd + len + R * 0.10f * open;
        g.FillEllipse(&b, cx + ux * dd - dr, cy + uy * dd - dr, dr * 2.0f, dr * 2.0f);
    }
    // 远处的小墨点
    const int nd = 6 + density * 3;
    for (int i = 0; i < nd; ++i) {
        const float ang = frand(s) * 2.0f * kPi;
        const float d = R * (0.75f + 0.80f * frand(s)) * easeOutPow((std::min)(1.0f, t / 0.5f), 2.4f);
        const float rad = R * (0.022f + 0.045f * frand(s)) * (1.0f - 0.4f * t);
        SolidBrush b(makeColor(col, a * 0.85f));
        g.FillEllipse(&b, cx + std::cos(ang) * d - rad, cy + std::sin(ang) * d - rad, rad * 2.0f, rad * 2.0f);
    }
}

// ---------- 5. 烟花绽放 ----------
void drawFxFirework(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 10 + density * 4;
    const float sc = R / 50.0f;
    unsigned s = seed;
    const float grav = R * 0.55f * t * t;
    const float alpha = A * powSafe(1.0f - t, 1.3f);
    const float eHead = easeOutPow(t, 2.3f);
    const float eTail = easeOutPow((std::max)(0.0f, t - 0.18f), 2.3f);
    for (int i = 0; i < n; ++i) {
        const float ang = 2.0f * kPi * static_cast<float>(i) / n + (frand(s) - 0.5f) * 0.35f;
        const float reach = R * (0.55f + 0.55f * frand(s));
        const float mix = 0.5f * frand(s);
        const float pw = (2.0f + 2.2f * frand(s)) * sc * (1.0f - 0.45f * t);
        const float ux = std::cos(ang), uy = std::sin(ang);
        const float hx = cx + ux * reach * eHead, hy = cy + uy * reach * eHead + grav;
        const float tx = cx + ux * reach * eTail, ty = cy + uy * reach * eTail + grav;
        Pen halo(makeColor(col, alpha * 0.22f, mix), (std::max)(2.0f, pw * 2.8f));
        halo.SetStartCap(LineCapRound); halo.SetEndCap(LineCapRound);
        g.DrawLine(&halo, tx, ty, hx, hy);
        Pen pen(makeColor(col, alpha, mix), (std::max)(1.2f, pw));
        pen.SetStartCap(LineCapRound); pen.SetEndCap(LineCapRound);
        g.DrawLine(&pen, tx, ty, hx, hy);
        const float hr = 2.2f * sc * (1.0f - t);
        SolidBrush head(makeColor(col, alpha, 0.65f));
        g.FillEllipse(&head, hx - hr, hy - hr, hr * 2.0f, hr * 2.0f);
    }
    // 闪烁的碎光
    for (int i = 0; i < n; ++i) {
        const float ang = frand(s) * 2.0f * kPi;
        const float d = R * (0.35f + 0.75f * frand(s)) * eHead;
        const float ph = frand(s) * 2.0f * kPi;
        const float tw = 0.5f + 0.5f * std::sin(t * 22.0f + ph);
        const float rr = 1.8f * sc;
        SolidBrush b(makeColor(col, alpha * 0.9f * tw, 0.2f));
        g.FillEllipse(&b, cx + std::cos(ang) * d - rr, cy + std::sin(ang) * d + grav - rr, rr * 2.0f, rr * 2.0f);
    }
    if (t < 0.18f) {
        const float k = 1.0f - t / 0.18f;
        const float fr = (5.0f + 10.0f * (1.0f - k)) * sc;
        softGlow(g, cx, cy, fr * 1.8f, col, A * 0.9f * k, 0.4f);
    }
}

// ---------- 6. 泡泡升空 ----------
void drawFxBubbles(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 5 + density * 2;
    const float sc = R / 50.0f;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float x0 = (frand(s) - 0.5f) * R * 0.9f;
        const float y0 = (frand(s) - 0.5f) * R * 0.4f;
        const float rise = R * (1.2f + 1.0f * frand(s));
        const float delay = 0.2f * frand(s);
        const float rad0 = R * (0.10f + 0.16f * frand(s));
        const float phase = frand(s) * 2.0f * kPi;

        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float px = cx + x0 * (0.4f + 0.6f * easeOutPow(u, 2.0f)) + std::sin(u * kPi * 2.2f + phase) * R * 0.10f;
        const float py = cy + y0 - rise * easeOutPow(u, 1.6f);
        const float pop = (u > 0.82f) ? (u - 0.82f) / 0.18f : 0.0f;
        const float rad = rad0 * (0.5f + 0.5f * easeOutPow(u, 2.0f)) * (1.0f + 0.5f * pop);
        const float a = A * clamp01(u * 8.0f) * (1.0f - pop);

        SolidBrush fill(makeColor(col, a * 0.14f, 0.4f));
        g.FillEllipse(&fill, px - rad, py - rad, rad * 2.0f, rad * 2.0f);
        Pen rim(makeColor(col, a * 0.85f, 0.15f), (std::max)(1.0f, 1.3f * sc));
        g.DrawEllipse(&rim, px - rad, py - rad, rad * 2.0f, rad * 2.0f);
        SolidBrush hl(Color(alphaByte(a * 0.85f), 255, 255, 255));
        g.FillEllipse(&hl, px - rad * 0.55f, py - rad * 0.58f, rad * 0.38f, rad * 0.24f);
        SolidBrush hl2(Color(alphaByte(a * 0.45f), 255, 255, 255));
        g.FillEllipse(&hl2, px + rad * 0.25f, py + rad * 0.35f, rad * 0.22f, rad * 0.14f);
    }
}

// ---------- 7. 爱心飘升 ----------
void addHeartPath(GraphicsPath& p, float s) {
    p.AddBezier(PointF(0, s * 0.95f), PointF(-s * 1.25f, s * 0.15f), PointF(-s * 0.75f, -s * 0.95f), PointF(0, -s * 0.35f));
    p.AddBezier(PointF(0, -s * 0.35f), PointF(s * 0.75f, -s * 0.95f), PointF(s * 1.25f, s * 0.15f), PointF(0, s * 0.95f));
    p.CloseFigure();
}

void drawFxHearts(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 4 + density * 2;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float x0 = (frand(s) - 0.5f) * R * 1.7f;
        const float rise = R * (0.9f + 1.1f * frand(s));
        const float size = R * (0.20f + 0.14f * frand(s));
        const float delay = 0.25f * frand(s);
        const float phase = frand(s) * 2.0f * kPi;
        const float tilt = (frand(s) - 0.5f) * 0.7f;
        const float shade = 0.3f * frand(s) - 0.1f;

        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float px = cx + x0 * easeOutPow(u, 2.0f) + std::sin(u * kPi * 2.0f + phase) * R * 0.08f;
        const float py = cy + R * 0.1f - rise * easeOutPow(u, 1.7f);
        const float pk = (std::min)(1.0f, u / 0.30f);
        const float scale = easeOutPow(pk, 2.0f) * (1.0f + 0.18f * std::sin(pk * kPi));
        const float a = A * clamp01(u * 8.0f) * (1.0f - powSafe(u, 3.0f));
        const float rot = tilt + std::sin(u * kPi * 2.0f + phase) * 0.20f;

        GraphicsPath heart;
        addHeartPath(heart, size);
        Matrix m;
        m.Translate(px, py);
        m.Rotate(rot * 180.0f / kPi);
        m.Scale(scale, scale);
        g.SetTransform(&m);
        SolidBrush body(makeColor(col, a, shade));
        g.FillPath(&body, &heart);
        SolidBrush hl(Color(alphaByte(a * 0.55f), 255, 255, 255));
        g.FillEllipse(&hl, -size * 0.62f, -size * 0.62f, size * 0.34f, size * 0.22f);
        g.ResetTransform();
    }
}

COLORREF hsvShade(COLORREF c, float shade) {
    float r = static_cast<float>(GetRValue(c)), gg = static_cast<float>(GetGValue(c)), b = static_cast<float>(GetBValue(c));
    if (shade >= 0.0f) { r += (255.0f - r) * shade; gg += (255.0f - gg) * shade; b += (255.0f - b) * shade; }
    else { const float k = 1.0f + shade; r *= k; gg *= k; b *= k; }
    return RGB(static_cast<BYTE>(r + 0.5f), static_cast<BYTE>(gg + 0.5f), static_cast<BYTE>(b + 0.5f));
}

// ---------- 8. 雪花飘落 ----------
void drawSnowflakeShape(Graphics& g, float L, float a, COLORREF col, float w) {
    // 在当前坐标原点画一朵六角雪花：先画一层深色底边（浅色桌面上也看得见），再画亮色主体
    for (int pass = 0; pass < 2; ++pass) {
        Pen pen(pass == 0 ? makeColor(col, a * 0.55f, -0.45f) : makeColor(col, a, 0.30f),
                pass == 0 ? w * 2.3f : w);
        pen.SetStartCap(LineCapRound); pen.SetEndCap(LineCapRound);
        for (int k = 0; k < 6; ++k) {
            const float ang = static_cast<float>(k) * kPi / 3.0f;
            const float ux = std::cos(ang), uy = std::sin(ang);
            g.DrawLine(&pen, 0.0f, 0.0f, ux * L, uy * L);
            const float bx = ux * L * 0.58f, by = uy * L * 0.58f;
            for (int sgn = -1; sgn <= 1; sgn += 2) {
                const float ba = ang + static_cast<float>(sgn) * 0.85f;
                g.DrawLine(&pen, bx, by, bx + std::cos(ba) * L * 0.30f, by + std::sin(ba) * L * 0.30f);
            }
        }
    }
}

void drawFxSnow(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 7 + density * 3;
    const float sc = R / 50.0f;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float ang = frand(s) * 2.0f * kPi;
        const float speed = R * (0.5f + 0.9f * frand(s));
        const float delay = 0.08f * frand(s);
        const float L = R * (0.10f + 0.08f * frand(s));
        const float phase = frand(s) * 2.0f * kPi;
        const float spin = (frand(s) - 0.5f) * kPi * 1.6f;
        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float e = easeOutPow(u, 2.0f);
        const float px = cx + std::cos(ang) * speed * e + std::sin(u * kPi * 2.2f + phase) * R * 0.12f * u;
        const float py = cy + std::sin(ang) * speed * e * 0.6f + R * 0.75f * u * (0.5f + 0.5f * u);
        const float a = A * clamp01(u * 8.0f) * (1.0f - powSafe(u, 3.0f));

        softGlow(g, px, py, L * 1.5f, col, a * 0.35f, 0.4f);
        g.TranslateTransform(px, py);
        g.RotateTransform((phase + spin * u) * 180.0f / kPi);
        drawSnowflakeShape(g, L, a, col, (std::max)(1.0f, 1.3f * sc));
        g.ResetTransform();
    }
}

// ---------- 9. 落叶 ----------
void drawFxLeaves(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 6 + density * 2;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float ang = frand(s) * 2.0f * kPi;
        const float speed = R * (0.6f + 0.9f * frand(s));
        const float delay = 0.06f * frand(s);
        const float L = R * (0.20f + 0.12f * frand(s));
        const float phase = frand(s) * 2.0f * kPi;
        const float rot0 = frand(s) * 2.0f * kPi;
        const float shade = 0.45f * frand(s) - 0.28f;
        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float e = easeOutPow(u, 2.2f);
        const float sway = std::sin(u * kPi * 3.0f + phase);
        const float px = cx + std::cos(ang) * speed * e + sway * R * 0.22f * u;
        const float py = cy + std::sin(ang) * speed * e * 0.7f + R * 1.15f * u * u;
        const float rot = rot0 + sway * 0.9f;
        const float flip = 0.45f + 0.55f * std::fabs(std::cos(u * kPi * 2.4f + phase));
        const float a = A * clamp01(u * 10.0f) * (1.0f - powSafe(u, 2.6f));

        const float w = L * 0.42f;
        GraphicsPath leaf;
        leaf.AddBezier(PointF(0, 0), PointF(-w, -L * 0.25f), PointF(-w * 1.1f, -L * 0.75f), PointF(0, -L));
        leaf.AddBezier(PointF(0, -L), PointF(w * 1.1f, -L * 0.75f), PointF(w, -L * 0.25f), PointF(0, 0));
        leaf.CloseFigure();

        Matrix m;
        m.Translate(px, py);
        m.Rotate(rot * 180.0f / kPi);
        m.Scale(flip, 1.0f);
        g.SetTransform(&m);
        SolidBrush body(makeColor(col, a, shade));
        g.FillPath(&body, &leaf);
        Pen edge(makeColor(col, a * 0.6f, shade - 0.30f), 0.9f);
        edge.SetLineJoin(LineJoinRound);
        g.DrawPath(&edge, &leaf);
        Pen vein(makeColor(col, a * 0.7f, shade + 0.30f), 0.9f);
        g.DrawLine(&vein, PointF(0, L * 0.02f), PointF(0, -L * 0.85f));
        Pen stem(makeColor(col, a, shade - 0.40f), 1.3f);
        stem.SetEndCap(LineCapRound);
        g.DrawLine(&stem, PointF(0, 0), PointF(0, L * 0.24f));
        g.ResetTransform();
    }
}

// ---------- 10. 猫爪印 ----------
void drawPawShape(Graphics& g, float s, float a, COLORREF col) {
    // 朝向：脚趾指向局部坐标的 -y 方向
    SolidBrush br(makeColor(col, a));
    g.FillEllipse(&br, -s * 1.0f, -s * 0.4f, s * 2.0f, s * 1.5f);          // 主肉垫
    const float tx[4] = { -1.15f, -0.42f, 0.42f, 1.15f };
    const float ty[4] = { -0.75f, -1.45f, -1.45f, -0.75f };
    const float tilt[4] = { -28.0f, -8.0f, 8.0f, 28.0f };
    for (int i = 0; i < 4; ++i) {                                          // 四颗脚趾豆
        GraphicsState st = g.Save();
        g.TranslateTransform(tx[i] * s, ty[i] * s, MatrixOrderPrepend);
        g.RotateTransform(tilt[i], MatrixOrderPrepend);
        g.FillEllipse(&br, -s * 0.42f, -s * 0.56f, s * 0.84f, s * 1.12f);
        g.Restore(st);
    }
}

void drawFxPaws(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 3 + density;
    unsigned s = seed;
    const float ang0 = -kPi * 0.5f + (frand(s) - 0.5f) * 1.4f;     // 大致朝上“走”出去
    const float step = R * 0.50f;
    const float ps = R * 0.15f;
    const float dx = std::cos(ang0), dy = std::sin(ang0);
    const float nx = -dy, ny = dx;
    for (int k = 0; k < n; ++k) {
        const float appear = static_cast<float>(k) * 0.12f;
        if (t < appear) continue;
        const float pk = (std::min)(1.0f, (t - appear) / 0.12f);
        const float pop = easeOutPow(pk, 2.0f) * (1.0f + 0.2f * std::sin(pk * kPi));
        // 依次淡出：先踩的先消失，平滑缓出；最后一只在生命周期结束之前就已经完全透明
        const float fadeStart = 0.30f + 0.09f * static_cast<float>(k);
        const float fade = 1.0f - smoothstep01((t - fadeStart) / 0.22f);
        if (fade <= 0.0f) continue;
        const float side = (k % 2 == 0) ? -1.0f : 1.0f;
        const float px = cx + dx * step * static_cast<float>(k) + nx * side * R * 0.13f;
        const float py = cy + dy * step * static_cast<float>(k) + ny * side * R * 0.13f;
        g.TranslateTransform(px, py);
        g.RotateTransform((ang0 + kPi * 0.5f) * 180.0f / kPi);
        g.ScaleTransform(pop, pop);
        drawPawShape(g, ps, A * fade, col);
        g.ResetTransform();
    }
}

// ---------- 11. 金币飞溅 ----------
void drawFxCoins(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 5 + density * 2;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float ang = -kPi * 0.5f + (frand(s) - 0.5f) * 1.7f;
        const float v = R * (1.2f + 1.0f * frand(s));
        const float r = R * (0.14f + 0.05f * frand(s));
        const float phase = frand(s) * kPi;
        const float spins = 4.0f + 5.0f * frand(s);
        const float delay = 0.05f * frand(s);
        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float px = cx + std::cos(ang) * v * u;
        const float py = cy + std::sin(ang) * v * u * 1.25f + R * 2.3f * u * u;   // 先上抛，再落下
        const float flip = std::fabs(std::cos(u * spins * kPi + phase));
        const float sx = 0.18f + 0.82f * flip;
        const float a = A * clamp01((1.0f - u) / 0.25f);

        g.TranslateTransform(px, py);
        g.ScaleTransform(sx, 1.0f);
        SolidBrush rim(makeColor(col, a, -0.35f));
        g.FillEllipse(&rim, -r, -r, r * 2.0f, r * 2.0f);
        SolidBrush face(makeColor(col, a, 0.12f));
        g.FillEllipse(&face, -r * 0.78f, -r * 0.78f, r * 1.56f, r * 1.56f);
        Pen ring(makeColor(col, a * 0.8f, -0.25f), 1.0f);
        g.DrawEllipse(&ring, -r * 0.48f, -r * 0.48f, r * 0.96f, r * 0.96f);
        SolidBrush hl(Color(alphaByte(a * 0.85f * flip), 255, 255, 255));
        g.FillEllipse(&hl, -r * 0.62f, -r * 0.62f, r * 0.50f, r * 0.30f);
        g.ResetTransform();
    }
}

// ---------- 12. 彩带礼花 ----------
COLORREF hsvColor(float h, float sat, float v) {
    h -= std::floor(h);
    const float hh = h * 6.0f;
    const int i = static_cast<int>(hh);
    const float f = hh - static_cast<float>(i);
    const float p = v * (1.0f - sat);
    const float q = v * (1.0f - sat * f);
    const float tt = v * (1.0f - sat * (1.0f - f));
    float r = v, gg = tt, b = p;
    switch (i % 6) {
    case 0: r = v;  gg = tt; b = p;  break;
    case 1: r = q;  gg = v;  b = p;  break;
    case 2: r = p;  gg = v;  b = tt; break;
    case 3: r = p;  gg = q;  b = v;  break;
    case 4: r = tt; gg = p;  b = v;  break;
    default: r = v; gg = p;  b = q;  break;
    }
    return RGB(static_cast<BYTE>(r * 255.0f + 0.5f), static_cast<BYTE>(gg * 255.0f + 0.5f), static_cast<BYTE>(b * 255.0f + 0.5f));
}

void drawFxConfetti(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 14 + density * 6;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float ang = -kPi * 0.5f + (frand(s) - 0.5f) * 2.6f;
        const float v = R * (0.9f + 1.4f * frand(s));
        const float w = R * (0.05f + 0.035f * frand(s));
        const float h = w * (1.6f + 1.2f * frand(s));
        const float phase = frand(s) * 2.0f * kPi;
        const float spin = (frand(s) - 0.5f) * 2.0f * kPi * 4.0f;
        const float delay = 0.04f * frand(s);
        const float hue = frand(s);
        const bool useBase = (i % 3 == 0);
        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float e = easeOutPow(u, 2.2f);
        const float px = cx + std::cos(ang) * v * e + std::sin(u * kPi * 3.0f + phase) * R * 0.08f * u;
        const float py = cy + std::sin(ang) * v * e + R * 1.3f * u * u;
        const float flip = 0.15f + 0.85f * std::fabs(std::cos(u * kPi * 4.0f + phase));
        const float a = A * clamp01((1.0f - u) / 0.30f) * clamp01(u * 12.0f);

        Matrix m;
        m.Translate(px, py);
        m.Rotate(spin * u * 180.0f / kPi);
        m.Scale(1.0f, flip);
        g.SetTransform(&m);
        SolidBrush b(useBase ? makeColor(col, a) : makeColor(hsvColor(hue, 0.72f, 1.0f), a));
        g.FillRectangle(&b, -w * 0.5f, -h * 0.5f, w, h);
        g.ResetTransform();
    }
    if (t < 0.12f) {
        const float k = 1.0f - t / 0.12f;
        softGlow(g, cx, cy, R * (0.25f + 0.25f * (1.0f - k)), col, A * 0.8f * k, 0.6f);
    }
}

// ---------- 13. 流星坠落 ----------
void drawFxMeteor(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    unsigned s = seed;
    const float sc = R / 50.0f;
    const float dirAng = -kPi * 0.5f - 0.55f + (frand(s) - 0.5f) * 0.5f;      // 从左上方斜着坠落
    const float sx = cx + std::cos(dirAng) * R * 3.2f;
    const float sy = cy + std::sin(dirAng) * R * 3.2f;
    const float tImpact = 0.42f;

    const float ph = powSafe((std::min)(1.0f, t / tImpact), 1.7f);
    const float pt = powSafe(clamp01((t - 0.05f) / (tImpact + 0.10f - 0.05f)), 1.5f);
    const float hx = lerpf(sx, cx, ph), hy = lerpf(sy, cy, ph);
    const float tx = lerpf(sx, cx, pt), ty = lerpf(sy, cy, pt);

    if (ph - pt > 0.002f) {
        const int nseg = 16;
        for (int j = 0; j < nseg; ++j) {
            const float f0 = static_cast<float>(j) / nseg;
            const float f1 = static_cast<float>(j + 1) / nseg;
            const float x0 = lerpf(tx, hx, f0), y0 = lerpf(ty, hy, f0);
            const float x1 = lerpf(tx, hx, f1), y1 = lerpf(ty, hy, f1);
            const float w = (0.5f + 3.6f * f1) * sc;
            const float al = A * powSafe(f1, 1.5f);
            Pen under(makeColor(col, al * 0.40f, -0.55f), w * 2.2f);
            under.SetStartCap(LineCapRound); under.SetEndCap(LineCapRound);
            g.DrawLine(&under, x0, y0, x1, y1);
            Pen glow(makeColor(col, al * 0.22f), w * 3.4f);
            glow.SetStartCap(LineCapRound); glow.SetEndCap(LineCapRound);
            g.DrawLine(&glow, x0, y0, x1, y1);
            Pen pen(makeColor(col, al), w);
            pen.SetStartCap(LineCapRound); pen.SetEndCap(LineCapRound);
            g.DrawLine(&pen, x0, y0, x1, y1);
        }
    }
    if (t < tImpact + 0.04f) {
        softGlow(g, hx, hy, R * 0.22f, col, A, 0.3f);
        GraphicsPath star;
        addSparklePath(star, hx, hy, R * 0.20f, 0.25f, 4, t * 6.0f);
        SolidBrush b(Color(alphaByte(A), 255, 255, 255));
        g.FillPath(&b, &star);
    }
    if (t >= tImpact) {
        const float k = (t - tImpact) / (1.0f - tImpact);
        const float fadeK = powSafe(1.0f - k, 1.4f);
        Pen ring(makeColor(col, A * fadeK), (std::max)(1.2f, 2.4f * sc * (1.0f - k)));
        const float rr = R * 0.7f * easeOutPow(k, 2.2f);
        g.DrawEllipse(&ring, cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);
        for (int i = 0; i < 9; ++i) {
            const float a2 = frand(s) * 2.0f * kPi;
            const float d = R * (0.3f + 0.5f * frand(s)) * easeOutPow(k, 2.4f);
            const float dr = 2.0f * sc * (1.0f - k) + 0.6f;
            SolidBrush b(makeColor(col, A * fadeK, 0.35f));
            g.FillEllipse(&b, cx + std::cos(a2) * d - dr, cy + std::sin(a2) * d - dr, dr * 2.0f, dr * 2.0f);
        }
        const float ss = R * 0.35f * powSafe(1.0f - k, 0.8f);
        if (ss > 1.0f) {
            softGlow(g, cx, cy, ss * 1.6f, col, A * fadeK, 0.4f);
            GraphicsPath star;
            addSparklePath(star, cx, cy, ss, 0.22f, 4, kPi * 0.25f);
            SolidBrush b(Color(alphaByte(A * fadeK), 255, 255, 255));
            g.FillPath(&b, &star);
        }
    }
}

// ---------- 14. 蝴蝶翩翩 ----------
void drawButterflyShape(Graphics& g, float S, float flap, float a, COLORREF col, float shade) {
    // 局部坐标：身体沿 y 轴，头朝 -y；flap 是翅膀张开程度 (0~1)
    for (int side = -1; side <= 1; side += 2) {
        const float f = static_cast<float>(side);
        GraphicsPath up, lo;
        up.AddBezier(PointF(0, -0.1f * S), PointF(f * 0.9f * S, -1.3f * S), PointF(f * 1.5f * S, -0.2f * S), PointF(0, 0.1f * S));
        up.CloseFigure();
        lo.AddBezier(PointF(0, 0.05f * S), PointF(f * 1.1f * S, 0.0f), PointF(f * 0.8f * S, 1.1f * S), PointF(0, 0.7f * S));
        lo.CloseFigure();
        Matrix m;
        m.Scale(flap, 1.0f);
        up.Transform(&m);
        lo.Transform(&m);
        SolidBrush bu(makeColor(col, a * 0.92f, shade));
        SolidBrush bl(makeColor(col, a * 0.85f, shade - 0.18f));
        g.FillPath(&bl, &lo);
        g.FillPath(&bu, &up);
        Pen edge(makeColor(col, a * 0.7f, shade - 0.40f), 0.9f);
        edge.SetLineJoin(LineJoinRound);
        g.DrawPath(&edge, &up);
        g.DrawPath(&edge, &lo);
        SolidBrush spot(Color(alphaByte(a * 0.75f), 255, 255, 255));
        g.FillEllipse(&spot, f * 0.85f * S * flap - S * 0.13f, -0.62f * S - S * 0.13f, S * 0.26f, S * 0.26f);
    }
    SolidBrush body(makeColor(col, a, -0.6f));
    g.FillEllipse(&body, -0.08f * S, -0.5f * S, 0.16f * S, 1.2f * S);
    Pen ant(makeColor(col, a, -0.6f), 0.9f);
    ant.SetEndCap(LineCapRound);
    g.DrawLine(&ant, PointF(-0.03f * S, -0.5f * S), PointF(-0.32f * S, -0.95f * S));
    g.DrawLine(&ant, PointF(0.03f * S, -0.5f * S), PointF(0.32f * S, -0.95f * S));
}

void drawFxButterflies(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 2 + density;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float th0 = -kPi * 0.5f + (frand(s) - 0.5f) * 2.2f;
        const float speed = R * (1.0f + 0.8f * frand(s));
        const float S = R * (0.24f + 0.08f * frand(s));
        const float phase = frand(s) * 2.0f * kPi;
        const float shade = 0.35f * frand(s) - 0.15f;
        const float delay = 0.10f * static_cast<float>(i);
        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        auto posAt = [&](float uu, float& x, float& y) {
            const float e = easeOutPow(uu, 1.6f);
            const float dx = std::cos(th0), dy = std::sin(th0);
            const float wob = std::sin(uu * kPi * 3.0f + phase) * R * 0.35f * uu;
            x = cx + dx * speed * e + (-dy) * wob;
            y = cy + dy * speed * e + dx * wob;
        };
        float x0, y0, x1, y1;
        posAt(u, x0, y0);
        posAt((std::min)(1.0f, u + 0.01f), x1, y1);
        const float heading = std::atan2(y1 - y0, x1 - x0);
        const float flap = 0.20f + 0.80f * std::fabs(std::cos(u * kPi * 7.0f + phase));
        const float a = A * clamp01(u * 8.0f) * clamp01((1.0f - u) / 0.30f);

        g.TranslateTransform(x0, y0);
        g.RotateTransform((heading + kPi * 0.5f) * 180.0f / kPi);
        drawButterflyShape(g, S, flap, a, col, shade);
        g.ResetTransform();
    }
}

// ---------- 15. 萤火虫 ----------
void drawFxFireflies(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 8 + density * 3;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float ang = frand(s) * 2.0f * kPi;
        const float d = R * (0.3f + 1.0f * frand(s));
        const float phase = frand(s) * 2.0f * kPi;
        const float size = R * (0.045f + 0.035f * frand(s));
        const float delay = 0.15f * frand(s);
        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float e = easeOutPow(u, 1.8f);
        const float px = cx + std::cos(ang) * d * e + std::sin(u * kPi * 3.0f + phase) * R * 0.12f;
        const float py = cy + std::sin(ang) * d * e * 0.8f - R * 0.35f * u + std::cos(u * kPi * 2.3f + phase) * R * 0.10f;
        const float env = std::sin(u * kPi);
        const float blink = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(u * kPi * 7.0f + phase));
        const float a = A * env * blink;

        softGlow(g, px, py, size * 2.3f, col, a * 0.45f, 0.0f);
        SolidBrush core(makeColor(col, a, 0.55f));
        g.FillEllipse(&core, px - size * 0.40f, py - size * 0.40f, size * 0.8f, size * 0.8f);
        SolidBrush hot(Color(alphaByte(a * 0.9f), 255, 255, 235));
        g.FillEllipse(&hot, px - size * 0.20f, py - size * 0.20f, size * 0.4f, size * 0.4f);
    }
}

// ---------- 16. 音符飘升 ----------
void drawMusicNote(Graphics& g, float s, float a, COLORREF col, bool pair) {
    SolidBrush br(makeColor(col, a));
    Pen stem(makeColor(col, a), s * 0.22f);
    stem.SetStartCap(LineCapFlat); stem.SetEndCap(LineCapFlat);
    auto head = [&](float x, float y) {
        GraphicsState st = g.Save();
        g.TranslateTransform(x, y, MatrixOrderPrepend);
        g.RotateTransform(-22.0f, MatrixOrderPrepend);
        g.FillEllipse(&br, -s * 0.62f, -s * 0.44f, s * 1.24f, s * 0.88f);
        g.Restore(st);
    };
    const float stemX = s * 0.52f;
    if (!pair) {                                   // ♪ 单个八分音符
        head(0.0f, 0.0f);
        g.DrawLine(&stem, stemX, 0.0f, stemX, -s * 3.2f);
        Pen flag(makeColor(col, a), s * 0.34f);
        flag.SetStartCap(LineCapRound); flag.SetEndCap(LineCapRound);
        g.DrawBezier(&flag, PointF(stemX, -s * 3.2f), PointF(stemX + s * 1.4f, -s * 2.7f),
                     PointF(stemX + s * 1.6f, -s * 1.7f), PointF(stemX + s * 0.7f, -s * 1.1f));
    } else {                                       // ♫ 连梁双音符
        const float x2 = s * 2.1f, y2 = -s * 0.45f;
        head(0.0f, 0.0f);
        head(x2, y2);
        g.DrawLine(&stem, stemX, 0.0f, stemX, -s * 3.2f);
        g.DrawLine(&stem, x2 + stemX, y2, x2 + stemX, -s * 3.65f);
        PointF beam[4] = { PointF(stemX - s * 0.11f, -s * 3.2f), PointF(x2 + stemX + s * 0.11f, -s * 3.65f),
                           PointF(x2 + stemX + s * 0.11f, -s * 3.10f), PointF(stemX - s * 0.11f, -s * 2.65f) };
        g.FillPolygon(&br, beam, 4);
    }
}

void drawFxNotes(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 4 + density;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float x0 = (frand(s) - 0.5f) * R * 1.3f;
        const float rise = R * (1.0f + 0.9f * frand(s));
        const float size = R * (0.09f + 0.05f * frand(s));
        const float delay = 0.22f * frand(s);
        const float phase = frand(s) * 2.0f * kPi;
        const float tilt = (frand(s) - 0.5f) * 0.5f;
        const float shade = 0.3f * frand(s) - 0.12f;
        const bool pair = (i % 3 == 1);
        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float px = cx + x0 * easeOutPow(u, 2.0f) + std::sin(u * kPi * 2.0f + phase) * R * 0.10f;
        const float py = cy - rise * easeOutPow(u, 1.6f) + R * 0.1f;
        const float pk = (std::min)(1.0f, u / 0.30f);
        const float scale = easeOutPow(pk, 2.0f) * (1.0f + 0.18f * std::sin(pk * kPi));
        const float a = A * clamp01(u * 8.0f) * (1.0f - powSafe(u, 3.0f));
        const float rot = tilt + std::sin(u * kPi * 2.0f + phase) * 0.18f;

        Matrix m;
        m.Translate(px, py);
        m.Rotate(rot * 180.0f / kPi);
        m.Scale(scale, scale);
        g.SetTransform(&m);
        drawMusicNote(g, size, a, hsvShade(col, shade), pair);
        g.ResetTransform();
    }
}

// ---------- 17. 星环爆裂 (独立版：光环扩散 + 十字星芒 + 火花) ----------
void drawFxStarBurst(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const float sc = R / 50.0f;
    unsigned s = seed;
    const float rot0 = frand(s) * kPi;

    // 1) 十字星芒 + 中心四角星：瞬间亮起，快速收缩
    const float flare = powSafe(1.0f - t, 1.2f) * easeOutPow((std::min)(1.0f, t / 0.10f), 2.0f);
    if (flare > 0.01f) {
        const float len = R * (0.55f + 0.50f * easeOutPow(t, 2.0f));
        for (int k = 0; k < 4; ++k) {
            const float a = rot0 + static_cast<float>(k) * kPi * 0.5f;
            const float L = (k % 2 == 0) ? len : len * 0.62f;
            const float ux = std::cos(a), uy = std::sin(a);
            const float w = 2.4f * sc * flare;
            PointF tri[3] = { PointF(cx - uy * w, cy + ux * w), PointF(cx + ux * L, cy + uy * L), PointF(cx + uy * w, cy - ux * w) };
            SolidBrush glowB(makeColor(col, A * 0.35f * flare));
            PointF triW[3] = { PointF(cx - uy * w * 2.6f, cy + ux * w * 2.6f), PointF(cx + ux * L, cy + uy * L), PointF(cx + uy * w * 2.6f, cy - ux * w * 2.6f) };
            g.FillPolygon(&glowB, triW, 3);
            SolidBrush b(makeColor(col, A * flare, 0.55f));
            g.FillPolygon(&b, tri, 3);
        }
        softGlow(g, cx, cy, R * 0.50f * (0.5f + 0.5f * flare), col, A * 0.9f * flare, 0.35f);
        GraphicsPath star;
        addSparklePath(star, cx, cy, R * 0.36f * flare, 0.24f, 4, rot0 + t * 1.2f);
        SolidBrush core(Color(alphaByte(A * flare), 255, 255, 255));
        g.FillPath(&core, &star);
    }

    // 2) 两圈先后扩散的光环
    for (int ri = 0; ri < 2; ++ri) {
        const float delay = (ri == 0) ? 0.0f : 0.12f;
        const float k = clamp01((t - delay) / (1.0f - delay));
        if (k <= 0.0f || k >= 1.0f) continue;
        const float rr = R * ((ri == 0) ? 0.95f : 0.65f) * easeOutPow(k, 2.2f);
        const float al = A * ((ri == 0) ? 1.0f : 0.8f) * powSafe(1.0f - k, 1.4f);
        const float w = (std::max)(1.2f, 2.8f * sc * (1.0f - k));
        Pen under(makeColor(col, al * 0.35f, -0.55f), w * 2.2f);
        g.DrawEllipse(&under, cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);
        Pen glow(makeColor(col, al * 0.22f), w * 3.6f);
        g.DrawEllipse(&glow, cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);
        Pen ring(makeColor(col, al, 0.2f), w);
        g.DrawEllipse(&ring, cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);
    }

    // 3) 向外飞散的火花点与小星星
    const int ns = 8 + density * 3;
    const float kk = clamp01((t - 0.04f) / 0.96f);
    const float sparkA = A * powSafe(1.0f - t, 1.2f);
    for (int i = 0; i < ns; ++i) {
        const float ang = frand(s) * 2.0f * kPi;
        const float d = R * (0.35f + 0.65f * frand(s)) * easeOutPow(kk, 2.4f);
        const float px = cx + std::cos(ang) * d, py = cy + std::sin(ang) * d;
        if (i % 3 == 0) {
            const float sz = 4.5f * sc * powSafe(1.0f - t, 0.6f);
            if (sz > 0.8f) {
                softGlow(g, px, py, sz * 1.8f, col, sparkA * 0.6f, 0.4f);
                GraphicsPath tw;
                addSparklePath(tw, px, py, sz, 0.24f, 4, ang + t * 2.0f);
                SolidBrush b(Color(alphaByte(sparkA), 255, 255, 255));
                g.FillPath(&b, &tw);
            }
        } else {
            const float dr = (1.6f + 1.4f * frand(s)) * sc * (1.0f - t) + 0.6f;
            SolidBrush b(makeColor(col, sparkA, 0.35f));
            g.FillEllipse(&b, px - dr, py - dr, dr * 2.0f, dr * 2.0f);
        }
    }
}

// ---------- 18. 雨滴溅起 ----------
void drawFxRain(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const float sc = R / 50.0f;
    const int n = 5 + density * 2;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float x0 = (frand(s) - 0.5f) * R * 1.5f;
        const float y0 = (frand(s) - 0.5f) * R * 0.5f;
        const float land = 0.16f + 0.34f * frand(s);
        const float dropLen = R * (0.35f + 0.25f * frand(s));
        const float phase = frand(s) * 2.0f * kPi;
        const float px = cx + x0, py = cy + y0;
        if (t < land) {
            // 下落：加速砸向地面
            const float f = t / land;
            const float yHead = py - R * 2.0f * (1.0f - f * f);
            const float al = A * 0.9f * clamp01(f * 6.0f);
            Pen under(makeColor(col, al * 0.35f, -0.5f), 3.2f * sc);
            under.SetStartCap(LineCapRound); under.SetEndCap(LineCapRound);
            g.DrawLine(&under, px, yHead - dropLen, px, yHead);
            Pen pen(makeColor(col, al, 0.2f), 1.7f * sc);
            pen.SetStartCap(LineCapRound); pen.SetEndCap(LineCapRound);
            g.DrawLine(&pen, px, yHead - dropLen, px, yHead);
        } else {
            // 溅起：扁平的涟漪圈 + 几颗向上弹起的小水珠
            const float k = (t - land) / (1.0f - land);
            if (k >= 1.0f) continue;
            const float fadeK = powSafe(1.0f - k, 1.5f);
            for (int ri = 0; ri < 2; ++ri) {
                const float kr = clamp01((k - 0.15f * static_cast<float>(ri)) / (1.0f - 0.15f * static_cast<float>(ri)));
                if (kr <= 0.0f) continue;
                const float rx = R * (0.10f + 0.28f * easeOutPow(kr, 2.0f)) * (ri == 0 ? 1.0f : 0.65f);
                const float ry = rx * 0.36f;
                const float al = A * powSafe(1.0f - kr, 1.5f) * (ri == 0 ? 1.0f : 0.7f);
                const float w = (std::max)(1.0f, 1.8f * sc * (1.0f - kr));
                Pen under(makeColor(col, al * 0.35f, -0.5f), w * 2.2f);
                g.DrawEllipse(&under, px - rx, py - ry, rx * 2.0f, ry * 2.0f);
                Pen pen(makeColor(col, al, 0.2f), w);
                g.DrawEllipse(&pen, px - rx, py - ry, rx * 2.0f, ry * 2.0f);
            }
            for (int j = 0; j < 4; ++j) {
                const float a = -kPi * (0.15f + 0.70f * (static_cast<float>(j) + 0.5f * std::sin(phase + static_cast<float>(j))) / 4.0f);
                const float vv = R * (0.25f + 0.20f * frand(s));
                const float dx = std::cos(a) * vv * k;
                const float dy = std::sin(a) * vv * k + R * 0.55f * k * k;
                const float dr = 1.7f * sc * (1.0f - k) + 0.5f;
                SolidBrush b(makeColor(col, A * fadeK, 0.25f));
                g.FillEllipse(&b, px + dx - dr, py + dy - dr, dr * 2.0f, dr * 2.0f);
            }
        }
    }
}

// ---------- 19. 魔法阵 ----------
void drawRegularPoly(Graphics& g, Pen& pen, float cx, float cy, float r, int n, float rot) {
    PointF pts[8];
    for (int i = 0; i < n; ++i) {
        const float a = rot + 2.0f * kPi * static_cast<float>(i) / static_cast<float>(n);
        pts[i] = PointF(cx + std::cos(a) * r, cy + std::sin(a) * r);
    }
    g.DrawPolygon(&pen, pts, n);
}

void drawFxMagic(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const float sc = R / 50.0f;
    unsigned s = seed;
    const float rotBase = frand(s) * 2.0f * kPi;
    const float scaleIn = 0.35f + 0.65f * easeOutPow((std::min)(1.0f, t / 0.25f), 2.5f);
    const float fade = 1.0f - smoothstep01((t - 0.62f) / 0.38f);
    const float a = A * fade * clamp01(t / 0.08f);
    if (a < 1.0f) return;
    const float r0 = R * 0.95f * scaleIn;
    const float rot = rotBase + t * kPi * 1.2f;

    softGlow(g, cx, cy, r0 * 0.9f, col, a * 0.30f, 0.0f);

    // 三层描边：深色底边（浅色背景也看得清）+ 光晕 + 亮线
    auto layered = [&](float w, auto&& fn) {
        Pen under(makeColor(col, a * 0.35f, -0.55f), w * 2.4f);
        under.SetLineJoin(LineJoinRound); fn(under);
        Pen glow(makeColor(col, a * 0.22f), w * 4.0f);
        glow.SetLineJoin(LineJoinRound); fn(glow);
        Pen main(makeColor(col, a, 0.20f), w);
        main.SetLineJoin(LineJoinRound); fn(main);
    };
    const float wBase = (std::max)(1.0f, 1.8f * sc);

    layered(wBase, [&](Pen& p) { g.DrawEllipse(&p, cx - r0, cy - r0, r0 * 2.0f, r0 * 2.0f); });
    layered(wBase * 0.6f, [&](Pen& p) { const float r = r0 * 0.86f; g.DrawEllipse(&p, cx - r, cy - r, r * 2.0f, r * 2.0f); });
    // 外圈刻度
    layered(wBase * 0.5f, [&](Pen& p) {
        for (int i = 0; i < 36; ++i) {
            const float ang = -rot * 0.5f + 2.0f * kPi * static_cast<float>(i) / 36.0f;
            const float r1 = r0 * ((i % 3 == 0) ? 0.86f : 0.90f), r2 = r0 * 0.97f;
            g.DrawLine(&p, cx + std::cos(ang) * r1, cy + std::sin(ang) * r1, cx + std::cos(ang) * r2, cy + std::sin(ang) * r2);
        }
    });
    // 六芒星（顺时针旋转）
    layered(wBase * 0.9f, [&](Pen& p) {
        drawRegularPoly(g, p, cx, cy, r0 * 0.80f, 3, rot);
        drawRegularPoly(g, p, cx, cy, r0 * 0.80f, 3, rot + kPi / 3.0f);
    });
    // 内圈 + 内方框（逆时针旋转）
    layered(wBase * 0.7f, [&](Pen& p) {
        g.DrawEllipse(&p, cx - r0 * 0.42f, cy - r0 * 0.42f, r0 * 0.84f, r0 * 0.84f);
        drawRegularPoly(g, p, cx, cy, r0 * 0.42f, 4, -rot * 1.4f);
    });
    // 六芒星顶点上的小圆点
    for (int i = 0; i < 6; ++i) {
        const float ang = rot + kPi / 3.0f * static_cast<float>(i);
        const float px = cx + std::cos(ang) * r0 * 0.80f, py = cy + std::sin(ang) * r0 * 0.80f;
        const float dr = 2.4f * sc;
        SolidBrush b(makeColor(col, a, 0.6f));
        g.FillEllipse(&b, px - dr, py - dr, dr * 2.0f, dr * 2.0f);
    }
    // 绕圈飞舞的星光
    for (int k = 0; k < 3; ++k) {
        const float ang = rot * 2.0f + 2.0f * kPi * static_cast<float>(k) / 3.0f;
        const float px = cx + std::cos(ang) * r0 * 0.93f, py = cy + std::sin(ang) * r0 * 0.93f;
        softGlow(g, px, py, 8.0f * sc, col, a * 0.8f, 0.4f);
        GraphicsPath st;
        addSparklePath(st, px, py, 5.5f * sc, 0.25f, 4, ang);
        SolidBrush b(Color(alphaByte(a), 255, 255, 255));
        g.FillPath(&b, &st);
    }
}

// ---------- 20. 漫画爆炸 ----------
void drawFxComic(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const float sc = R / 50.0f;
    unsigned s = seed;
    const float fade = 1.0f - smoothstep01((t - 0.62f) / 0.38f);
    const float a = A * fade;
    if (a < 1.0f) return;

    float scale;
    if (t < 0.18f) scale = easeOutPow(t / 0.18f, 2.0f) * 1.15f;
    else if (t < 0.34f) scale = lerpf(1.15f, 1.0f, (t - 0.18f) / 0.16f);
    else scale = 1.0f;
    scale *= 1.0f - 0.15f * smoothstep01((t - 0.62f) / 0.38f);
    const float shake = std::sin(t * 60.0f) * 0.10f * (1.0f - t);
    const float rot = (frand(s) - 0.5f) * 0.3f + shake;

    const int spikes = 12 + density;
    PointF pts[40];
    const int cnt = spikes * 2;
    for (int i = 0; i < cnt; ++i) {
        const float ang = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(cnt);
        const float rr = (i % 2 == 0) ? R * (0.70f + 0.30f * frand(s)) : R * (0.42f + 0.08f * frand(s));
        pts[i] = PointF(std::cos(ang) * rr, std::sin(ang) * rr);
    }

    // 爆炸线（冲出尖角之外的短线）
    if (t < 0.55f) {
        const float k = 1.0f - t / 0.55f;
        Pen line(makeColor(col, a * k, -0.55f), (std::max)(1.2f, 2.2f * sc));
        line.SetStartCap(LineCapRound); line.SetEndCap(LineCapRound);
        for (int i = 0; i < 8; ++i) {
            const float ang = rot + 2.0f * kPi * (static_cast<float>(i) + 0.5f) / 8.0f;
            const float r1 = R * (1.05f + 0.1f * frand(s)) * scale, r2 = r1 + R * 0.30f * k;
            g.DrawLine(&line, cx + std::cos(ang) * r1, cy + std::sin(ang) * r1, cx + std::cos(ang) * r2, cy + std::sin(ang) * r2);
        }
    }

    g.TranslateTransform(cx, cy);
    g.RotateTransform(rot * 180.0f / kPi);
    g.ScaleTransform(scale, scale);
    SolidBrush outer(makeColor(col, a, 0.0f));
    g.FillPolygon(&outer, pts, cnt);
    Pen outline(makeColor(col, a, -0.70f), (std::max)(1.5f, 3.2f * sc));
    outline.SetLineJoin(LineJoinMiter);
    g.DrawPolygon(&outline, pts, cnt);
    // 内层：更亮的一圈，像爆炸的火心
    PointF inner[40];
    for (int i = 0; i < cnt; ++i) inner[i] = PointF(pts[i].X * 0.62f, pts[i].Y * 0.62f);
    SolidBrush mid(makeColor(col, a, 0.45f));
    g.FillPolygon(&mid, inner, cnt);
    PointF core[40];
    for (int i = 0; i < cnt; ++i) core[i] = PointF(pts[i].X * 0.30f, pts[i].Y * 0.30f);
    SolidBrush hot(Color(alphaByte(a * 0.75f), 255, 255, 240));
    g.FillPolygon(&hot, core, cnt);
    g.ResetTransform();

    // 周围弹出的三个小爆炸
    for (int m = 0; m < 3; ++m) {
        const float ang = frand(s) * 2.0f * kPi;
        const float dd = R * (1.05f + 0.25f * frand(s));
        const float delay = 0.10f + 0.10f * static_cast<float>(m);
        const float k = clamp01((t - delay) / 0.30f);
        if (k <= 0.0f) continue;
        const float ms = R * 0.20f * easeOutPow(k, 2.0f) * (1.0f - 0.5f * smoothstep01((t - 0.62f) / 0.38f));
        PointF mp[12];
        for (int i = 0; i < 12; ++i) {
            const float aa = 2.0f * kPi * static_cast<float>(i) / 12.0f;
            const float rr = (i % 2 == 0) ? ms : ms * 0.5f;
            mp[i] = PointF(cx + std::cos(ang) * dd + std::cos(aa) * rr, cy + std::sin(ang) * dd + std::sin(aa) * rr);
        }
        SolidBrush mb(makeColor(col, a, 0.2f));
        g.FillPolygon(&mb, mp, 12);
        Pen mo(makeColor(col, a, -0.70f), (std::max)(1.0f, 1.8f * sc));
        mo.SetLineJoin(LineJoinMiter);
        g.DrawPolygon(&mo, mp, 12);
    }
}

// ---------- 21. 彩虹光环 ----------
void drawFxRainbow(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    (void)col; (void)seed; (void)density;
    const float sc = R / 50.0f;
    const int nr = 6;
    for (int i = 0; i < nr; ++i) {
        const float delay = static_cast<float>(i) * 0.045f;
        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;
        const float rFinal = R * (1.0f - 0.095f * static_cast<float>(i));
        const float rr = rFinal * easeOutPow(u, 2.2f);
        const float al = A * powSafe(1.0f - u, 1.1f) * clamp01(u * 10.0f);
        const COLORREF c = hsvColor(static_cast<float>(i) / static_cast<float>(nr) * 0.78f, 0.85f, 1.0f);
        const float w = (std::max)(1.2f, 3.0f * sc * (1.0f - 0.4f * u));
        Pen glow(makeColor(c, al * 0.22f), w * 3.2f);
        g.DrawEllipse(&glow, cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);
        Pen pen(makeColor(c, al), w);
        g.DrawEllipse(&pen, cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);
    }
    if (t < 0.22f) {
        const float k = 1.0f - t / 0.22f;
        softGlow(g, cx, cy, R * 0.30f * (1.5f - k * 0.5f), RGB(255, 255, 255), A * 0.8f * k, 0.0f);
    }
}

// ---------- 22. 气球升空 ----------
void drawFxBalloons(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 3 + density;
    unsigned s = seed;
    for (int i = 0; i < n; ++i) {
        const float x0 = (frand(s) - 0.5f) * R * 1.2f;
        const float rise = R * (1.7f + 1.0f * frand(s));
        const float rx = R * (0.15f + 0.05f * frand(s));
        const float delay = 0.12f * frand(s);
        const float phase = frand(s) * 2.0f * kPi;
        const float hue = frand(s);
        const bool useBase = (i % 3 == 0);
        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float ry = rx * 1.22f;
        const float sway = std::sin(u * kPi * 2.2f + phase);
        const float px = cx + x0 * easeOutPow(u, 2.0f) + sway * R * 0.08f;
        const float py = cy + R * 0.05f - rise * easeOutPow(u, 1.5f);
        const float pk = (std::min)(1.0f, u / 0.20f);
        const float scale = easeOutPow(pk, 2.0f) * (1.0f + 0.12f * std::sin(pk * kPi));
        const float a = A * clamp01(u * 8.0f) * clamp01((1.0f - u) / 0.25f);
        const COLORREF c = useBase ? col : hsvColor(hue, 0.70f, 1.0f);

        g.TranslateTransform(px, py);
        g.RotateTransform(sway * 8.0f);
        g.ScaleTransform(scale, scale);
        Pen str(makeColor(RGB(120, 120, 130), a * 0.8f), 1.0f);
        g.DrawBezier(&str, PointF(0, ry * 1.1f), PointF(rx * 0.5f, ry * 1.6f), PointF(-rx * 0.5f, ry * 2.1f), PointF(0, ry * 2.7f));
        SolidBrush body(makeColor(c, a));
        g.FillEllipse(&body, -rx, -ry, rx * 2.0f, ry * 2.0f);
        Pen rim(makeColor(c, a * 0.8f, -0.35f), 1.0f);
        g.DrawEllipse(&rim, -rx, -ry, rx * 2.0f, ry * 2.0f);
        PointF knot[3] = { PointF(0, ry * 0.98f), PointF(-rx * 0.16f, ry * 1.20f), PointF(rx * 0.16f, ry * 1.20f) };
        SolidBrush kb(makeColor(c, a, -0.25f));
        g.FillPolygon(&kb, knot, 3);
        SolidBrush hl(Color(alphaByte(a * 0.6f), 255, 255, 255));
        g.FillEllipse(&hl, -rx * 0.62f, -ry * 0.72f, rx * 0.40f, ry * 0.30f);
        g.ResetTransform();
    }
}

// ---------- 23. 烈焰升腾 ----------
void addFlamePath(GraphicsPath& p, float w, float h) {
    // 水滴形火苗：尖端朝上 (0,-h)，圆润的底部在 (0, 0.55h)
    p.AddBezier(PointF(0, -h), PointF(w * 0.35f, -h * 0.45f), PointF(w * 1.2f, 0.0f), PointF(0, h * 0.55f));
    p.AddBezier(PointF(0, h * 0.55f), PointF(-w * 1.2f, 0.0f), PointF(-w * 0.35f, -h * 0.45f), PointF(0, -h));
    p.CloseFigure();
}

void drawFxFlames(Graphics& g, float cx, float cy, float t, float R, float A, COLORREF col, unsigned seed, int density) {
    const int n = 7 + density * 3;
    unsigned s = seed;
    if (t < 0.6f) softGlow(g, cx, cy, R * 0.55f * (1.0f - t / 0.6f), col, A * 0.55f, 0.1f);
    for (int i = 0; i < n; ++i) {
        const float x0 = (frand(s) - 0.5f) * R * 0.9f;
        const float y0 = (frand(s) - 0.2f) * R * 0.3f;
        const float rise = R * (1.0f + 1.0f * frand(s));
        const float w = R * (0.12f + 0.07f * frand(s));
        const float h = w * (2.4f + 1.0f * frand(s));
        const float delay = 0.20f * frand(s);
        const float phase = frand(s) * 2.0f * kPi;
        const float u = clamp01((t - delay) / (1.0f - delay));
        if (u <= 0.0f || u >= 1.0f) continue;

        const float px = cx + x0 * (1.0f - 0.5f * u) + std::sin(u * kPi * 5.0f + phase) * R * 0.05f * (1.0f - u);
        const float py = cy + y0 - rise * easeOutPow(u, 1.4f);
        const float sizeK = (1.0f - 0.75f * u) * easeOutPow((std::min)(1.0f, u / 0.15f), 2.0f);
        const float a = A * clamp01(u * 10.0f) * powSafe(1.0f - u, 0.8f);
        const float wob = std::sin(u * kPi * 6.0f + phase) * 6.0f;

        softGlow(g, px, py, w * 2.8f * sizeK, col, a * 0.35f, 0.0f);
        g.TranslateTransform(px, py);
        g.RotateTransform(wob);
        const float shades[3] = { 0.0f, 0.38f, 0.78f };
        const float scales[3] = { 1.0f, 0.68f, 0.38f };
        const float alphas[3] = { 0.85f, 0.90f, 0.95f };
        for (int L = 0; L < 3; ++L) {
            GraphicsPath fl;
            addFlamePath(fl, w * sizeK * scales[L], h * sizeK * scales[L]);
            SolidBrush b(makeColor(col, a * alphas[L], shades[L]));
            g.FillPath(&b, &fl);
        }
        g.ResetTransform();
    }
}

// =============================================================
// 鼠标拖尾：散落的粒子 / 彩虹带，跟随鼠标移动
// 记录鼠标轨迹时，每隔一小段距离“放出”一颗粒子（trailPushPoint 决定），
// 每颗粒子根据自己的年龄 u (0~1) 独立飘动、渐隐；不需要保存任何额外状态。
// =============================================================
void fillPetalShape(Graphics& g, float L, float a, COLORREF col, float shade) {
    const float w = L * 0.55f;
    GraphicsPath petal;
    petal.AddBezier(PointF(0, 0), PointF(-w, -L * 0.35f), PointF(-w * 0.7f, -L * 0.9f), PointF(0, -L));
    petal.AddBezier(PointF(0, -L), PointF(w * 0.7f, -L * 0.9f), PointF(w, -L * 0.35f), PointF(0, 0));
    petal.CloseFigure();
    SolidBrush body(makeColor(col, a, shade));
    g.FillPath(&body, &petal);
    Pen vein(makeColor(col, a * 0.45f, shade + 0.25f), 0.8f);
    g.DrawLine(&vein, PointF(0, -L * 0.08f), PointF(0, -L * 0.78f));
}

void fillLeafShape(Graphics& g, float L, float a, COLORREF col, float shade) {
    const float w = L * 0.42f;
    GraphicsPath leaf;
    leaf.AddBezier(PointF(0, 0), PointF(-w, -L * 0.25f), PointF(-w * 1.1f, -L * 0.75f), PointF(0, -L));
    leaf.AddBezier(PointF(0, -L), PointF(w * 1.1f, -L * 0.75f), PointF(w, -L * 0.25f), PointF(0, 0));
    leaf.CloseFigure();
    SolidBrush body(makeColor(col, a, shade));
    g.FillPath(&body, &leaf);
    Pen edge(makeColor(col, a * 0.6f, shade - 0.30f), 0.8f);
    g.DrawPath(&edge, &leaf);
    Pen vein(makeColor(col, a * 0.7f, shade + 0.30f), 0.8f);
    g.DrawLine(&vein, PointF(0, L * 0.02f), PointF(0, -L * 0.85f));
}

// 记录一个拖尾点；按距离间隔决定这个点是否“放出”一颗粒子
void trailPushPoint(std::vector<TrailPoint>& pts, POINT pt, double nowMs, float spacing) {
    static float s_accum = 0.0f;
    static unsigned s_counter = 1u;
    TrailPoint tp;
    tp.screenPt = pt;
    tp.timeMs = nowMs;
    if (pts.empty()) {
        s_accum = spacing;                       // 轨迹的第一个点总是放出一颗
    } else {
        const float dx = static_cast<float>(pt.x - pts.back().screenPt.x);
        const float dy = static_cast<float>(pt.y - pts.back().screenPt.y);
        s_accum += std::sqrt(dx * dx + dy * dy);
    }
    if (s_accum >= spacing) {
        tp.emit = true;
        s_accum = 0.0f;
        ++s_counter;
        tp.seed = s_counter * 2654435761u ^ (static_cast<unsigned>(pt.x) * 73856093u) ^ (static_cast<unsigned>(pt.y) * 19349663u);
    }
    pts.push_back(tp);
}

// 画一颗拖尾粒子。x,y = 它诞生的位置；u = 年龄 0~1；sc = 大小倍率；A = 不透明度 0~255
void drawTrailParticle(Graphics& g, int style, float x, float y, float u, float sc, float A, COLORREF col, unsigned seed) {
    unsigned s = seed;
    const float r1 = frand(s), r2 = frand(s), r3 = frand(s), r4 = frand(s);
    const float phase = r3 * 2.0f * kPi;
    x += (r1 - 0.5f) * 10.0f * sc;
    y += (r2 - 0.5f) * 10.0f * sc;

    switch (style) {
    case TRAIL_SPARKLE: {
        const float ang = r4 * 2.0f * kPi;
        const float d = 14.0f * sc * easeOutPow(u, 1.8f) * (0.4f + r1);
        const float px = x + std::cos(ang) * d, py = y + std::sin(ang) * d - 6.0f * sc * u;
        const float pulse = std::sin(u * kPi);
        const float sz = (3.0f + 5.0f * r2) * sc * (0.2f + 0.8f * powSafe(pulse, 0.7f));
        const float a = A * powSafe(pulse, 0.6f) * (0.8f + 0.2f * std::sin(u * kPi * 5.0f + phase));
        const float shade = 0.5f * r4;
        const bool five = (r3 > 0.6f);
        softGlow(g, px, py, sz * 1.9f, col, a * 0.55f, shade);
        GraphicsPath p;
        if (five) addSparklePath(p, px, py, sz * 0.9f, 0.45f, 5, phase + u * 1.5f - kPi * 0.5f);
        else addSparklePath(p, px, py, sz * 1.15f, 0.24f, 4, phase + u * 1.5f);
        SolidBrush b(makeColor(col, a, shade));
        g.FillPath(&b, &p);
        Pen edge(makeColor(col, a * 0.6f, -0.30f), 0.8f);
        edge.SetLineJoin(LineJoinRound);
        g.DrawPath(&edge, &p);
        SolidBrush core(Color(alphaByte(a * 0.9f), 255, 255, 255));
        const float cr = sz * 0.18f;
        g.FillEllipse(&core, px - cr, py - cr, cr * 2.0f, cr * 2.0f);
    } break;

    case TRAIL_PETAL: {
        const float px = x + std::sin(u * kPi * 2.0f + phase) * 12.0f * sc * u;
        const float py = y + 38.0f * sc * u * u + 8.0f * sc * u;
        const float rot = phase + (r4 - 0.5f) * 2.0f * kPi * 2.0f * u;
        const float flip = 0.30f + 0.70f * std::fabs(std::cos(u * kPi * 3.0f + phase));
        const float a = A * clamp01(u * 10.0f) * (1.0f - powSafe(u, 2.2f));
        g.TranslateTransform(px, py);
        g.RotateTransform(rot * 180.0f / kPi);
        g.ScaleTransform(flip, 1.0f);
        fillPetalShape(g, (9.0f + 5.0f * r2) * sc, a, col, 0.4f * r4 - 0.12f);
        g.ResetTransform();
    } break;

    case TRAIL_LEAF: {
        const float sw = std::sin(u * kPi * 2.6f + phase);
        const float px = x + sw * 14.0f * sc * u;
        const float py = y + 42.0f * sc * u * u + 6.0f * sc * u;
        const float flip = 0.45f + 0.55f * std::fabs(std::cos(u * kPi * 2.2f + phase));
        const float a = A * clamp01(u * 10.0f) * (1.0f - powSafe(u, 2.4f));
        g.TranslateTransform(px, py);
        g.RotateTransform((phase + sw * 0.9f) * 180.0f / kPi);
        g.ScaleTransform(flip, 1.0f);
        fillLeafShape(g, (7.0f + 4.0f * r2) * sc, a, col, 0.4f * r4 - 0.26f);
        g.ResetTransform();
    } break;

    case TRAIL_SNOW: {
        const float px = x + std::sin(u * kPi * 2.2f + phase) * 10.0f * sc * u;
        const float py = y + 28.0f * sc * u * (0.5f + 0.5f * u);
        const float L = (3.6f + 2.6f * r2) * sc;
        const float a = A * clamp01(u * 8.0f) * (1.0f - powSafe(u, 3.0f));
        softGlow(g, px, py, L * 1.5f, col, a * 0.35f, 0.4f);
        g.TranslateTransform(px, py);
        g.RotateTransform((phase + (r4 - 0.5f) * 3.0f * u) * 180.0f / kPi);
        drawSnowflakeShape(g, L, a, col, (std::max)(1.0f, 1.0f * sc));
        g.ResetTransform();
    } break;

    case TRAIL_HEART: {
        const float px = x + std::sin(u * kPi * 2.0f + phase) * 7.0f * sc;
        const float py = y - 30.0f * sc * easeOutPow(u, 1.6f);
        const float size = (3.6f + 2.4f * r2) * sc;
        const float pk = (std::min)(1.0f, u / 0.25f);
        const float scale = easeOutPow(pk, 2.0f) * (1.0f + 0.18f * std::sin(pk * kPi));
        const float a = A * clamp01(u * 8.0f) * (1.0f - powSafe(u, 3.0f));
        GraphicsPath heart;
        addHeartPath(heart, size);
        g.TranslateTransform(px, py);
        g.RotateTransform((r4 - 0.5f) * 40.0f + std::sin(u * kPi * 2.0f + phase) * 10.0f);
        g.ScaleTransform(scale, scale);
        SolidBrush b(makeColor(col, a, 0.3f * r4 - 0.1f));
        g.FillPath(&b, &heart);
        SolidBrush hl(Color(alphaByte(a * 0.55f), 255, 255, 255));
        g.FillEllipse(&hl, -size * 0.62f, -size * 0.62f, size * 0.34f, size * 0.22f);
        g.ResetTransform();
    } break;

    case TRAIL_BUBBLE: {
        const float px = x + std::sin(u * kPi * 2.2f + phase) * 8.0f * sc;
        const float py = y - 26.0f * sc * easeOutPow(u, 1.5f);
        const float pop = (u > 0.85f) ? (u - 0.85f) / 0.15f : 0.0f;
        const float rad = (3.0f + 4.0f * r2) * sc * (0.6f + 0.4f * easeOutPow(u, 2.0f)) * (1.0f + 0.5f * pop);
        const float a = A * clamp01(u * 8.0f) * (1.0f - pop);
        SolidBrush fill(makeColor(col, a * 0.14f, 0.4f));
        g.FillEllipse(&fill, px - rad, py - rad, rad * 2.0f, rad * 2.0f);
        Pen rim(makeColor(col, a * 0.85f, 0.15f), (std::max)(1.0f, 1.1f * sc));
        g.DrawEllipse(&rim, px - rad, py - rad, rad * 2.0f, rad * 2.0f);
        SolidBrush hl(Color(alphaByte(a * 0.85f), 255, 255, 255));
        g.FillEllipse(&hl, px - rad * 0.55f, py - rad * 0.58f, rad * 0.38f, rad * 0.24f);
    } break;

    case TRAIL_FIREFLY: {
        const float px = x + std::cos(u * kPi * 2.3f + phase) * 10.0f * sc;
        const float py = y - 14.0f * sc * u + std::sin(u * kPi * 3.0f + phase) * 8.0f * sc;
        const float size = (2.4f + 1.8f * r2) * sc;
        const float env = std::sin(u * kPi);
        const float blink = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(u * kPi * 7.0f + phase));
        const float a = A * env * blink;
        softGlow(g, px, py, size * 2.6f, col, a * 0.5f, 0.0f);
        SolidBrush core(makeColor(col, a, 0.55f));
        g.FillEllipse(&core, px - size * 0.45f, py - size * 0.45f, size * 0.9f, size * 0.9f);
        SolidBrush hot(Color(alphaByte(a * 0.9f), 255, 255, 235));
        g.FillEllipse(&hot, px - size * 0.22f, py - size * 0.22f, size * 0.44f, size * 0.44f);
    } break;

    case TRAIL_NOTE: {
        const float px = x + std::sin(u * kPi * 2.0f + phase) * 9.0f * sc;
        const float py = y - 32.0f * sc * easeOutPow(u, 1.6f);
        const float size = (2.5f + 1.3f * r2) * sc;
        const float pk = (std::min)(1.0f, u / 0.25f);
        const float scale = easeOutPow(pk, 2.0f) * (1.0f + 0.18f * std::sin(pk * kPi));
        const float a = A * clamp01(u * 8.0f) * (1.0f - powSafe(u, 3.0f));
        g.TranslateTransform(px, py);
        g.RotateTransform((r4 - 0.5f) * 30.0f + std::sin(u * kPi * 2.0f + phase) * 8.0f);
        g.ScaleTransform(scale, scale);
        drawMusicNote(g, size, a, hsvShade(col, 0.3f * r4 - 0.12f), r3 > 0.7f);
        g.ResetTransform();
    } break;

    case TRAIL_EMBER: {
        const float px = x + (r4 - 0.5f) * 20.0f * sc * u + std::sin(u * kPi * 4.0f + phase) * 4.0f * sc;
        const float py = y - 40.0f * sc * easeOutPow(u, 1.3f);
        const float size = (2.6f + 2.4f * r2) * sc * (1.0f - 0.6f * u);
        const float a = A * clamp01(u * 12.0f) * powSafe(1.0f - u, 0.9f);
        softGlow(g, px, py, size * 2.0f, col, a * 0.40f, 0.0f);
        SolidBrush core(makeColor(col, a, 0.35f));
        g.FillEllipse(&core, px - size * 0.65f, py - size * 0.65f, size * 1.3f, size * 1.3f);
        SolidBrush hot(Color(alphaByte(a * 0.9f), 255, 245, 210));
        g.FillEllipse(&hot, px - size * 0.32f, py - size * 0.32f, size * 0.64f, size * 0.64f);
    } break;

    case TRAIL_GLITTER: {
        const float px = x + std::sin(u * kPi * 2.5f + phase) * 8.0f * sc * u;
        const float py = y + 26.0f * sc * u * u + 4.0f * sc * u;
        const float w = (3.0f + 2.2f * r2) * sc;
        const float tw = 0.25f + 0.75f * std::fabs(std::cos(u * kPi * 5.0f + phase));
        const float a = A * clamp01(u * 10.0f) * (1.0f - powSafe(u, 2.5f)) * (0.6f + 0.4f * tw);
        g.TranslateTransform(px, py);
        g.RotateTransform((phase + (r4 - 0.5f) * 6.0f * u) * 180.0f / kPi);
        g.ScaleTransform(1.0f, 0.25f + 0.75f * tw);
        SolidBrush b(makeColor(hsvColor(r4, 0.70f, 1.0f), a));
        g.FillRectangle(&b, -w * 0.5f, -w * 0.9f, w, w * 1.8f);
        g.ResetTransform();
    } break;

    default: break;
    }
}

// 画整条拖尾：粒子样式逐点放出；彩虹样式画成颜色渐变的彩带
void drawTrailParticles(Graphics& g, const std::vector<TrailPoint>& pts, int style, float durationMs, float width,
                        float A, COLORREF col, int vx, int vy, double nowMs) {
    const float sc = (std::max)(0.4f, width / 8.0f);
    const float dur = (std::max)(100.0f, durationMs);

    if (style == TRAIL_RAINBOW) {
        for (size_t i = 1; i < pts.size(); ++i) {
            const TrailPoint& p0 = pts[i - 1];
            const TrailPoint& p1 = pts[i];
            const float u = static_cast<float>((nowMs - p1.timeMs) / dur);
            if (u >= 1.0f) continue;
            const float uu = clamp01(u);
            const float w = (std::max)(1.5f, width * 1.5f * (1.0f - 0.65f * uu));
            const float al = A * powSafe(1.0f - uu, 0.9f);
            const COLORREF c = hsvColor(uu * 0.85f, 0.85f, 1.0f);
            const float x0 = static_cast<float>(p0.screenPt.x - vx), y0 = static_cast<float>(p0.screenPt.y - vy);
            const float x1 = static_cast<float>(p1.screenPt.x - vx), y1 = static_cast<float>(p1.screenPt.y - vy);
            Pen glow(makeColor(c, al * 0.25f), w * 2.6f);
            glow.SetStartCap(LineCapRound); glow.SetEndCap(LineCapRound);
            g.DrawLine(&glow, x0, y0, x1, y1);
            Pen pen(makeColor(c, al), w);
            pen.SetStartCap(LineCapRound); pen.SetEndCap(LineCapRound);
            g.DrawLine(&pen, x0, y0, x1, y1);
        }
        return;
    }

    for (const TrailPoint& p : pts) {
        if (!p.emit) continue;
        const float u = static_cast<float>((nowMs - p.timeMs) / dur);
        if (u < 0.0f || u >= 1.0f) continue;
        drawTrailParticle(g, style, static_cast<float>(p.screenPt.x - vx), static_cast<float>(p.screenPt.y - vy),
                          u, sc, A, col, p.seed);
    }
}

// 统一入口：根据当前选中的特效类型，画出一次点击的这一帧
void drawClickEffect(Graphics& g, int effect, float cx, float cy, float t, float R, float A,
                     COLORREF col, unsigned seed, int density) {
    switch (effect) {
    case EFFECT_STARS:     drawFxStars(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_LIGHTNING: drawFxLightning(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_PETALS:    drawFxPetals(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_INK:       drawFxInk(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_FIREWORK:  drawFxFirework(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_BUBBLES:   drawFxBubbles(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_HEARTS:    drawFxHearts(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_SNOW:      drawFxSnow(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_LEAVES:    drawFxLeaves(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_PAWS:      drawFxPaws(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_COINS:     drawFxCoins(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_CONFETTI:  drawFxConfetti(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_METEOR:    drawFxMeteor(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_BUTTERFLY: drawFxButterflies(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_FIREFLY:   drawFxFireflies(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_NOTES:     drawFxNotes(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_STARBURST: drawFxStarBurst(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_RAIN:      drawFxRain(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_MAGIC:     drawFxMagic(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_COMIC:     drawFxComic(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_RAINBOW:   drawFxRainbow(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_BALLOONS:  drawFxBalloons(g, cx, cy, t, R, A, col, seed, density); break;
    case EFFECT_FLAMES:    drawFxFlames(g, cx, cy, t, R, A, col, seed, density); break;
    default: break;
    }
}

// -------------------------------------------------------------
// Click Ripple Drawing Animation (Wacom & Water Ripple with Gradients)
// -------------------------------------------------------------
void drawRipple(Graphics& g, const Ripple& ripple, ULONGLONG now) {
    const float duration = static_cast<float>((std::max)(100, g_config.durationMs));
    const float elapsed = static_cast<float>(now - ripple.startedAt);
    const float t = clamp01(elapsed / duration);

    COLORREF colorRef = g_config.leftColor;
    if (ripple.buttonType == 1) {
        colorRef = g_config.rightColor;
    } else if (ripple.buttonType == 2) {
        colorRef = g_config.middleColor;
    }

    const BYTE r = GetRValue(colorRef);
    const BYTE gg = GetGValue(colorRef);
    const BYTE b = GetBValue(colorRef);

    const float cx = static_cast<float>(ripple.screenPt.x - g_virtualX);
    const float cy = static_cast<float>(ripple.screenPt.y - g_virtualY);

    const float maxR = static_cast<float>(g_config.maxRadius);
    const float baseThickness = static_cast<float>(g_config.ringThickness);
    const float maxAlpha = static_cast<float>(g_config.maxAlpha);

    // 1. Center tactile micro-dot (soft radial gradient flash)
    if (g_config.centerDot && t < 0.22f) {
        const float q = t / 0.22f;
        const float dotR = 2.0f + 4.0f * (1.0f - std::pow(1.0f - q, 2.0f));
        const float dotA = (maxAlpha * 0.50f) * std::pow(1.0f - q, 1.5f);
        GraphicsPath dotPath;
        dotPath.AddEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
        PathGradientBrush dotBrush(&dotPath);
        dotBrush.SetCenterPoint(PointF(cx, cy));
        dotBrush.SetCenterColor(Color(alphaByte(dotA), r, gg, b));
        Color outCol(0, r, gg, b);
        int cnt = 1;
        dotBrush.SetSurroundColors(&outCol, &cnt);
        g.FillEllipse(&dotBrush, cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
    }

    // 2. Wave 1: Primary wide water ripple (Toroidal soft wave)
    const float e1 = 1.0f - std::pow(1.0f - t, 2.8f);
    const float startR = (std::min)(5.0f, maxR * 0.18f);
    const float r1 = startR + (maxR - startR) * e1;
    const float a1 = maxAlpha * std::pow(1.0f - t, 1.25f);
    const float band1 = (std::max)(8.0f, baseThickness * 3.2f + 12.0f * t);

    drawSoftToroidalWave(g, cx, cy, r1, band1, a1, r, gg, b);

    // 3. Multi-ring harmonic waves
    if (g_config.ringCount >= 2 && t > 0.14f) {
        const float q2 = clamp01((t - 0.14f) / 0.86f);
        const float e2 = 1.0f - std::pow(1.0f - q2, 2.6f);
        const float r2 = startR + (maxR * 0.72f - startR) * e2;
        const float a2 = (maxAlpha * 0.55f) * std::pow(1.0f - q2, 1.4f);
        const float band2 = (std::max)(7.0f, band1 * 0.85f);
        drawSoftToroidalWave(g, cx, cy, r2, band2, a2, r, gg, b);
    }

    if (g_config.ringCount >= 3 && t > 0.28f) {
        const float q3 = clamp01((t - 0.28f) / 0.72f);
        const float e3 = 1.0f - std::pow(1.0f - q3, 2.2f);
        const float r3 = startR + (maxR * 0.48f - startR) * e3;
        const float a3 = (maxAlpha * 0.35f) * std::pow(1.0f - q3, 1.5f);
        const float band3 = (std::max)(6.0f, band1 * 0.70f);
        drawSoftToroidalWave(g, cx, cy, r3, band3, a3, r, gg, b);
    }
}

void renderOverlay(HWND hwnd) {
    updateVirtualDesktopMetrics();
    if (g_virtualW <= 0 || g_virtualH <= 0) return;

    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_virtualW;
    bmi.bmiHeader.biHeight = -g_virtualH; // top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib) {
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(memDC, dib);
    if (bits) {
        ZeroMemory(bits, static_cast<SIZE_T>(g_virtualW) * static_cast<SIZE_T>(g_virtualH) * 4u);
    }

    {
        Graphics graphics(memDC);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        graphics.SetCompositingMode(CompositingModeSourceOver);

        const double nowMs = getHighPrecisionMs();
        const ULONGLONG now = GetTickCount64();

        // 1. Draw ink ribbon trail if enabled
        if (g_config.trailEnabled && g_trailPoints.size() >= 2) {
            if (g_config.trailStyle == TRAIL_RIBBON) {
                drawSmoothInkRibbon(graphics, g_trailPoints,
                                    static_cast<float>(g_config.trailDurationMs),
                                    static_cast<float>(g_config.trailWidth),
                                    g_config.trailAlpha,
                                    g_config.trailColor,
                                    g_virtualX, g_virtualY, nowMs);
            } else {
                drawTrailParticles(graphics, g_trailPoints, g_config.trailStyle,
                                   static_cast<float>(g_config.trailDurationMs),
                                   static_cast<float>(g_config.trailWidth),
                                   static_cast<float>(g_config.trailAlpha),
                                   g_config.trailColor,
                                   g_virtualX, g_virtualY, nowMs);
            }
        }

        // 2. Draw ambient continuous gradient water ripple if enabled
        if (g_config.ambientRipple) {
            drawAmbientRipple(graphics, now);
        }

        // 3. Draw click ripples
        for (const auto& ripple : g_ripples) {
            if (g_config.clickEffect == EFFECT_RIPPLE) {
                drawRipple(graphics, ripple, now);
            } else {
                COLORREF fxColor = g_config.leftColor;
                if (ripple.buttonType == 1) fxColor = g_config.rightColor;
                else if (ripple.buttonType == 2) fxColor = g_config.middleColor;
                const float fxDur = static_cast<float>((std::max)(100, g_config.durationMs));
                const float fxT = clamp01(static_cast<float>(now - ripple.startedAt) / fxDur);
                drawClickEffect(graphics, g_config.clickEffect,
                                static_cast<float>(ripple.screenPt.x - g_virtualX),
                                static_cast<float>(ripple.screenPt.y - g_virtualY),
                                fxT, static_cast<float>(g_config.maxRadius),
                                static_cast<float>(g_config.maxAlpha), fxColor,
                                ripple.seed, g_config.ringCount);
            }
        }

        // 4. Draw teaching annotations (Live ink brush & quick arrows)
        for (const auto& stroke : g_annotations) {
            if (stroke.type == ANNOTATION_INK) {
                drawAnnotationInk(graphics, stroke, nowMs, g_virtualX, g_virtualY);
            } else if (stroke.type == ANNOTATION_ARROW) {
                drawQuickArrow(graphics, stroke, nowMs, g_virtualX, g_virtualY);
            }
        }
    }

    POINT dst{g_virtualX, g_virtualY};
    SIZE size{g_virtualW, g_virtualH};
    POINT src{0, 0};
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(hwnd, screenDC, &dst, &size, memDC, &src, 0, &blend, ULW_ALPHA);

    SelectObject(memDC, oldBitmap);
    DeleteObject(dib);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}

void addRipple(POINT screenPt, int buttonType) {
    Ripple ripple;
    ripple.screenPt = screenPt;
    ripple.buttonType = buttonType;
    ripple.startedAt = GetTickCount64();
    ripple.seed = static_cast<unsigned>(ripple.startedAt * 2654435761ull) ^
                  (static_cast<unsigned>(screenPt.x) * 73856093u) ^
                  (static_cast<unsigned>(screenPt.y) * 19349663u);
    g_ripples.push_back(ripple);

    if (g_ripples.size() > 64) {
        g_ripples.erase(g_ripples.begin(), g_ripples.begin() + (g_ripples.size() - 64));
    }
}

LRESULT CALLBACK lowLevelMouseProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && g_overlay) {
        const auto* info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        const bool isCtrlAlt = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) &&
                               ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);

        if (g_config.annotationEnabled) {
            // 1. Start ink drawing with Ctrl+Alt + Left Button Down
            if (wParam == WM_LBUTTONDOWN && isCtrlAlt) {
                g_isDrawingInk = true;
                AnnotationStroke s;
                s.type = ANNOTATION_INK;
                s.points.push_back(info->pt);
                s.isDrawing = true;
                s.startedAt = getHighPrecisionMs();
                s.color = g_config.inkColor;
                s.width = static_cast<float>(g_config.inkWidth);
                s.holdMs = static_cast<float>(g_config.annotationHoldMs);
                s.fadeMs = 800.0f;
                g_annotations.push_back(s);
                return 1; // Intercept: do not click underlying apps or desktop
            }

            // 2. Start arrow drawing with Ctrl+Alt + Right Button Down
            if (wParam == WM_RBUTTONDOWN && isCtrlAlt) {
                g_isDrawingArrow = true;
                AnnotationStroke s;
                s.type = ANNOTATION_ARROW;
                s.startPt = info->pt;
                s.endPt = info->pt;
                s.isDrawing = true;
                s.startedAt = getHighPrecisionMs();
                s.color = g_config.arrowColor;
                s.width = static_cast<float>(g_config.arrowWidth);
                s.holdMs = static_cast<float>(g_config.annotationHoldMs);
                s.fadeMs = 800.0f;
                g_annotations.push_back(s);
                return 1; // Intercept: do not show context menu
            }

            // 3. Mouse move while drawing
            // CRITICAL: NEVER return 1 on WM_MOUSEMOVE! Returning 1 freezes the hardware cursor!
            // NEVER call heavy renderOverlay synchronously inside low-level hook! WM_TIMER renders at 83 FPS!
            if (wParam == WM_MOUSEMOVE) {
                if (g_isDrawingInk) {
                    for (auto it = g_annotations.rbegin(); it != g_annotations.rend(); ++it) {
                        if (it->isDrawing && it->type == ANNOTATION_INK) {
                            if (it->points.empty()) {
                                it->points.push_back(info->pt);
                            } else {
                                const auto& last = it->points.back();
                                const int dx = info->pt.x - last.x;
                                const int dy = info->pt.y - last.y;
                                if (dx * dx + dy * dy >= 9) { // At least 3px move for clean sampling
                                    it->points.push_back(info->pt);
                                }
                            }
                            break;
                        }
                    }
                } else if (g_isDrawingArrow) {
                    for (auto it = g_annotations.rbegin(); it != g_annotations.rend(); ++it) {
                        if (it->isDrawing && it->type == ANNOTATION_ARROW) {
                            it->endPt = info->pt;
                            break;
                        }
                    }
                }
            }

            // 4. Mouse button up finishes drawing and triggers auto-fade countdown
            if (wParam == WM_LBUTTONUP && g_isDrawingInk) {
                g_isDrawingInk = false;
                const double nowMs = getHighPrecisionMs();
                for (auto it = g_annotations.rbegin(); it != g_annotations.rend(); ++it) {
                    if (it->isDrawing && it->type == ANNOTATION_INK) {
                        it->isDrawing = false;
                        it->releasedAt = nowMs;
                        break;
                    }
                }
                return 1; // Intercept button up
            }

            if (wParam == WM_RBUTTONUP && g_isDrawingArrow) {
                g_isDrawingArrow = false;
                const double nowMs = getHighPrecisionMs();
                for (auto it = g_annotations.rbegin(); it != g_annotations.rend(); ++it) {
                    if (it->isDrawing && it->type == ANNOTATION_ARROW) {
                        it->isDrawing = false;
                        it->releasedAt = nowMs;
                        break;
                    }
                }
                return 1; // Intercept right button up
            }
        }

        if (wParam == WM_MOUSEMOVE) {
            if (g_config.trailEnabled && !g_isDrawingInk && !g_isDrawingArrow) {
                const double nowMs = getHighPrecisionMs();
                const float trailSpacing = 6.0f + static_cast<float>(g_config.trailWidth) * 1.3f;   // 粒子间距随“拖尾粗细”变化
                if (g_trailPoints.empty()) {
                    trailPushPoint(g_trailPoints, info->pt, nowMs, trailSpacing);
                } else {
                    const auto& last = g_trailPoints.back();
                    const int dx = info->pt.x - last.screenPt.x;
                    const int dy = info->pt.y - last.screenPt.y;
                    if (dx * dx + dy * dy >= 2) { // 位移至少 ~1.4px，精确保留平滑弧线同时过滤原地重复事件
                        trailPushPoint(g_trailPoints, info->pt, nowMs, trailSpacing);
                        if (g_trailPoints.size() > 1500) {   // 粒子拖尾存活更久，需要保留更多轨迹点
                            g_trailPoints.erase(g_trailPoints.begin(), g_trailPoints.begin() + 100);
                        }
                    }
                }
            }
        } else {
            int btn = -1;
            if (wParam == WM_LBUTTONDOWN) btn = 0;
            else if (wParam == WM_RBUTTONDOWN) btn = 1;
            else if (wParam == WM_MBUTTONDOWN) btn = 2;

            if (btn >= 0) {
                POINT* p = new POINT(info->pt);
                PostMessage(g_overlay, WM_RIPPLE_CLICK, static_cast<WPARAM>(btn), reinterpret_cast<LPARAM>(p));
            }
        }
    }
    return CallNextHookEx(g_mouseHook, code, wParam, lParam);
}

LRESULT CALLBACK lowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            const auto* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
            if (kbd->vkCode == VK_ESCAPE) {
                if (!g_annotations.empty() || g_isDrawingInk || g_isDrawingArrow) {
                    g_annotations.clear();
                    g_isDrawingInk = false;
                    g_isDrawingArrow = false;
                    if (g_overlay) {
                        renderOverlay(g_overlay);
                    }
                }
            }
        }
    }
    return CallNextHookEx(g_kbdHook, code, wParam, lParam);
}

void addTrayIcon(HWND hwnd) {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
    nid.uCallbackMessage = WM_TRAYICON;
    HICON hCustomIcon = (HICON)LoadImageW(g_instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                          GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    nid.hIcon = hCustomIcon ? hCustomIcon : LoadIcon(nullptr, IDI_APPLICATION);
    lstrcpynW(nid.szTip, L"MouseRipple - 鼠标水波纹 (双击打开设置)", ARRAYSIZE(nid.szTip));
    lstrcpynW(nid.szInfoTitle, L"MouseRipple 水波纹已运行", ARRAYSIZE(nid.szInfoTitle));
    lstrcpynW(nid.szInfo, L"鼠标水波纹已生效。双击任务栏托盘图标可调整设置。", ARRAYSIZE(nid.szInfo));
    nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void removeTrayIcon(HWND hwnd) {
    NOTIFYICONDATA nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void openSettingsWindow();

void showTrayMenu(HWND hwnd) {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    AppendMenu(menu, MF_STRING, ID_TRAY_SETTINGS, L"设置 (Settings)...");
    SetMenuDefaultItem(menu, ID_TRAY_SETTINGS, FALSE);
    AppendMenu(menu, MF_SEPARATOR, 0, nullptr);

    const UINT autostartFlag = isAutoStartEnabled() ? (MF_STRING | MF_CHECKED) : (MF_STRING | MF_UNCHECKED);
    AppendMenu(menu, autostartFlag, ID_TRAY_AUTOSTART, L"开机自启动 (Start with Windows)");
    AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(menu, MF_STRING, ID_TRAY_EXIT, L"退出 (Exit)");

    POINT pt{};
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                   pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(menu);
}

// -------------------------------------------------------------
// Settings Window Implementation
// -------------------------------------------------------------

void updateSettingsLabels(HWND hwnd) {
    std::wostringstream ssR;
    ssR << L"最大半径: " << g_config.maxRadius << L" px";
    if (g_config.maxRadius <= 32) ssR << L" (Wacom细腻)";
    SetDlgItemTextW(hwnd, IDC_LBL_RADIUS, ssR.str().c_str());

    std::wostringstream ssD;
    ssD << L"动画时长: " << g_config.durationMs << L" ms";
    if (g_config.durationMs <= 320) ssD << L" (灵敏敏捷)";
    SetDlgItemTextW(hwnd, IDC_LBL_DURATION, ssD.str().c_str());

    std::wostringstream ssT;
    ssT << L"线条粗细: " << g_config.ringThickness << L" px";
    SetDlgItemTextW(hwnd, IDC_LBL_THICKNESS, ssT.str().c_str());

    std::wostringstream ssC;
    if (g_config.clickEffect == EFFECT_RIPPLE) {
        ssC << L"波纹圈数: " << g_config.ringCount << L" 圈";
        if (g_config.ringCount == 1) ssC << L" (纯净单圈)";
        else if (g_config.ringCount >= 3) ssC << L" (水波涟漪)";
    } else {
        ssC << L"特效数量: " << g_config.ringCount << L" 档";
        if (g_config.ringCount == 1) ssC << L" (疏朗)";
        else if (g_config.ringCount >= 3) ssC << L" (繁盛)";
    }
    SetDlgItemTextW(hwnd, IDC_LBL_RINGS, ssC.str().c_str());

    std::wostringstream ssA;
    int alphaPercent = static_cast<int>(g_config.maxAlpha * 100.0f / 255.0f + 0.5f);
    ssA << L"波纹透明度: " << alphaPercent << L" %";
    if (alphaPercent <= 30) ssA << L" (清淡半透)";
    else if (alphaPercent >= 85) ssA << L" (鲜明醒目)";
    SetDlgItemTextW(hwnd, IDC_LBL_ALPHA, ssA.str().c_str());

    // Ambient ripple labels
    std::wostringstream ssAR;
    ssAR << L"常驻线条半径: " << g_config.ambientRadius << L" px";
    SetDlgItemTextW(hwnd, IDC_LBL_AMBIENT_RADIUS, ssAR.str().c_str());

    std::wostringstream ssAA;
    ssAA << L"常驻透明度: " << g_config.ambientAlphaPercent << L" %";
    if (g_config.ambientAlphaPercent == 0) ssAA << L" (0% 完全隐藏)";
    else if (g_config.ambientAlphaPercent <= 30) ssAA << L" (清爽低调)";
    else if (g_config.ambientAlphaPercent >= 90) ssAA << L" (极度醒目)";
    SetDlgItemTextW(hwnd, IDC_LBL_AMBIENT_ALPHA, ssAA.str().c_str());

    std::wostringstream ssAB;
    ssAB << L"常驻线条粗细: " << g_config.ambientThickness << L" px";
    if (g_config.ambientThickness == 1) ssAB << L" (极细简约)";
    else if (g_config.ambientThickness == 2) ssAB << L" (细腻推荐)";
    SetDlgItemTextW(hwnd, IDC_LBL_AMBIENT_BAND, ssAB.str().c_str());

    // Ink ribbon trail labels (书法水墨流线拖尾)
    std::wostringstream ssTD;
    ssTD << L"拖尾留存: " << g_config.trailDurationMs << L" ms";
    if (g_config.trailDurationMs <= 250) ssTD << L" (短促灵敏)";
    else if (g_config.trailDurationMs >= 500) ssTD << L" (行云流水)";
    SetDlgItemTextW(hwnd, IDC_LBL_TRAIL_DURATION, ssTD.str().c_str());

    std::wostringstream ssTW;
    ssTW << L"拖尾粗细: " << g_config.trailWidth << L" px";
    if (g_config.trailWidth <= 5) ssTW << L" (纤细柔和)";
    else if (g_config.trailWidth >= 12) ssTW << L" (浓墨粗犷)";
    SetDlgItemTextW(hwnd, IDC_LBL_TRAIL_WIDTH, ssTW.str().c_str());

    std::wostringstream ssTA;
    ssTA << L"拖尾透明度: " << g_config.trailAlphaPercent << L" %";
    if (g_config.trailAlphaPercent <= 40) ssTA << L" (淡雅水墨)";
    else if (g_config.trailAlphaPercent >= 80) ssTA << L" (饱满醒目)";
    SetDlgItemTextW(hwnd, IDC_LBL_TRAIL_ALPHA, ssTA.str().c_str());

    // Teaching annotation labels
    std::wostringstream ssIW;
    ssIW << L"标注线条粗细: " << g_config.inkWidth << L" px";
    SetDlgItemTextW(hwnd, IDC_LBL_INK_WIDTH, ssIW.str().c_str());

    std::wostringstream ssAH;
    ssAH << L"停留展示时长: " << g_config.annotationHoldMs << L" ms ("
         << std::fixed << std::setprecision(1) << (g_config.annotationHoldMs / 1000.0f) << L" 秒后自动淡出)";
    SetDlgItemTextW(hwnd, IDC_LBL_ANNOTATION_HOLD, ssAH.str().c_str());
}

void syncSettingsControls(HWND hwnd) {
    SendDlgItemMessageW(hwnd, IDC_PRESET_COMBO, CB_SETCURSEL, presetToComboIndex(g_config.stylePreset), 0);

    SendDlgItemMessageW(hwnd, IDC_TRK_RADIUS, TBM_SETPOS, TRUE, g_config.maxRadius);
    SendDlgItemMessageW(hwnd, IDC_TRK_DURATION, TBM_SETPOS, TRUE, g_config.durationMs);
    SendDlgItemMessageW(hwnd, IDC_TRK_THICKNESS, TBM_SETPOS, TRUE, g_config.ringThickness);
    SendDlgItemMessageW(hwnd, IDC_TRK_RINGS, TBM_SETPOS, TRUE, g_config.ringCount);

    int alphaPercent = static_cast<int>(g_config.maxAlpha * 100.0f / 255.0f + 0.5f);
    SendDlgItemMessageW(hwnd, IDC_TRK_ALPHA, TBM_SETPOS, TRUE, alphaPercent);

    CheckDlgButton(hwnd, IDC_CHK_CENTERDOT, g_config.centerDot ? BST_CHECKED : BST_UNCHECKED);

    // Ambient ripple controls sync
    CheckDlgButton(hwnd, IDC_CHK_AMBIENT, g_config.ambientRipple ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessageW(hwnd, IDC_TRK_AMBIENT_RADIUS, TBM_SETPOS, TRUE, g_config.ambientRadius);
    SendDlgItemMessageW(hwnd, IDC_TRK_AMBIENT_ALPHA, TBM_SETPOS, TRUE, g_config.ambientAlphaPercent);
    SendDlgItemMessageW(hwnd, IDC_TRK_AMBIENT_BAND, TBM_SETPOS, TRUE, g_config.ambientThickness);

    // Trail controls sync
    CheckDlgButton(hwnd, IDC_CHK_TRAIL, g_config.trailEnabled ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessageW(hwnd, IDC_TRK_TRAIL_DURATION, TBM_SETPOS, TRUE, g_config.trailDurationMs);
    SendDlgItemMessageW(hwnd, IDC_TRK_TRAIL_WIDTH, TBM_SETPOS, TRUE, g_config.trailWidth);
    SendDlgItemMessageW(hwnd, IDC_TRK_TRAIL_ALPHA, TBM_SETPOS, TRUE, g_config.trailAlphaPercent);

    // Teaching annotations controls sync
    CheckDlgButton(hwnd, IDC_CHK_ANNOTATION, g_config.annotationEnabled ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessageW(hwnd, IDC_TRK_INK_WIDTH, TBM_SETPOS, TRUE, g_config.inkWidth);
    SendDlgItemMessageW(hwnd, IDC_TRK_ANNOTATION_HOLD, TBM_SETPOS, TRUE, g_config.annotationHoldMs);

    updateSettingsLabels(hwnd);

    // Invalidate color preview buttons
    InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_LEFT), nullptr, TRUE);
    InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_RIGHT), nullptr, TRUE);
    InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_MID), nullptr, TRUE);
    InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_AMBIENT), nullptr, TRUE);
    InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_TRAIL), nullptr, TRUE);
    InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_INK), nullptr, TRUE);
    InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_ARROW), nullptr, TRUE);
}

bool pickColor(HWND owner, COLORREF& targetColor) {
    CHOOSECOLOR cc{};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = owner;
    cc.rgbResult = targetColor;
    cc.lpCustColors = g_customColors;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
    if (ChooseColorW(&cc)) {
        targetColor = cc.rgbResult;
        return true;
    }
    return false;
}

LRESULT CALLBACK settingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        NONCLIENTMETRICS ncm{};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        g_uiFont = CreateFontIndirect(&ncm.lfMessageFont);
        if (!g_uiFont) {
            g_uiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }

        // ==========================================
        // LEFT COLUMN (x = 15..405, w = 390)
        // ==========================================

        // 1. Preset GroupBox
        CreateWindowExW(0, L"BUTTON", L" 预设风格 (Preset Style) ",
                        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                        15, 10, 390, 58, hwnd, nullptr, g_instance, nullptr);

        HWND hCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                      WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                                      30, 30, 360, 640, hwnd, (HMENU)(INT_PTR)IDC_PRESET_COMBO, g_instance, nullptr);
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"✨ Wacom 细腻笔触风格 (推荐: 小巧/灵动/触点)");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"🌊 经典水波涟漪风格 (大半径/多层扩散/悠长)");
        for (int i = 0; i < kThemeCount; ++i) {
            SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)kThemes[i].name);
        }
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"🛠️ 自定义参数 (自由调节)");
        SendMessageW(hCombo, CB_SETMINVISIBLE, 64, 0);   // 尽量一次显示全部预设（屏幕放不下时自动截断）

        // 2. Click Ripples GroupBox
        CreateWindowExW(0, L"BUTTON", L" 点击波纹色彩与动态 ",
                        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                        15, 74, 390, 258, hwnd, nullptr, g_instance, nullptr);

        CreateWindowExW(0, L"STATIC", L"左键:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        30, 94, 38, 20, hwnd, nullptr, g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                        70, 92, 36, 22, hwnd, (HMENU)(INT_PTR)IDC_BTN_COLOR_LEFT, g_instance, nullptr);

        CreateWindowExW(0, L"STATIC", L"右键:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        130, 94, 38, 20, hwnd, nullptr, g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                        170, 92, 36, 22, hwnd, (HMENU)(INT_PTR)IDC_BTN_COLOR_RIGHT, g_instance, nullptr);

        CreateWindowExW(0, L"STATIC", L"中键:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        230, 94, 38, 20, hwnd, nullptr, g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                        270, 92, 36, 22, hwnd, (HMENU)(INT_PTR)IDC_BTN_COLOR_MID, g_instance, nullptr);

        // Max Radius Trackbar (15 - 90 px)
        CreateWindowExW(0, L"STATIC", L"最大半径:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        30, 120, 220, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_RADIUS, g_instance, nullptr);
        HWND hTrkRadius = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                          WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                          25, 138, 365, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_RADIUS, g_instance, nullptr);
        SendMessageW(hTrkRadius, TBM_SETRANGE, TRUE, MAKELPARAM(15, 90));
        SendMessageW(hTrkRadius, TBM_SETTICFREQ, 5, 0);

        // Duration Trackbar (150 - 750 ms)
        CreateWindowExW(0, L"STATIC", L"动画时长:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        30, 168, 220, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_DURATION, g_instance, nullptr);
        HWND hTrkDuration = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                            25, 186, 365, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_DURATION, g_instance, nullptr);
        SendMessageW(hTrkDuration, TBM_SETRANGE, TRUE, MAKELPARAM(150, 1500));
        SendMessageW(hTrkDuration, TBM_SETTICFREQ, 50, 0);

        // Thickness Trackbar (1 - 5 px)
        CreateWindowExW(0, L"STATIC", L"线条粗细:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        30, 216, 170, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_THICKNESS, g_instance, nullptr);
        HWND hTrkThickness = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                             WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                             25, 234, 175, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_THICKNESS, g_instance, nullptr);
        SendMessageW(hTrkThickness, TBM_SETRANGE, TRUE, MAKELPARAM(1, 5));

        // Rings Trackbar (1 - 3)
        CreateWindowExW(0, L"STATIC", L"波纹圈数:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        215, 216, 170, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_RINGS, g_instance, nullptr);
        HWND hTrkRings = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                         WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                         210, 234, 180, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_RINGS, g_instance, nullptr);
        SendMessageW(hTrkRings, TBM_SETRANGE, TRUE, MAKELPARAM(1, 3));

        // Click Ripple Opacity Trackbar (10 - 100 %)
        CreateWindowExW(0, L"STATIC", L"波纹透明度:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        30, 264, 170, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_ALPHA, g_instance, nullptr);
        HWND hTrkAlpha = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                         WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                         25, 282, 175, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_ALPHA, g_instance, nullptr);
        SendMessageW(hTrkAlpha, TBM_SETRANGE, TRUE, MAKELPARAM(10, 100));
        SendMessageW(hTrkAlpha, TBM_SETTICFREQ, 10, 0);

        // Center dot checkbox (落笔触感微点)
        CreateWindowExW(0, L"BUTTON", L"开启落笔触点 (Wacom)",
                        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                        215, 284, 180, 24, hwnd, (HMENU)(INT_PTR)IDC_CHK_CENTERDOT, g_instance, nullptr);

        // 3. Ambient Continuous Ripple GroupBox (常驻鼠标动态线条圈)
        CreateWindowExW(0, L"BUTTON", L" 常驻鼠标动态线条圈 (单圈纯净呼吸) ",
                        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                        15, 338, 390, 200, hwnd, nullptr, g_instance, nullptr);

        CreateWindowExW(0, L"BUTTON", L"开启常驻动态线条圈",
                        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                        30, 358, 260, 22, hwnd, (HMENU)(INT_PTR)IDC_CHK_AMBIENT, g_instance, nullptr);

        CreateWindowExW(0, L"STATIC", L"颜色:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        300, 360, 38, 20, hwnd, nullptr, g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                        340, 358, 38, 22, hwnd, (HMENU)(INT_PTR)IDC_BTN_COLOR_AMBIENT, g_instance, nullptr);

        CreateWindowExW(0, L"STATIC", L"常驻线条半径:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        30, 384, 170, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_AMBIENT_RADIUS, g_instance, nullptr);
        HWND hTrkAmbientR = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                            25, 402, 175, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_AMBIENT_RADIUS, g_instance, nullptr);
        SendMessageW(hTrkAmbientR, TBM_SETRANGE, TRUE, MAKELPARAM(10, 45));
        SendMessageW(hTrkAmbientR, TBM_SETTICFREQ, 5, 0);

        CreateWindowExW(0, L"STATIC", L"常驻透明度:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        215, 384, 170, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_AMBIENT_ALPHA, g_instance, nullptr);
        HWND hTrkAmbientA = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                            210, 402, 180, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_AMBIENT_ALPHA, g_instance, nullptr);
        SendMessageW(hTrkAmbientA, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        SendMessageW(hTrkAmbientA, TBM_SETTICFREQ, 10, 0);

        CreateWindowExW(0, L"STATIC", L"常驻线条粗细:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        30, 434, 360, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_AMBIENT_BAND, g_instance, nullptr);
        HWND hTrkAmbientB = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                            25, 452, 365, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_AMBIENT_BAND, g_instance, nullptr);
        SendMessageW(hTrkAmbientB, TBM_SETRANGE, TRUE, MAKELPARAM(1, 6));
        SendMessageW(hTrkAmbientB, TBM_SETTICFREQ, 1, 0);

        CreateWindowExW(0, L"STATIC", L"💡 常驻线条单圈纯净呼吸，透明度可调至0%完全隐藏",
                        WS_CHILD | WS_VISIBLE | SS_LEFT,
                        28, 488, 365, 36, hwnd, nullptr, g_instance, nullptr);

        // ==========================================
        // RIGHT COLUMN (x = 420..810, w = 390)
        // ==========================================

        // 4. Ink Ribbon Trail GroupBox (鼠标移动水墨流线拖尾)
        CreateWindowExW(0, L"BUTTON", L" 鼠标移动水墨流线拖尾 (书法笔触) ",
                        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                        420, 10, 390, 224, hwnd, nullptr, g_instance, nullptr);

        CreateWindowExW(0, L"BUTTON", L"开启水墨流线拖尾 (移动渐隐)",
                        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                        435, 30, 260, 22, hwnd, (HMENU)(INT_PTR)IDC_CHK_TRAIL, g_instance, nullptr);

        CreateWindowExW(0, L"STATIC", L"颜色:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        705, 32, 38, 20, hwnd, nullptr, g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                        745, 30, 38, 22, hwnd, (HMENU)(INT_PTR)IDC_BTN_COLOR_TRAIL, g_instance, nullptr);

        CreateWindowExW(0, L"STATIC", L"拖尾留存:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        435, 56, 360, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_TRAIL_DURATION, g_instance, nullptr);
        HWND hTrkTrailD = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                          WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                          430, 74, 365, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_TRAIL_DURATION, g_instance, nullptr);
        SendMessageW(hTrkTrailD, TBM_SETRANGE, TRUE, MAKELPARAM(150, 1500));
        SendMessageW(hTrkTrailD, TBM_SETTICFREQ, 50, 0);

        CreateWindowExW(0, L"STATIC", L"拖尾粗细:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        435, 104, 170, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_TRAIL_WIDTH, g_instance, nullptr);
        HWND hTrkTrailW = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                          WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                          430, 122, 175, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_TRAIL_WIDTH, g_instance, nullptr);
        SendMessageW(hTrkTrailW, TBM_SETRANGE, TRUE, MAKELPARAM(3, 18));
        SendMessageW(hTrkTrailW, TBM_SETTICFREQ, 2, 0);

        CreateWindowExW(0, L"STATIC", L"拖尾透明度:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        620, 104, 170, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_TRAIL_ALPHA, g_instance, nullptr);
        HWND hTrkTrailA = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                          WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                          615, 122, 180, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_TRAIL_ALPHA, g_instance, nullptr);
        SendMessageW(hTrkTrailA, TBM_SETRANGE, TRUE, MAKELPARAM(10, 100));
        SendMessageW(hTrkTrailA, TBM_SETTICFREQ, 10, 0);

        CreateWindowExW(0, L"STATIC", L"💡 如同书法水墨流畅挥毫，头部饱满、尾部渐细羽化、自然消散",
                        WS_CHILD | WS_VISIBLE | SS_LEFT,
                        435, 168, 365, 36, hwnd, nullptr, g_instance, nullptr);

        // 5. Teaching Annotations GroupBox (教学演示标注)
        CreateWindowExW(0, L"BUTTON", L" 教学演示标注 (阅后即焚画笔与快捷箭头) ",
                        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                        420, 240, 390, 298, hwnd, nullptr, g_instance, nullptr);

        CreateWindowExW(0, L"BUTTON", L"开启教学演示标注 (Ctrl+Alt 唤起)",
                        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                        435, 260, 350, 22, hwnd, (HMENU)(INT_PTR)IDC_CHK_ANNOTATION, g_instance, nullptr);

        CreateWindowExW(0, L"STATIC", L"画笔颜色:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        435, 288, 62, 20, hwnd, nullptr, g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                        498, 286, 38, 22, hwnd, (HMENU)(INT_PTR)IDC_BTN_COLOR_INK, g_instance, nullptr);

        CreateWindowExW(0, L"STATIC", L"箭头颜色:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        570, 288, 62, 20, hwnd, nullptr, g_instance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                        633, 286, 38, 22, hwnd, (HMENU)(INT_PTR)IDC_BTN_COLOR_ARROW, g_instance, nullptr);

        CreateWindowExW(0, L"STATIC", L"标注线条粗细:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        435, 314, 360, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_INK_WIDTH, g_instance, nullptr);
        HWND hTrkInkWidth = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                            430, 332, 365, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_INK_WIDTH, g_instance, nullptr);
        SendMessageW(hTrkInkWidth, TBM_SETRANGE, TRUE, MAKELPARAM(3, 16));
        SendMessageW(hTrkInkWidth, TBM_SETTICFREQ, 1, 0);

        CreateWindowExW(0, L"STATIC", L"停留展示时长:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        435, 364, 360, 18, hwnd, (HMENU)(INT_PTR)IDC_LBL_ANNOTATION_HOLD, g_instance, nullptr);
        HWND hTrkAnnotationHold = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                                  WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP,
                                                  430, 382, 365, 28, hwnd, (HMENU)(INT_PTR)IDC_TRK_ANNOTATION_HOLD, g_instance, nullptr);
        SendMessageW(hTrkAnnotationHold, TBM_SETRANGE, TRUE, MAKELPARAM(500, 5000));
        SendMessageW(hTrkAnnotationHold, TBM_SETTICFREQ, 500, 0);

        CreateWindowExW(0, L"STATIC",
                        L"✏️ Ctrl+Alt + 鼠标左键拖拽：自由流光画笔\n"
                        L"➡️ Ctrl+Alt + 鼠标右键拖拽：快捷箭头指引\n"
                        L"⚡ 按 Esc 键：立即清除屏幕全部标注\n"
                        L"🔥 停留数秒后自动平滑淡出，阅后即焚无需擦除！",
                        WS_CHILD | WS_VISIBLE | SS_LEFT,
                        435, 418, 365, 84, hwnd, nullptr, g_instance, nullptr);

        // ==========================================
        // BOTTOM ACTION BUTTONS
        // ==========================================
        CreateWindowExW(0, L"BUTTON", L"恢复默认设置",
                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        15, 552, 140, 32, hwnd, (HMENU)(INT_PTR)IDC_BTN_RESET, g_instance, nullptr);

        CreateWindowExW(0, L"BUTTON", L"保存并关闭",
                        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                        670, 552, 140, 32, hwnd, (HMENU)(INT_PTR)IDC_BTN_SAVE, g_instance, nullptr);

        // Apply modern font to all child controls
        EnumChildWindows(hwnd, [](HWND hChild, LPARAM lParam) -> BOOL {
            SendMessage(hChild, WM_SETFONT, (WPARAM)lParam, TRUE);
            return TRUE;
        }, (LPARAM)g_uiFont);

        syncSettingsControls(hwnd);
        return 0;
    }

    case WM_DRAWITEM: {
        const auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        COLORREF fillCol = RGB(128, 128, 128);
        if (dis->CtlID == IDC_BTN_COLOR_LEFT) fillCol = g_config.leftColor;
        else if (dis->CtlID == IDC_BTN_COLOR_RIGHT) fillCol = g_config.rightColor;
        else if (dis->CtlID == IDC_BTN_COLOR_MID) fillCol = g_config.middleColor;
        else if (dis->CtlID == IDC_BTN_COLOR_AMBIENT) fillCol = g_config.ambientColor;
        else if (dis->CtlID == IDC_BTN_COLOR_TRAIL) fillCol = g_config.trailColor;
        else if (dis->CtlID == IDC_BTN_COLOR_INK) fillCol = g_config.inkColor;
        else if (dis->CtlID == IDC_BTN_COLOR_ARROW) fillCol = g_config.arrowColor;

        HBRUSH brush = CreateSolidBrush(fillCol);
        FillRect(dis->hDC, &dis->rcItem, brush);
        DeleteObject(brush);

        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HGDIOBJ oldPen = SelectObject(dis->hDC, borderPen);
        SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
        Rectangle(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom);
        SelectObject(dis->hDC, oldPen);
        DeleteObject(borderPen);
        return TRUE;
    }

    case WM_HSCROLL: {
        HWND hTrack = reinterpret_cast<HWND>(lParam);
        const int val = (int)SendMessageW(hTrack, TBM_GETPOS, 0, 0);

        if (hTrack == GetDlgItem(hwnd, IDC_TRK_RADIUS)) {
            g_config.maxRadius = val;
            g_config.stylePreset = PRESET_CUSTOM;
            SendDlgItemMessageW(hwnd, IDC_PRESET_COMBO, CB_SETCURSEL, presetToComboIndex(PRESET_CUSTOM), 0);
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_DURATION)) {
            g_config.durationMs = val;
            g_config.stylePreset = PRESET_CUSTOM;
            SendDlgItemMessageW(hwnd, IDC_PRESET_COMBO, CB_SETCURSEL, presetToComboIndex(PRESET_CUSTOM), 0);
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_THICKNESS)) {
            g_config.ringThickness = val;
            g_config.stylePreset = PRESET_CUSTOM;
            SendDlgItemMessageW(hwnd, IDC_PRESET_COMBO, CB_SETCURSEL, presetToComboIndex(PRESET_CUSTOM), 0);
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_RINGS)) {
            g_config.ringCount = val;
            g_config.stylePreset = PRESET_CUSTOM;
            SendDlgItemMessageW(hwnd, IDC_PRESET_COMBO, CB_SETCURSEL, presetToComboIndex(PRESET_CUSTOM), 0);
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_ALPHA)) {
            g_config.maxAlpha = static_cast<int>(255.0f * (val / 100.0f));
            g_config.stylePreset = PRESET_CUSTOM;
            SendDlgItemMessageW(hwnd, IDC_PRESET_COMBO, CB_SETCURSEL, presetToComboIndex(PRESET_CUSTOM), 0);
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_AMBIENT_RADIUS)) {
            g_config.ambientRadius = val;
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_AMBIENT_ALPHA)) {
            g_config.ambientAlphaPercent = val;
            g_config.ambientAlpha = static_cast<int>(255.0f * (val / 100.0f));
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_AMBIENT_BAND)) {
            g_config.ambientThickness = val;
            g_config.ambientBand = val;
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_TRAIL_DURATION)) {
            g_config.trailDurationMs = val;
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_TRAIL_WIDTH)) {
            g_config.trailWidth = val;
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_TRAIL_ALPHA)) {
            g_config.trailAlphaPercent = val;
            g_config.trailAlpha = static_cast<int>(255.0f * (val / 100.0f));
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_INK_WIDTH)) {
            g_config.inkWidth = val;
            g_config.arrowWidth = val;
        } else if (hTrack == GetDlgItem(hwnd, IDC_TRK_ANNOTATION_HOLD)) {
            g_config.annotationHoldMs = val;
        }
        updateSettingsLabels(hwnd);
        return 0;
    }

    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        const int code = HIWORD(wParam);

        if (id == IDC_PRESET_COMBO && code == CBN_DROPDOWN) {
            HWND hCb = GetDlgItem(hwnd, IDC_PRESET_COMBO);
            const int cnt = (int)SendMessageW(hCb, CB_GETCOUNT, 0, 0);
            const int itemH = (int)SendMessageW(hCb, CB_GETITEMHEIGHT, 0, 0);
            const int fieldH = (int)SendMessageW(hCb, CB_GETITEMHEIGHT, (WPARAM)-1, 0);
            if (cnt > 0 && itemH > 0) {
                RECT rc{};
                GetWindowRect(hCb, &rc);
                MONITORINFO mi{};
                mi.cbSize = sizeof(mi);
                GetMonitorInfoW(MonitorFromWindow(hCb, MONITOR_DEFAULTTONEAREST), &mi);
                const int maxH = (mi.rcWork.bottom - mi.rcWork.top) - 16;          // 不超过屏幕可用高度
                int wantH = fieldH + itemH * cnt + 10;                              // 选择框 + 全部列表项
                if (wantH > maxH) wantH = maxH;
                SetWindowPos(hCb, nullptr, 0, 0, rc.right - rc.left, wantH,
                             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        if (id == IDC_PRESET_COMBO && code == CBN_SELCHANGE) {
            const int selIdx = (int)SendDlgItemMessageW(hwnd, IDC_PRESET_COMBO, CB_GETCURSEL, 0, 0);
            const int sel = (selIdx >= 0) ? comboIndexToPreset(selIdx) : -1;
            if (sel >= 0 && sel != PRESET_CUSTOM) {
                applyPreset(sel);
                syncSettingsControls(hwnd);
            }
            return 0;
        }

        if (id == IDC_BTN_COLOR_LEFT) {
            if (pickColor(hwnd, g_config.leftColor)) {
                InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_LEFT), nullptr, TRUE);
            }
            return 0;
        }

        if (id == IDC_BTN_COLOR_RIGHT) {
            if (pickColor(hwnd, g_config.rightColor)) {
                InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_RIGHT), nullptr, TRUE);
            }
            return 0;
        }

        if (id == IDC_BTN_COLOR_MID) {
            if (pickColor(hwnd, g_config.middleColor)) {
                InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_MID), nullptr, TRUE);
            }
            return 0;
        }

        if (id == IDC_BTN_COLOR_AMBIENT) {
            if (pickColor(hwnd, g_config.ambientColor)) {
                InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_AMBIENT), nullptr, TRUE);
            }
            return 0;
        }

        if (id == IDC_BTN_COLOR_TRAIL) {
            if (pickColor(hwnd, g_config.trailColor)) {
                InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_TRAIL), nullptr, TRUE);
            }
            return 0;
        }

        if (id == IDC_BTN_COLOR_INK) {
            if (pickColor(hwnd, g_config.inkColor)) {
                InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_INK), nullptr, TRUE);
            }
            return 0;
        }

        if (id == IDC_BTN_COLOR_ARROW) {
            if (pickColor(hwnd, g_config.arrowColor)) {
                InvalidateRect(GetDlgItem(hwnd, IDC_BTN_COLOR_ARROW), nullptr, TRUE);
            }
            return 0;
        }

        if (id == IDC_CHK_CENTERDOT) {
            g_config.centerDot = (IsDlgButtonChecked(hwnd, IDC_CHK_CENTERDOT) == BST_CHECKED);
            return 0;
        }

        if (id == IDC_CHK_AMBIENT) {
            g_config.ambientRipple = (IsDlgButtonChecked(hwnd, IDC_CHK_AMBIENT) == BST_CHECKED);
            return 0;
        }

        if (id == IDC_CHK_TRAIL) {
            g_config.trailEnabled = (IsDlgButtonChecked(hwnd, IDC_CHK_TRAIL) == BST_CHECKED);
            if (!g_config.trailEnabled) {
                g_trailPoints.clear();
            }
            return 0;
        }

        if (id == IDC_CHK_ANNOTATION) {
            g_config.annotationEnabled = (IsDlgButtonChecked(hwnd, IDC_CHK_ANNOTATION) == BST_CHECKED);
            if (!g_config.annotationEnabled) {
                g_annotations.clear();
                g_isDrawingInk = false;
                g_isDrawingArrow = false;
            }
            return 0;
        }

        if (id == IDC_BTN_RESET) {
            applyPreset(PRESET_WACOM);
            g_config.leftColor = RGB(232, 65, 82);
            g_config.rightColor = RGB(41, 128, 245);
            g_config.middleColor = RGB(245, 166, 35);
            g_config.ambientColor = RGB(145, 145, 145);
            g_config.ambientBand = 2;
            g_config.ambientThickness = 2;
            g_config.trailEnabled = true;
            g_config.trailColor = RGB(41, 128, 245);
            g_config.trailDurationMs = 380;
            g_config.trailWidth = 8;
            g_config.trailAlphaPercent = 80;
            g_config.trailAlpha = static_cast<int>(255.0f * 0.80f);
            g_config.annotationEnabled = true;
            g_config.inkColor = RGB(255, 68, 68);
            g_config.inkWidth = 6;
            g_config.arrowColor = RGB(255, 140, 0);
            g_config.arrowWidth = 6;
            g_config.annotationHoldMs = 2200;
            syncSettingsControls(hwnd);
            return 0;
        }

        if (id == IDC_BTN_SAVE) {
            saveConfig();
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        saveConfig();
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        if (g_uiFont) {
            DeleteObject(g_uiFont);
            g_uiFont = nullptr;
        }
        g_settingsWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void openSettingsWindow() {
    if (g_settingsWnd && IsWindow(g_settingsWnd)) {
        ShowWindow(g_settingsWnd, SW_SHOW);
        ShowWindow(g_settingsWnd, SW_RESTORE);
        SetWindowPos(g_settingsWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(g_settingsWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(g_settingsWnd);
        SetActiveWindow(g_settingsWnd);
        return;
    }

    const wchar_t kSettingsClassName[] = L"MouseRippleSettingsWindow";
    static bool s_settingsClassRegistered = false;
    if (!s_settingsClassRegistered) {
        WNDCLASSEX wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = settingsWndProc;
        wc.hInstance = g_instance;
        wc.lpszClassName = kSettingsClassName;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hIcon = (HICON)LoadImageW(g_instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                     GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
        wc.hIconSm = (HICON)LoadImageW(g_instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                       GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassEx(&wc);
        s_settingsClassRegistered = true;
    }

    const int w = 840;
    const int h = 635;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    g_settingsWnd = CreateWindowExW(
        WS_EX_APPWINDOW | WS_EX_TOPMOST,
        kSettingsClassName,
        L"MouseRipple - 自定义设置",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, w, h,
        nullptr, nullptr, g_instance, nullptr);

    if (g_settingsWnd) {
        HICON hBig = (HICON)LoadImageW(g_instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                       GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
        HICON hSm  = (HICON)LoadImageW(g_instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                       GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
        if (hBig) SendMessageW(g_settingsWnd, WM_SETICON, ICON_BIG, (LPARAM)hBig);
        if (hSm)  SendMessageW(g_settingsWnd, WM_SETICON, ICON_SMALL, (LPARAM)hSm);
        ShowWindow(g_settingsWnd, SW_SHOW);
        UpdateWindow(g_settingsWnd);
        SetWindowPos(g_settingsWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(g_settingsWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(g_settingsWnd);
    }
}

// -------------------------------------------------------------
// Main Overlay Window Implementation
// -------------------------------------------------------------

LRESULT CALLBACK overlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        addTrayIcon(hwnd);
        SetTimer(hwnd, kFrameTimerId, kFrameMs, nullptr);
        return 0;

    case WM_RIPPLE_CLICK: {
        POINT* p = reinterpret_cast<POINT*>(lParam);
        if (p) {
            addRipple(*p, static_cast<int>(wParam));
            delete p;
        }
        renderOverlay(hwnd);
        return 0;
    }

    case WM_TIMER:
        if (wParam == kFrameTimerId) {
            const ULONGLONG now = GetTickCount64();
            const ULONGLONG maxDuration = static_cast<ULONGLONG>(g_config.durationMs + 60);
            g_ripples.erase(
                std::remove_if(g_ripples.begin(), g_ripples.end(),
                    [now, maxDuration](const Ripple& r) {
                        return (now - r.startedAt) > maxDuration;
                    }),
                g_ripples.end());

            // Prune expired trail points
            if (!g_trailPoints.empty()) {
                const double nowMs = getHighPrecisionMs();
                const double trailMaxAge = static_cast<double>(g_config.trailDurationMs);
                g_trailPoints.erase(
                    std::remove_if(g_trailPoints.begin(), g_trailPoints.end(),
                        [nowMs, trailMaxAge](const TrailPoint& tp) {
                            return (nowMs - tp.timeMs) > trailMaxAge;
                        }),
                    g_trailPoints.end());
            }

            // Prune expired annotations
            if (!g_annotations.empty()) {
                const double nowMs = getHighPrecisionMs();
                g_annotations.erase(
                    std::remove_if(g_annotations.begin(), g_annotations.end(),
                        [nowMs](const AnnotationStroke& s) {
                            if (s.isDrawing) return false;
                            return (nowMs - s.releasedAt) > (s.holdMs + s.fadeMs);
                        }),
                    g_annotations.end());
            }

            static bool s_wasIdle = false;
            const bool isIdle = g_ripples.empty() && g_trailPoints.empty() && g_annotations.empty() && !g_config.ambientRipple;
            if (isIdle) {
                if (!s_wasIdle) {
                    renderOverlay(hwnd);
                    s_wasIdle = true;
                }
            } else {
                s_wasIdle = false;
                renderOverlay(hwnd);
            }
        }
        return 0;

    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
        updateVirtualDesktopMetrics();
        SetWindowPos(hwnd, HWND_TOPMOST,
                     g_virtualX, g_virtualY, g_virtualW, g_virtualH,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
        renderOverlay(hwnd);
        return 0;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            showTrayMenu(hwnd);
        } else if (lParam == WM_LBUTTONDBLCLK) {
            openSettingsWindow();
        }
        return 0;

    case WM_COMMAND: {
        const UINT id = LOWORD(wParam);
        if (id == ID_TRAY_SETTINGS) {
            openSettingsWindow();
            return 0;
        }
        if (id == ID_TRAY_AUTOSTART) {
            const bool enabled = isAutoStartEnabled();
            setAutoStart(!enabled);
            return 0;
        }
        if (id == ID_TRAY_EXIT) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }

    case WM_NCHITTEST:
        return HTTRANSPARENT;

    case WM_DESTROY:
        KillTimer(hwnd, kFrameTimerId);
        removeTrayIcon(hwnd);
        if (g_settingsWnd) {
            DestroyWindow(g_settingsWnd);
            g_settingsWnd = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

} // namespace

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int) {
    g_instance = hInstance;

    // Single instance protection: wake up existing instance and bring settings to front
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, L"MouseRipple_SingleInstance_Mutex_2026");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hOverlay = FindWindowW(L"MouseRippleOverlayWindow", nullptr);
        HWND hSettings = FindWindowW(L"MouseRippleSettingsWindow", nullptr);
        if (hOverlay || hSettings) {
            DWORD targetPid = 0;
            if (hOverlay) {
                GetWindowThreadProcessId(hOverlay, &targetPid);
            } else if (hSettings) {
                GetWindowThreadProcessId(hSettings, &targetPid);
            }
            if (targetPid) {
                AllowSetForegroundWindow(targetPid);
            }
            if (hOverlay) {
                PostMessageW(hOverlay, WM_COMMAND, ID_TRAY_SETTINGS, 0);
            }
            if (hSettings) {
                ShowWindow(hSettings, SW_SHOW);
                ShowWindow(hSettings, SW_RESTORE);
                SetWindowPos(hSettings, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                SetWindowPos(hSettings, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                SetForegroundWindow(hSettings);
                SetActiveWindow(hSettings);
            }
            if (hMutex) CloseHandle(hMutex);
            return 0;
        }
        // If neither overlay nor settings window exists, previous instance died or was orphaned. Proceed!
    }

    // Modern Per-Monitor DPI
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL (WINAPI *SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
        auto setContext = (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (setContext) {
            setContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            SetProcessDPIAware();
        }
    } else {
        SetProcessDPIAware();
    }

    // Initialize Common Controls v6
    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // Load user configuration
    loadConfig();

    timeBeginPeriod(1);

    GdiplusStartupInput gdiplusStartupInput;
    if (GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) {
        MessageBox(nullptr, L"Could not start GDI+.", L"Mouse Ripple", MB_ICONERROR);
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    const wchar_t kClassName[] = L"MouseRippleOverlayWindow";
    WNDCLASSEX wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = overlayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                 GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                   GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    RegisterClassEx(&wc);

    updateVirtualDesktopMetrics();

    g_overlay = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        kClassName,
        L"Mouse Ripple Overlay",
        WS_POPUP,
        g_virtualX, g_virtualY, g_virtualW, g_virtualH,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_overlay) {
        GdiplusShutdown(g_gdiplusToken);
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    ShowWindow(g_overlay, SW_SHOWNOACTIVATE);
    SetWindowPos(g_overlay, HWND_TOPMOST,
                 g_virtualX, g_virtualY, g_virtualW, g_virtualH,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, lowLevelMouseProc, hInstance, 0);
    g_kbdHook   = SetWindowsHookEx(WH_KEYBOARD_LL, lowLevelKeyboardProc, hInstance, 0);
    if (!g_mouseHook) {
        MessageBox(nullptr, L"Could not install mouse hook.", L"Mouse Ripple", MB_ICONERROR);
        if (g_kbdHook) {
            UnhookWindowsHookEx(g_kbdHook);
            g_kbdHook = nullptr;
        }
        DestroyWindow(g_overlay);
        GdiplusShutdown(g_gdiplusToken);
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    // On manual launch, always open settings GUI so the user clearly sees the software has opened!
    // Only suppress window if started via Windows autostart (--autostart) or explicit background flag (--tray)
    const bool isBackgroundMode = lpCmdLine && (wcsstr(lpCmdLine, L"--autostart") || wcsstr(lpCmdLine, L"--tray") || wcsstr(lpCmdLine, L"--minimized"));
    if (!isBackgroundMode) {
        openSettingsWindow();
    }

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_mouseHook) {
        UnhookWindowsHookEx(g_mouseHook);
        g_mouseHook = nullptr;
    }
    if (g_kbdHook) {
        UnhookWindowsHookEx(g_kbdHook);
        g_kbdHook = nullptr;
    }
    timeEndPeriod(1);
    GdiplusShutdown(g_gdiplusToken);
    if (hMutex) CloseHandle(hMutex);
    return static_cast<int>(msg.wParam);
}
