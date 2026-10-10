#include <windows.h>
#include <commdlg.h>
#include <richedit.h>
#include <string>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")

// ==============================
// 工具栏按钮命令 ID
// ==============================
#define ID_BUTTON_OPEN       1001
#define ID_BUTTON_SAVE       1002
#define ID_BUTTON_UNDO       1003
#define ID_BUTTON_REDO       1004
#define ID_BUTTON_CUT        1005
#define ID_BUTTON_COPY       1006
#define ID_BUTTON_PASTE      1007
#define ID_BUTTON_FIND       1008
#define ID_BUTTON_BOLD       1009
#define ID_BUTTON_ITALIC     1010
#define ID_BUTTON_UNDERLINE  1011
#define ID_BUTTON_HIGHLIGHT  1012
#define ID_BUTTON_TEXTCOLOR  1013  // 【新增】字体颜色按钮 ID

#define ID_COMBO_FONT        1101
#define ID_COMBO_SIZE        1102
#define ID_STATUS            1201
#define ID_TOOLBAR           1301

#define ID_ACCEL_SAVE        2001
#define ID_ACCEL_FIND        2002

#define ID_FIND_EDIT         3001
#define ID_FIND_BUTTON       3002

#define ID_TIMER_AUTOSAVE    5001

// 菜单 ID
#define ID_MENU_FILE_OPEN          6001
#define ID_MENU_FILE_SAVE          6002
#define ID_MENU_FILE_SAVEAS        6003
#define ID_MENU_FILE_EXPORT_TXT    6004
#define ID_MENU_FILE_EXIT          6005

#define ID_MENU_EDIT_UNDO          6101
#define ID_MENU_EDIT_REDO          6102
#define ID_MENU_EDIT_CUT           6103
#define ID_MENU_EDIT_COPY          6104
#define ID_MENU_EDIT_PASTE         6105
#define ID_MENU_EDIT_SELECTALL     6106
#define ID_MENU_EDIT_FIND          6107

#define ID_MENU_FORMAT_FONT        6201
#define ID_MENU_FORMAT_SIZE        6202
#define ID_MENU_FORMAT_BOLD        6203
#define ID_MENU_FORMAT_ITALIC      6204
#define ID_MENU_FORMAT_UNDERLINE   6205
#define ID_MENU_FORMAT_STRIKE      6206
#define ID_MENU_FORMAT_TEXTCOLOR   6207
#define ID_MENU_FORMAT_HIGHLIGHT   6208
#define ID_MENU_FORMAT_CLEAR       6209

#define ID_MENU_VIEW_STATUSBAR     6301
#define ID_MENU_VIEW_WORDWRAP      6302
#define ID_MENU_VIEW_ZOOMIN        6303
#define ID_MENU_VIEW_ZOOMOUT       6304
#define ID_MENU_VIEW_ZOOMRESET     6305

#define ID_MENU_HELP_ABOUT         6401

// ==============================
// 全局
// ==============================
std::wstring g_currentFilePath = L"";
bool g_isModified = false;
bool g_showStatusBar = true;
bool g_wordWrap = false;

HWND hEdit = NULL;
HWND hStatus = NULL;
HWND hToolbar = NULL;
HINSTANCE g_hInstance = NULL;
HWND g_hMainWnd = NULL;

HWND g_hFindWindow = NULL;
HWND g_hFindEdit = NULL;

HWND g_hFontCombo = NULL;
HWND g_hSizeCombo = NULL;

// 工具栏按钮图像列表（用文字代替图像）
HIMAGELIST g_hToolbarImages = NULL;

// 前置声明
void UpdateStatusBar();
void AutoSave();
void SaveLastFilePathToRegistry();
std::wstring LoadLastFilePathFromRegistry();
bool OpenNoteByPath(const std::wstring& path);
HMENU CreateAppMenu();
void SaveNoteAs(HWND hwnd);
void ExportTxt(HWND hwnd);
void SelectAll();
void ToggleBold();
void ToggleItalic();
void ToggleUnderline();
void ToggleStrike();
void SetTextColor(COLORREF color);
void ClearFormat();
void UpdateMenuCheck();
void OpenNote(HWND hwnd);
void SaveNote(HWND hwnd);
void ShowFindWindow(HWND hwndParent);
void SetSelectedFont(HWND hwndEdit, const wchar_t* fontName);
void SetSelectedFontSize(HWND hwndEdit, int pointSize);
void SetSelectedHighlight(HWND hwndEdit, COLORREF color);
void CreateToolbar(HWND hwnd);
void PositionToolbarCombos(HWND hwnd);
void LayoutChildren(HWND hwnd);


// ============================================================
// RTF 回调
// ============================================================
static DWORD CALLBACK SaveRtfCallback(
    DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb)
{
    HANDLE hFile = (HANDLE)dwCookie;
    DWORD written = 0;
    if (!WriteFile(hFile, pbBuff, (DWORD)cb, &written, NULL))
    {
        *pcb = 0;
        return 1;
    }
    *pcb = (LONG)written;
    return 0;
}

static DWORD CALLBACK LoadRtfCallback(
    DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb)
{
    HANDLE hFile = (HANDLE)dwCookie;
    DWORD read = 0;
    if (!ReadFile(hFile, pbBuff, (DWORD)cb, &read, NULL))
    {
        *pcb = 0;
        return 1;
    }
    *pcb = (LONG)read;
    return 0;
}

static DWORD CALLBACK SaveTextCallback(
    DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb)
{
    HANDLE hFile = (HANDLE)dwCookie;
    DWORD written = 0;
    if (!WriteFile(hFile, pbBuff, (DWORD)cb, &written, NULL))
    {
        *pcb = 0;
        return 1;
    }
    *pcb = (LONG)written;
    return 0;
}


// ============================================================
// 格式操作
// ============================================================
void SetSelectedFont(HWND hwndEdit, const wchar_t* fontName)
{
    CHARFORMATW cf = {};
    cf.cbSize = sizeof(CHARFORMATW);
    cf.dwMask = CFM_FACE;
    wcscpy_s(cf.szFaceName, LF_FACESIZE, fontName);
    SendMessageW(hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

void SetSelectedFontSize(HWND hwndEdit, int pointSize)
{
    CHARFORMATW cf = {};
    cf.cbSize = sizeof(CHARFORMATW);
    cf.dwMask = CFM_SIZE;
    cf.yHeight = pointSize * 20;
    SendMessageW(hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

void SetSelectedHighlight(HWND hwndEdit, COLORREF color)
{
    CHARFORMAT2W cf = {};
    cf.cbSize = sizeof(CHARFORMAT2W);
    cf.dwMask = CFM_BACKCOLOR;

    if (color == CLR_NONE)
        cf.dwEffects = CFE_AUTOBACKCOLOR;
    else
        cf.crBackColor = color;

    SendMessageW(hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

void SetTextColor(COLORREF color)
{
    CHARFORMAT2W cf = {};
    cf.cbSize = sizeof(CHARFORMAT2W);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    SendMessageW(hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

static void ToggleEffect(DWORD mask, DWORD effect)
{
    CHARFORMAT2W cf = {};
    cf.cbSize = sizeof(CHARFORMAT2W);
    cf.dwMask = mask;

    CHARFORMAT2W cfGet = {};
    cfGet.cbSize = sizeof(CHARFORMAT2W);
    SendMessageW(hEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfGet);

    if (cfGet.dwEffects & effect)
        cf.dwEffects = 0;
    else
        cf.dwEffects = effect;

    SendMessageW(hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

void ToggleBold() { ToggleEffect(CFM_BOLD, CFE_BOLD); }
void ToggleItalic() { ToggleEffect(CFM_ITALIC, CFE_ITALIC); }
void ToggleUnderline() { ToggleEffect(CFM_UNDERLINE, CFE_UNDERLINE); }
void ToggleStrike() { ToggleEffect(CFM_STRIKEOUT, CFE_STRIKEOUT); }

void ClearFormat()
{
    CHARFORMAT2W cf = {};
    cf.cbSize = sizeof(CHARFORMAT2W);
    cf.dwMask = CFM_FACE | CFM_SIZE | CFM_BOLD | CFM_ITALIC |
        CFM_UNDERLINE | CFM_STRIKEOUT | CFM_COLOR | CFM_BACKCOLOR;
    wcscpy_s(cf.szFaceName, LF_FACESIZE, L"微软雅黑");
    cf.yHeight = 12 * 20;
    cf.crTextColor = RGB(0, 0, 0);
    cf.dwEffects = CFE_AUTOBACKCOLOR;

    SendMessageW(hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

void SelectAll()
{
    SendMessageW(hEdit, EM_SETSEL, 0, -1);
    SetFocus(hEdit);
    UpdateStatusBar();
}


// ============================================================
// 查找
// ============================================================
void FindTextInEdit(HWND hwndEdit, const std::wstring& searchText)
{
    if (searchText.empty())
    {
        MessageBoxW(g_hFindWindow, L"请输入要查找的文字。", L"查找",
            MB_OK | MB_ICONINFORMATION);
        return;
    }

    CHARRANGE range = {};
    SendMessageW(hwndEdit, EM_EXGETSEL, 0, (LPARAM)&range);

    FINDTEXTW ft = {};
    ft.chrg.cpMin = range.cpMax;
    ft.chrg.cpMax = -1;
    ft.lpstrText = searchText.c_str();

    LONG result = (LONG)SendMessageW(
        hwndEdit, EM_FINDTEXTW, FR_DOWN, (LPARAM)&ft);

    if (result == -1)
    {
        ft.chrg.cpMin = 0;
        ft.chrg.cpMax = -1;
        result = (LONG)SendMessageW(
            hwndEdit, EM_FINDTEXTW, FR_DOWN, (LPARAM)&ft);
    }

    if (result != -1)
    {
        CHARRANGE foundRange = {};
        foundRange.cpMin = result;
        foundRange.cpMax = result + (LONG)searchText.length();
        SendMessageW(hwndEdit, EM_EXSETSEL, 0, (LPARAM)&foundRange);
        SendMessageW(hwndEdit, EM_SCROLLCARET, 0, 0);
    }
    else
    {
        MessageBoxW(g_hFindWindow, L"没有找到指定文字。", L"查找",
            MB_OK | MB_ICONINFORMATION);
    }
}


// ============================================================
// 状态栏
// ============================================================
void UpdateStatusBar()
{
    if (hEdit == NULL || hStatus == NULL)
        return;

    CHARRANGE range = {};
    SendMessageW(hEdit, EM_EXGETSEL, 0, (LPARAM)&range);

    LONG line = (LONG)SendMessageW(
        hEdit, EM_LINEFROMCHAR, (WPARAM)range.cpMin, 0);
    LONG lineStart = (LONG)SendMessageW(
        hEdit, EM_LINEINDEX, (WPARAM)line, 0);

    int column = 1;
    if (lineStart >= 0 && range.cpMin >= lineStart)
        column = (int)(range.cpMin - lineStart) + 1;

    int charCount = GetWindowTextLengthW(hEdit);

    wchar_t part1[64] = {};
    swprintf_s(part1, L"行：%ld  列：%d", line + 1, column);

    wchar_t part2[64] = {};
    swprintf_s(part2, L"字符数：%d", charCount);

    const wchar_t* part3 = g_isModified ? L"已修改" : L"就绪";

    SendMessageW(hStatus, SB_SETTEXTW, 0, (LPARAM)part1);
    SendMessageW(hStatus, SB_SETTEXTW, 1, (LPARAM)part2);
    SendMessageW(hStatus, SB_SETTEXTW, 2, (LPARAM)part3);
}


// ============================================================
// 注册表
// ============================================================
void SaveLastFilePathToRegistry()
{
    if (g_currentFilePath.empty()) return;

    HKEY hKey = NULL;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\LightNote",
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
        NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, L"LastFile", 0, REG_SZ,
            (const BYTE*)g_currentFilePath.c_str(),
            (DWORD)((g_currentFilePath.length() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }
}

std::wstring LoadLastFilePathFromRegistry()
{
    std::wstring result;
    HKEY hKey = NULL;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\LightNote",
        0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        wchar_t buffer[MAX_PATH * 2] = {};
        DWORD size = sizeof(buffer);
        DWORD type = 0;

        if (RegQueryValueExW(hKey, L"LastFile", NULL, &type,
            (LPBYTE)buffer, &size) == ERROR_SUCCESS && type == REG_SZ)
        {
            result = buffer;
        }
        RegCloseKey(hKey);
    }
    return result;
}


// ============================================================
// 按路径打开
// ============================================================
bool OpenNoteByPath(const std::wstring& path)
{
    if (path.empty()) return false;

    HANDLE hFile = CreateFileW(path.c_str(),
        GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return false;

    EDITSTREAM es = {};
    es.dwCookie = (DWORD_PTR)hFile;
    es.pfnCallback = LoadRtfCallback;

    SendMessageW(hEdit, EM_STREAMIN, SF_RTF, (LPARAM)&es);

    if (es.dwError != 0)
    {
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        es.dwCookie = (DWORD_PTR)hFile;
        es.pfnCallback = LoadRtfCallback;
        es.dwError = 0;
        SendMessageW(hEdit, EM_STREAMIN, SF_TEXT, (LPARAM)&es);
    }

    CloseHandle(hFile);

    g_currentFilePath = path;
    g_isModified = false;

    CHARRANGE cr = { 0, 0 };
    SendMessageW(hEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
    UpdateStatusBar();
    return true;
}


// ============================================================
// 菜单
// ============================================================
HMENU CreateAppMenu()
{
    HMENU hMenuBar = CreateMenu();

    HMENU hFile = CreatePopupMenu();
    AppendMenuW(hFile, MF_STRING, ID_MENU_FILE_OPEN, L"打开(&O)...\tCtrl+O");
    AppendMenuW(hFile, MF_STRING, ID_MENU_FILE_SAVE, L"保存(&S)\tCtrl+S");
    AppendMenuW(hFile, MF_STRING, ID_MENU_FILE_SAVEAS, L"另存为(&A)...");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, ID_MENU_FILE_EXPORT_TXT, L"导出为 TXT(&E)...");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, ID_MENU_FILE_EXIT, L"退出(&X)\tAlt+F4");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFile, L"文件(&F)");

    HMENU hEditMenu = CreatePopupMenu();
    AppendMenuW(hEditMenu, MF_STRING, ID_MENU_EDIT_UNDO, L"撤销(&U)\tCtrl+Z");
    AppendMenuW(hEditMenu, MF_STRING, ID_MENU_EDIT_REDO, L"重做(&R)\tCtrl+Y");
    AppendMenuW(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEditMenu, MF_STRING, ID_MENU_EDIT_CUT, L"剪切(&T)\tCtrl+X");
    AppendMenuW(hEditMenu, MF_STRING, ID_MENU_EDIT_COPY, L"复制(&C)\tCtrl+C");
    AppendMenuW(hEditMenu, MF_STRING, ID_MENU_EDIT_PASTE, L"粘贴(&P)\tCtrl+V");
    AppendMenuW(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEditMenu, MF_STRING, ID_MENU_EDIT_SELECTALL, L"全选(&A)\tCtrl+A");
    AppendMenuW(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEditMenu, MF_STRING, ID_MENU_EDIT_FIND, L"查找(&F)...\tCtrl+F");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hEditMenu, L"编辑(&E)");

    HMENU hFormat = CreatePopupMenu();
    AppendMenuW(hFormat, MF_STRING, ID_MENU_FORMAT_FONT, L"字体(&F)...");
    AppendMenuW(hFormat, MF_STRING, ID_MENU_FORMAT_SIZE, L"字号(&S)...");
    AppendMenuW(hFormat, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFormat, MF_STRING, ID_MENU_FORMAT_BOLD, L"加粗(&B)\tCtrl+B");
    AppendMenuW(hFormat, MF_STRING, ID_MENU_FORMAT_ITALIC, L"斜体(&I)\tCtrl+I");
    AppendMenuW(hFormat, MF_STRING, ID_MENU_FORMAT_UNDERLINE, L"下划线(&U)\tCtrl+U");
    AppendMenuW(hFormat, MF_STRING, ID_MENU_FORMAT_STRIKE, L"删除线(&K)");
    AppendMenuW(hFormat, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFormat, MF_STRING, ID_MENU_FORMAT_TEXTCOLOR, L"文字颜色(&C)...");
    AppendMenuW(hFormat, MF_STRING, ID_MENU_FORMAT_HIGHLIGHT, L"高亮颜色(&H)...");
    AppendMenuW(hFormat, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFormat, MF_STRING, ID_MENU_FORMAT_CLEAR, L"清除格式(&L)");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFormat, L"格式(&O)");

    HMENU hView = CreatePopupMenu();
    AppendMenuW(hView, MF_STRING | MF_CHECKED, ID_MENU_VIEW_STATUSBAR, L"状态栏(&S)");
    AppendMenuW(hView, MF_STRING, ID_MENU_VIEW_WORDWRAP, L"自动换行(&W)");
    AppendMenuW(hView, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hView, MF_STRING, ID_MENU_VIEW_ZOOMIN, L"放大字号\tCtrl+=");
    AppendMenuW(hView, MF_STRING, ID_MENU_VIEW_ZOOMOUT, L"缩小字号\tCtrl+-");
    AppendMenuW(hView, MF_STRING, ID_MENU_VIEW_ZOOMRESET, L"重置字号\tCtrl+0");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hView, L"查看(&V)");

    HMENU hHelp = CreatePopupMenu();
    AppendMenuW(hHelp, MF_STRING, ID_MENU_HELP_ABOUT, L"关于 LightNote(&A)...");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hHelp, L"帮助(&H)");

    return hMenuBar;
}


// ============================================================
// 创建工具栏（用文字代替图标）
// ============================================================
void CreateToolbar(HWND hwnd)
{
    // 注册工具栏窗口类
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    hToolbar = CreateWindowExW(
        0,
        TOOLBARCLASSNAMEW,
        NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS |
        CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN,
        0, 0, 0, 0,
        hwnd,
        (HMENU)ID_TOOLBAR,
        g_hInstance,
        NULL
    );

    if (hToolbar == NULL) return;

    SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(hToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(0, 0));

    // 按钮列表
    TBBUTTON buttons[] =
    {
        { 0, ID_BUTTON_OPEN,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"打开" },
        { 0, ID_BUTTON_SAVE,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"保存" },
        { 0, 0,                   TBSTATE_ENABLED, BTNS_SEP,                    {0}, 0, 0 },
        { 0, ID_BUTTON_UNDO,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"撤销" },
        { 0, ID_BUTTON_REDO,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"重做" },
        { 0, 0,                   TBSTATE_ENABLED, BTNS_SEP,                    {0}, 0, 0 },
        { 0, ID_BUTTON_CUT,       TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"剪切" },
        { 0, ID_BUTTON_COPY,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"复制" },
        { 0, ID_BUTTON_PASTE,     TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"粘贴" },
        { 0, 0,                   TBSTATE_ENABLED, BTNS_SEP,                    {0}, 0, 0 },
        { 0, ID_BUTTON_FIND,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"查找" },
        { 0, 0,                   TBSTATE_ENABLED, BTNS_SEP,                    {0}, 0, 0 },
        { 0, ID_BUTTON_BOLD,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"B" },
        { 0, ID_BUTTON_ITALIC,    TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"I" },
        { 0, ID_BUTTON_UNDERLINE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"U" },
        { 0, 0,                   TBSTATE_ENABLED, BTNS_SEP,                    {0}, 0, 0 },
        { 0, ID_BUTTON_HIGHLIGHT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"高亮" },
        // 【新增】字体颜色按钮
        { 0, ID_BUTTON_TEXTCOLOR, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"颜色" },
    };

    SendMessageW(hToolbar, TB_ADDBUTTONSW,
        sizeof(buttons) / sizeof(TBBUTTON), (LPARAM)buttons);

    // 高度自适应
    SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);
}


// ============================================================
// 布局：把字体/字号下拉放到工具栏右侧
// ============================================================
void PositionToolbarCombos(HWND hwnd)
{
    if (hToolbar == NULL || g_hFontCombo == NULL || g_hSizeCombo == NULL)
        return;

    RECT rcTool;
    GetWindowRect(hToolbar, &rcTool);
    MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rcTool, 2);

    int toolH = rcTool.bottom - rcTool.top;

    // 字体下拉框
    int fontW = 130;
    int sizeW = 70;
    int gap = 6;
    int rightMargin = 10;

    int sizeX = rcTool.right - rightMargin - sizeW;
    int fontX = sizeX - gap - fontW;
    int comboY = rcTool.top + (toolH - 24) / 2;

    MoveWindow(g_hFontCombo, fontX, comboY, fontW, 200, TRUE);
    MoveWindow(g_hSizeCombo, sizeX, comboY, sizeW, 200, TRUE);
}


// ============================================================
// 统一布局（编辑框 + 状态栏 + 下拉框）
// ============================================================
void LayoutChildren(HWND hwnd)
{
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    // 工具栏
    int toolbarHeight = 0;
    if (hToolbar != NULL)
    {
        SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);

        RECT rcTool;
        GetWindowRect(hToolbar, &rcTool);
        toolbarHeight = rcTool.bottom - rcTool.top;
    }

    // 状态栏
    int statusHeight = 0;
    if (g_showStatusBar && hStatus != NULL)
    {
        SendMessageW(hStatus, WM_SIZE, 0, 0);

        RECT rcStatus;
        GetWindowRect(hStatus, &rcStatus);
        statusHeight = rcStatus.bottom - rcStatus.top;
    }

    // 编辑框
    if (hEdit != NULL)
    {
        MoveWindow(hEdit, 0, toolbarHeight, width,
            max(0, height - toolbarHeight - statusHeight), TRUE);
    }

    // 下拉框定位到工具栏右侧
    PositionToolbarCombos(hwnd);
}


// ============================================================
// 查找窗口
// ============================================================
LRESULT CALLBACK FindWndProc(HWND hwnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"查找内容：",
            WS_CHILD | WS_VISIBLE, 15, 18, 80, 25,
            hwnd, NULL, g_hInstance, NULL);

        g_hFindEdit = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            95, 15, 210, 28,
            hwnd, (HMENU)ID_FIND_EDIT, g_hInstance, NULL);

        CreateWindowW(L"BUTTON", L"查找",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            315, 15, 70, 28,
            hwnd, (HMENU)ID_FIND_BUTTON, g_hInstance, NULL);

        SetFocus(g_hFindEdit);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_FIND_BUTTON)
        {
            wchar_t buffer[256] = L"";
            GetWindowTextW(g_hFindEdit, buffer, 256);
            FindTextInEdit(hEdit, buffer);
            return 0;
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_RETURN)
        {
            wchar_t buffer[256] = L"";
            GetWindowTextW(g_hFindEdit, buffer, 256);
            FindTextInEdit(hEdit, buffer);
            return 0;
        }
        break;

    case WM_CLOSE:
        g_hFindWindow = NULL;
        g_hFindEdit = NULL;
        EnableWindow(GetParent(hwnd), TRUE);
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void ShowFindWindow(HWND hwndParent)
{
    if (g_hFindWindow != NULL)
    {
        SetForegroundWindow(g_hFindWindow);
        return;
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = FindWndProc;
    wc.hInstance = g_hInstance;
    wc.lpszClassName = L"LightNoteFindWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    g_hFindWindow = CreateWindowW(
        L"LightNoteFindWindow", L"查找",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 420, 95,
        hwndParent, NULL, g_hInstance, NULL);

    if (g_hFindWindow != NULL)
    {
        EnableWindow(hwndParent, FALSE);
        ShowWindow(g_hFindWindow, SW_SHOW);
        UpdateWindow(g_hFindWindow);
    }
}


// ============================================================
// 打开 / 保存 / 另存为 / 导出 / 自动保存
// ============================================================
void OpenNote(HWND hwnd)
{
    OPENFILENAMEW ofn = {};
    wchar_t fileName[MAX_PATH] = L"";

    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter =
        L"富文本文件 (*.rtf)\0*.rtf\0"
        L"文本文件 (*.txt)\0*.txt\0"
        L"所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"rtf";

    if (GetOpenFileNameW(&ofn))
    {
        if (OpenNoteByPath(fileName))
            SaveLastFilePathToRegistry();
    }
}

void SaveNoteAs(HWND hwnd)
{
    OPENFILENAMEW ofn = {};
    wchar_t fileName[MAX_PATH] = L"我的笔记.rtf";

    if (!g_currentFilePath.empty())
        wcscpy_s(fileName, MAX_PATH, g_currentFilePath.c_str());

    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter =
        L"富文本文件 (*.rtf)\0*.rtf\0"
        L"文本文件 (*.txt)\0*.txt\0"
        L"所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"rtf";

    if (!GetSaveFileNameW(&ofn))
        return;

    g_currentFilePath = fileName;
    SaveNote(hwnd);
}

void SaveNote(HWND hwnd)
{
    if (g_currentFilePath.empty())
    {
        SaveNoteAs(hwnd);
        return;
    }

    HANDLE hFile = CreateFileW(g_currentFilePath.c_str(),
        GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        EDITSTREAM es = {};
        es.dwCookie = (DWORD_PTR)hFile;
        es.pfnCallback = SaveRtfCallback;
        SendMessageW(hEdit, EM_STREAMOUT, SF_RTF, (LPARAM)&es);
        CloseHandle(hFile);

        if (es.dwError == 0)
        {
            g_isModified = false;
            UpdateStatusBar();
            SaveLastFilePathToRegistry();
        }
    }
}

void AutoSave()
{
    if (g_currentFilePath.empty()) return;
    if (!g_isModified) return;

    HANDLE hFile = CreateFileW(g_currentFilePath.c_str(),
        GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return;

    EDITSTREAM es = {};
    es.dwCookie = (DWORD_PTR)hFile;
    es.pfnCallback = SaveRtfCallback;
    SendMessageW(hEdit, EM_STREAMOUT, SF_RTF, (LPARAM)&es);
    CloseHandle(hFile);

    if (es.dwError == 0)
    {
        g_isModified = false;
        UpdateStatusBar();
        SaveLastFilePathToRegistry();
    }
}

void ExportTxt(HWND hwnd)
{
    OPENFILENAMEW ofn = {};
    wchar_t fileName[MAX_PATH] = L"导出.txt";

    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"文本文件 (*.txt)\0*.txt\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"txt";

    if (!GetSaveFileNameW(&ofn)) return;

    HANDLE hFile = CreateFileW(fileName,
        GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        MessageBoxW(hwnd, L"导出失败！", L"LightNote", MB_OK | MB_ICONERROR);
        return;
    }

    DWORD written = 0;
    const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
    WriteFile(hFile, bom, 3, &written, NULL);

    EDITSTREAM es = {};
    es.dwCookie = (DWORD_PTR)hFile;
    es.pfnCallback = SaveTextCallback;
    SendMessageW(hEdit, EM_STREAMOUT, SF_TEXT | SF_UNICODE, (LPARAM)&es);
    CloseHandle(hFile);

    if (es.dwError == 0)
        MessageBoxW(hwnd, L"已导出为 TXT（格式不保留）", L"LightNote",
            MB_OK | MB_ICONINFORMATION);
}


// ============================================================
// 更新查看菜单勾选
// ============================================================
void UpdateMenuCheck()
{
    HMENU hMenu = GetMenu(g_hMainWnd);
    if (hMenu == NULL) return;

    HMENU hView = GetSubMenu(hMenu, 3);
    if (hView == NULL) return;

    CheckMenuItem(hView, ID_MENU_VIEW_STATUSBAR,
        MF_BYCOMMAND | (g_showStatusBar ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hView, ID_MENU_VIEW_WORDWRAP,
        MF_BYCOMMAND | (g_wordWrap ? MF_CHECKED : MF_UNCHECKED));
}


// ============================================================
// 窗口过程
// ============================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        g_hMainWnd = hwnd;
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

        // 菜单
        HMENU hMenu = CreateAppMenu();
        SetMenu(hwnd, hMenu);

        // Rich Edit
        hEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"RichEdit20W",
            L"",
            WS_CHILD | WS_VISIBLE |
            ES_MULTILINE |
            ES_AUTOVSCROLL | ES_AUTOHSCROLL |
            WS_VSCROLL | WS_HSCROLL |
            ES_NOHIDESEL,
            0, 30, 1000, 500,
            hwnd, NULL, hInst, NULL);

        {
            LRESULT eventMask = SendMessageW(hEdit, EM_GETEVENTMASK, 0, 0);
            SendMessageW(hEdit, EM_SETEVENTMASK, 0,
                (LPARAM)(eventMask | ENM_SELCHANGE));
        }

        CHARFORMATW cf = {};
        cf.cbSize = sizeof(CHARFORMATW);
        cf.dwMask = CFM_FACE | CFM_SIZE;
        wcscpy_s(cf.szFaceName, LF_FACESIZE, L"微软雅黑");
        cf.yHeight = 12 * 20;
        SendMessageW(hEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

        // 工具栏
        CreateToolbar(hwnd);

        // 字体下拉
        g_hFontCombo = CreateWindowW(L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL,
            0, 0, 130, 200,
            hwnd, (HMENU)ID_COMBO_FONT, hInst, NULL);

        SendMessageW(g_hFontCombo, CB_ADDSTRING, 0, (LPARAM)L"微软雅黑");
        SendMessageW(g_hFontCombo, CB_ADDSTRING, 0, (LPARAM)L"宋体");
        SendMessageW(g_hFontCombo, CB_ADDSTRING, 0, (LPARAM)L"黑体");
        SendMessageW(g_hFontCombo, CB_ADDSTRING, 0, (LPARAM)L"Arial");
        SendMessageW(g_hFontCombo, CB_ADDSTRING, 0, (LPARAM)L"Times New Roman");
        SendMessageW(g_hFontCombo, CB_SETCURSEL, 0, 0);

        // 字号下拉
        g_hSizeCombo = CreateWindowW(L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL,
            0, 0, 70, 200,
            hwnd, (HMENU)ID_COMBO_SIZE, hInst, NULL);

        const wchar_t* sizes[] =
        { L"10", L"12", L"14", L"16", L"18", L"20", L"24", L"28", L"32" };

        for (int i = 0; i < 9; i++)
            SendMessageW(g_hSizeCombo, CB_ADDSTRING, 0, (LPARAM)sizes[i]);

        SendMessageW(g_hSizeCombo, CB_SETCURSEL, 1, 0);

        // 控件字体
        HFONT hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        SendMessageW(g_hFontCombo, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(g_hSizeCombo, WM_SETFONT, (WPARAM)hFont, TRUE);

        // 状态栏
        hStatus = CreateWindowExW(0, STATUSCLASSNAMEW, NULL,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hwnd, (HMENU)ID_STATUS, hInst, NULL);

        if (hStatus != NULL)
        {
            int parts[3] = { 260, 420, -1 };
            SendMessageW(hStatus, SB_SETPARTS, 3, (LPARAM)parts);
        }

        // 初次布局
        LayoutChildren(hwnd);

        SetTimer(hwnd, ID_TIMER_AUTOSAVE, 2000, NULL);
        return 0;
    }


    case WM_SIZE:
        LayoutChildren(hwnd);
        return 0;


    case WM_TIMER:
        if (wParam == ID_TIMER_AUTOSAVE) AutoSave();
        return 0;


    case WM_NOTIFY:
    {
        LPNMHDR pnm = (LPNMHDR)lParam;

        // 工具栏提示
        if (pnm != NULL && pnm->code == TTN_GETDISPINFOW &&
            pnm->hwndFrom == hToolbar)
        {
            LPNMTTDISPINFOW lpnmtdi = (LPNMTTDISPINFOW)lParam;
            static wchar_t buf[128] = L"";
            int id = (int)lpnmtdi->hdr.idFrom;

            const wchar_t* text = L"";
            switch (id)
            {
            case ID_BUTTON_OPEN:      text = L"打开文件 (Ctrl+O)"; break;
            case ID_BUTTON_SAVE:      text = L"保存 (Ctrl+S)"; break;
            case ID_BUTTON_UNDO:      text = L"撤销 (Ctrl+Z)"; break;
            case ID_BUTTON_REDO:      text = L"重做 (Ctrl+Y)"; break;
            case ID_BUTTON_CUT:       text = L"剪切 (Ctrl+X)"; break;
            case ID_BUTTON_COPY:      text = L"复制 (Ctrl+C)"; break;
            case ID_BUTTON_PASTE:     text = L"粘贴 (Ctrl+V)"; break;
            case ID_BUTTON_FIND:      text = L"查找 (Ctrl+F)"; break;
            case ID_BUTTON_BOLD:      text = L"加粗 (Ctrl+B)"; break;
            case ID_BUTTON_ITALIC:    text = L"斜体 (Ctrl+I)"; break;
            case ID_BUTTON_UNDERLINE: text = L"下划线 (Ctrl+U)"; break;
            case ID_BUTTON_HIGHLIGHT: text = L"高亮颜色"; break;
            case ID_BUTTON_TEXTCOLOR: text = L"字体颜色"; break; // 【新增】
            }

            wcscpy_s(buf, text);
            lpnmtdi->lpszText = buf;
            return TRUE;
        }

        // 编辑框选择变化
        if (pnm != NULL && pnm->hwndFrom == hEdit &&
            pnm->code == EN_SELCHANGE)
        {
            UpdateStatusBar();
        }
        return 0;
    }


    case WM_INITMENUPOPUP:
        UpdateMenuCheck();
        return 0;


    case WM_COMMAND:
    {
        if (HIWORD(wParam) == EN_CHANGE && (HWND)lParam == hEdit)
        {
            g_isModified = true;
            UpdateStatusBar();
        }

        switch (LOWORD(wParam))
        {
        case ID_BUTTON_OPEN:    OpenNote(hwnd); return 0;
        case ID_BUTTON_SAVE:    SaveNote(hwnd); return 0;

        case ID_BUTTON_UNDO:
            SendMessageW(hEdit, EM_UNDO, 0, 0);
            UpdateStatusBar();
            return 0;

        case ID_BUTTON_REDO:
            SendMessageW(hEdit, EM_REDO, 0, 0);
            UpdateStatusBar();
            return 0;

        case ID_BUTTON_CUT:   SendMessageW(hEdit, WM_CUT, 0, 0); return 0;
        case ID_BUTTON_COPY:  SendMessageW(hEdit, WM_COPY, 0, 0); return 0;
        case ID_BUTTON_PASTE: SendMessageW(hEdit, WM_PASTE, 0, 0); return 0;
        case ID_BUTTON_FIND:  ShowFindWindow(hwnd); return 0;

        case ID_BUTTON_BOLD:      ToggleBold(); g_isModified = true; return 0;
        case ID_BUTTON_ITALIC:    ToggleItalic(); g_isModified = true; return 0;
        case ID_BUTTON_UNDERLINE: ToggleUnderline(); g_isModified = true; return 0;

        case ID_BUTTON_HIGHLIGHT:
        {
            static COLORREF customColors[16] =
            {
                RGB(255, 255,   0), RGB(0, 255,   0),
                RGB(0, 255, 255), RGB(255,   0, 255),
                RGB(255,   0,   0), RGB(0,   0, 255),
                RGB(255, 128,   0), RGB(128,   0, 255),
                RGB(0, 128, 255), RGB(128, 255,   0),
                RGB(255, 192, 203), RGB(192, 192, 192),
                RGB(255, 255, 255), RGB(0,   0,   0),
                RGB(255, 165,   0), RGB(173, 216, 230)
            };

            CHOOSECOLORW cc = {};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = customColors;
            cc.rgbResult = RGB(255, 255, 0);
            cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR;

            if (ChooseColorW(&cc))
            {
                SetSelectedHighlight(hEdit, cc.rgbResult);
                g_isModified = true;
            }
            return 0;
        }

        // 【新增】字体颜色按钮响应（与菜单共用）
        case ID_BUTTON_TEXTCOLOR:
        case ID_MENU_FORMAT_TEXTCOLOR:
        {
            static COLORREF customColors[16] =
            {
                RGB(0,   0,   0), RGB(255,   0,   0),
                RGB(0, 128,   0), RGB(0,   0, 255),
                RGB(255, 128,   0), RGB(128,   0, 255),
                RGB(0, 128, 255), RGB(128, 128,   0),
                RGB(255,   0, 255), RGB(0, 255, 255),
                RGB(128,   0,   0), RGB(0, 128, 128),
                RGB(192, 192, 192), RGB(128, 128, 128),
                RGB(255, 255, 255), RGB(255, 255,   0)
            };

            CHOOSECOLORW cc = {};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = customColors;
            cc.rgbResult = RGB(0, 0, 0);
            cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR;

            if (ChooseColorW(&cc))
            {
                SetTextColor(cc.rgbResult);
                g_isModified = true;
                UpdateStatusBar();
            }
            return 0;
        }

        case ID_COMBO_FONT:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hCombo = (HWND)lParam;
                int index = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                wchar_t fontName[LF_FACESIZE] = L"";
                SendMessageW(hCombo, CB_GETLBTEXT, index, (LPARAM)fontName);
                SetSelectedFont(hEdit, fontName);
                g_isModified = true;
            }
            return 0;

        case ID_COMBO_SIZE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hCombo = (HWND)lParam;
                wchar_t sizeText[20] = L"";
                int index = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                SendMessageW(hCombo, CB_GETLBTEXT, index, (LPARAM)sizeText);
                int pointSize = _wtoi(sizeText);
                if (pointSize > 0)
                {
                    SetSelectedFontSize(hEdit, pointSize);
                    g_isModified = true;
                }
            }
            return 0;

            // 文件菜单
        case ID_MENU_FILE_OPEN:       OpenNote(hwnd); return 0;
        case ID_MENU_FILE_SAVE:       SaveNote(hwnd); return 0;
        case ID_MENU_FILE_SAVEAS:     SaveNoteAs(hwnd); return 0;
        case ID_MENU_FILE_EXPORT_TXT: ExportTxt(hwnd); return 0;

        case ID_MENU_FILE_EXIT:
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
            return 0;

            // 编辑菜单
        case ID_MENU_EDIT_UNDO:
            SendMessageW(hEdit, EM_UNDO, 0, 0);
            UpdateStatusBar();
            return 0;

        case ID_MENU_EDIT_REDO:
            SendMessageW(hEdit, EM_REDO, 0, 0);
            UpdateStatusBar();
            return 0;

        case ID_MENU_EDIT_CUT:       SendMessageW(hEdit, WM_CUT, 0, 0); return 0;
        case ID_MENU_EDIT_COPY:      SendMessageW(hEdit, WM_COPY, 0, 0); return 0;
        case ID_MENU_EDIT_PASTE:     SendMessageW(hEdit, WM_PASTE, 0, 0); return 0;
        case ID_MENU_EDIT_SELECTALL: SelectAll(); return 0;
        case ID_MENU_EDIT_FIND:      ShowFindWindow(hwnd); return 0;

            // 格式菜单
        case ID_MENU_FORMAT_FONT:
        {
            LOGFONTW lf = {};
            lf.lfHeight = -12;
            wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"微软雅黑");

            CHOOSEFONTW cf = {};
            cf.lStructSize = sizeof(cf);
            cf.hwndOwner = hwnd;
            cf.lpLogFont = &lf;
            cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;

            if (ChooseFontW(&cf))
            {
                SetSelectedFont(hEdit, lf.lfFaceName);
                g_isModified = true;
            }
            return 0;
        }

        case ID_MENU_FORMAT_SIZE:
            if (g_hSizeCombo != NULL)
                SetFocus(g_hSizeCombo);
            return 0;

        case ID_MENU_FORMAT_BOLD:
            ToggleBold(); g_isModified = true; return 0;

        case ID_MENU_FORMAT_ITALIC:
            ToggleItalic(); g_isModified = true; return 0;

        case ID_MENU_FORMAT_UNDERLINE:
            ToggleUnderline(); g_isModified = true; return 0;

        case ID_MENU_FORMAT_STRIKE:
            ToggleStrike(); g_isModified = true; return 0;

            // 【注意】原 ID_MENU_FORMAT_TEXTCOLOR 的 case 已合并到上方 ID_BUTTON_TEXTCOLOR 处

        case ID_MENU_FORMAT_HIGHLIGHT:
            SendMessageW(hwnd, WM_COMMAND,
                MAKEWPARAM(ID_BUTTON_HIGHLIGHT, 0), 0);
            return 0;

        case ID_MENU_FORMAT_CLEAR:
            ClearFormat(); g_isModified = true; return 0;

            // 查看菜单
        case ID_MENU_VIEW_STATUSBAR:
            g_showStatusBar = !g_showStatusBar;
            if (hStatus != NULL)
                ShowWindow(hStatus, g_showStatusBar ? SW_SHOW : SW_HIDE);
            LayoutChildren(hwnd);
            UpdateMenuCheck();
            return 0;

        case ID_MENU_VIEW_WORDWRAP:
            g_wordWrap = !g_wordWrap;
            SendMessageW(hEdit, EM_SETTARGETDEVICE, 0,
                g_wordWrap ? 1 : 0);
            UpdateMenuCheck();
            return 0;

        case ID_MENU_VIEW_ZOOMIN:
        {
            CHARFORMAT2W cf = {};
            cf.cbSize = sizeof(CHARFORMAT2W);
            cf.dwMask = CFM_SIZE;
            SendMessageW(hEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
            int newSize = cf.yHeight / 20 + 2;
            if (newSize > 200) newSize = 200;
            SetSelectedFontSize(hEdit, newSize);
            g_isModified = true;
            return 0;
        }

        case ID_MENU_VIEW_ZOOMOUT:
        {
            CHARFORMAT2W cf = {};
            cf.cbSize = sizeof(CHARFORMAT2W);
            cf.dwMask = CFM_SIZE;
            SendMessageW(hEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
            int newSize = cf.yHeight / 20 - 2;
            if (newSize < 6) newSize = 6;
            SetSelectedFontSize(hEdit, newSize);
            g_isModified = true;
            return 0;
        }

        case ID_MENU_VIEW_ZOOMRESET:
            SetSelectedFontSize(hEdit, 12);
            g_isModified = true;
            return 0;

            // 帮助
        case ID_MENU_HELP_ABOUT:
            MessageBoxW(hwnd,
                L"LightNote 1.0\n\n"
                L"轻量富文本记事本\n"
                L"支持 RTF 保存、高亮、自动保存\n\n"
                L"© 2024",
                L"关于 LightNote",
                MB_OK | MB_ICONINFORMATION);
            return 0;

            // 快捷键
        case ID_ACCEL_SAVE: SaveNote(hwnd); return 0;
        case ID_ACCEL_FIND: ShowFindWindow(hwnd); return 0;
        }

        break;
    }


    case WM_CLOSE:
    {
        AutoSave();

        if (g_isModified)
        {
            int result = MessageBoxW(hwnd,
                L"文件内容已经修改，是否保存？", L"LightNote",
                MB_YESNOCANCEL | MB_ICONQUESTION);

            if (result == IDYES)
            {
                SaveNote(hwnd);
                DestroyWindow(hwnd);
            }
            else if (result == IDNO)
                DestroyWindow(hwnd);
            else if (result == IDCANCEL)
                return 0;
        }
        else
            DestroyWindow(hwnd);

        return 0;
    }


    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER_AUTOSAVE);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}


// ============================================================
// 程序入口
// ============================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PWSTR pCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;

    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    LoadLibraryW(L"Riched20.dll");

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"LightNoteWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(
        L"LightNoteWindow",
        L"LightNote - 学习笔记",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1000, 650,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    std::wstring lastFile = LoadLastFilePathFromRegistry();
    if (!lastFile.empty())
        OpenNoteByPath(lastFile);

    UpdateStatusBar();

    ACCEL accels[] =
    {
        { FCONTROL | FVIRTKEY, 'S', ID_MENU_FILE_SAVE },
        { FCONTROL | FVIRTKEY, 'O', ID_MENU_FILE_OPEN },
        { FCONTROL | FVIRTKEY, 'F', ID_MENU_EDIT_FIND },
        { FCONTROL | FVIRTKEY, 'Z', ID_MENU_EDIT_UNDO },
        { FCONTROL | FVIRTKEY, 'Y', ID_MENU_EDIT_REDO },
        { FCONTROL | FVIRTKEY, 'X', ID_MENU_EDIT_CUT },
        { FCONTROL | FVIRTKEY, 'C', ID_MENU_EDIT_COPY },
        { FCONTROL | FVIRTKEY, 'V', ID_MENU_EDIT_PASTE },
        { FCONTROL | FVIRTKEY, 'A', ID_MENU_EDIT_SELECTALL },
        { FCONTROL | FVIRTKEY, 'B', ID_MENU_FORMAT_BOLD },
        { FCONTROL | FVIRTKEY, 'I', ID_MENU_FORMAT_ITALIC },
        { FCONTROL | FVIRTKEY, 'U', ID_MENU_FORMAT_UNDERLINE },
        { FCONTROL | FVIRTKEY, '0', ID_MENU_VIEW_ZOOMRESET },
        { FCONTROL | FVIRTKEY, VK_OEM_PLUS,  ID_MENU_VIEW_ZOOMIN },
        { FCONTROL | FVIRTKEY, VK_OEM_MINUS, ID_MENU_VIEW_ZOOMOUT }
    };

    HACCEL hAccel = CreateAcceleratorTableW(accels,
        sizeof(accels) / sizeof(accels[0]));

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (!TranslateAcceleratorW(hwnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    DestroyAcceleratorTable(hAccel);
    return (int)msg.wParam;
}