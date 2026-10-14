#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commdlg.h>
#include <richedit.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <cwchar>
#include <cstdlib>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Comdlg32.lib")

// ============================================================
// 控件 ID
// ============================================================

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
#define ID_BUTTON_TEXTCOLOR  1013

#define ID_COMBO_FONT        1101
#define ID_COMBO_SIZE        1102
#define ID_STATUS            1201
#define ID_TOOLBAR           1301

#define ID_ACCEL_SAVE        2001
#define ID_ACCEL_FIND        2002

#define ID_FIND_EDIT         3001
#define ID_FIND_BUTTON       3002

#define ID_TIMER_AUTOSAVE    5001

#define ID_MENU_FILE_NEW           6000
#define ID_MENU_FILE_OPEN          6001
#define ID_MENU_FILE_SAVE          6002
#define ID_MENU_FILE_SAVEAS        6003
#define ID_MENU_FILE_EXPORT_TXT    6004
#define ID_MENU_FILE_EXIT          6005
#define ID_MENU_FILE_OPEN_RECENT   6006

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

// ============================================================
// 全局变量
// ============================================================

std::wstring g_currentFilePath = L"";

bool g_isModified = false;
bool g_isLoadingFile = false;
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

HFONT g_hControlFont = NULL;

std::wstring g_autoSavePath = L"";

// ============================================================
// 前置声明
// ============================================================

void UpdateStatusBar();
void AutoSave();

void SaveLastFilePathToRegistry();
std::wstring LoadLastFilePathFromRegistry();
void ClearLastFilePathFromRegistry();

bool OpenNoteByPath(const std::wstring& path);
bool SaveNote(HWND hwnd);
bool SaveNoteAs(HWND hwnd);
bool ConfirmSaveChanges(HWND hwnd);

bool SavePlainTextToFile(const std::wstring& path);
bool SaveRichTextToFile(const std::wstring& path);
bool IsTxtPath(const std::wstring& path);

HMENU CreateAppMenu();

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

void ShowFindWindow(HWND hwndParent);

void SetSelectedFont(HWND hwndEdit, const wchar_t* fontName);
void SetSelectedFontSize(HWND hwndEdit, int pointSize);
void SetSelectedHighlight(HWND hwndEdit, COLORREF color);

void CreateToolbar(HWND hwnd);
void PositionToolbarCombos(HWND hwnd);
void LayoutChildren(HWND hwnd);

void SetModified(bool modified);
void ChooseTextColor(HWND hwnd);
void ChooseHighlightColor(HWND hwnd);

void NewDocument(HWND hwnd);
void OpenRecentFile(HWND hwnd);

// ============================================================
// RTF 文件读写回调
// ============================================================

static DWORD CALLBACK SaveRtfCallback(
    DWORD_PTR dwCookie,
    LPBYTE pbBuff,
    LONG cb,
    LONG* pcb)
{
    HANDLE hFile = (HANDLE)dwCookie;
    DWORD written = 0;

    if (!WriteFile(
        hFile,
        pbBuff,
        (DWORD)cb,
        &written,
        NULL))
    {
        *pcb = 0;
        return 1;
    }

    *pcb = (LONG)written;
    return 0;
}

static DWORD CALLBACK LoadRtfCallback(
    DWORD_PTR dwCookie,
    LPBYTE pbBuff,
    LONG cb,
    LONG* pcb)
{
    HANDLE hFile = (HANDLE)dwCookie;
    DWORD read = 0;

    if (!ReadFile(
        hFile,
        pbBuff,
        (DWORD)cb,
        &read,
        NULL))
    {
        *pcb = 0;
        return 1;
    }

    *pcb = (LONG)read;
    return 0;
}

// ============================================================
// 修改状态
// ============================================================

void SetModified(bool modified)
{
    g_isModified = modified;

    if (g_hMainWnd != NULL)
    {
        std::wstring title = L"LightNote - 学习笔记";

        if (g_isModified)
            title += L" *";

        SetWindowTextW(g_hMainWnd, title.c_str());
    }

    UpdateStatusBar();
}

// ============================================================
// 文件扩展名判断
// ============================================================

bool IsTxtPath(const std::wstring& path)
{
    if (path.length() < 4)
        return false;

    return _wcsicmp(
        path.c_str() + path.length() - 4,
        L".txt") == 0;
}

// ============================================================
// 保存为 UTF-8 TXT
// ============================================================

bool SavePlainTextToFile(const std::wstring& path)
{
    if (hEdit == NULL)
        return false;

    int length = GetWindowTextLengthW(hEdit);

    if (length < 0)
        return false;

    std::vector<wchar_t> buffer(
        (size_t)length + 1, L'\0');

    int actualLength = GetWindowTextW(
        hEdit,
        buffer.data(),
        length + 1);

    if (actualLength < 0)
        return false;

    int byteCount = 0;

    if (actualLength > 0)
    {
        byteCount = WideCharToMultiByte(
            CP_UTF8,
            0,
            buffer.data(),
            actualLength,
            NULL,
            0,
            NULL,
            NULL);

        if (byteCount <= 0)
            return false;
    }

    std::vector<char> utf8((size_t)byteCount);

    if (byteCount > 0)
    {
        int converted = WideCharToMultiByte(
            CP_UTF8,
            0,
            buffer.data(),
            actualLength,
            utf8.data(),
            byteCount,
            NULL,
            NULL);

        if (converted != byteCount)
            return false;
    }

    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    const BYTE bom[] = { 0xEF, 0xBB, 0xBF };

    DWORD written = 0;

    bool success = WriteFile(
        hFile,
        bom,
        sizeof(bom),
        &written,
        NULL)
        && written == sizeof(bom);

    if (success && byteCount > 0)
    {
        success = WriteFile(
            hFile,
            utf8.data(),
            (DWORD)byteCount,
            &written,
            NULL)
            && written == (DWORD)byteCount;
    }

    CloseHandle(hFile);

    return success;
}

// ============================================================
// 保存为 RTF
// ============================================================

bool SaveRichTextToFile(const std::wstring& path)
{
    if (hEdit == NULL)
        return false;

    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    EDITSTREAM es = {};
    es.dwCookie = (DWORD_PTR)hFile;
    es.pfnCallback = SaveRtfCallback;

    SendMessageW(
        hEdit,
        EM_STREAMOUT,
        SF_RTF,
        (LPARAM)&es);

    CloseHandle(hFile);

    return es.dwError == 0;
}

// ============================================================
// 字体和格式操作
// ============================================================

void SetSelectedFont(
    HWND hwndEdit,
    const wchar_t* fontName)
{
    CHARFORMATW cf = {};
    cf.cbSize = sizeof(CHARFORMATW);
    cf.dwMask = CFM_FACE;

    wcscpy_s(
        cf.szFaceName,
        LF_FACESIZE,
        fontName);

    SendMessageW(
        hwndEdit,
        EM_SETCHARFORMAT,
        SCF_SELECTION,
        (LPARAM)&cf);
}

void SetSelectedFontSize(
    HWND hwndEdit,
    int pointSize)
{
    if (pointSize < 1)
        return;

    CHARFORMATW cf = {};
    cf.cbSize = sizeof(CHARFORMATW);
    cf.dwMask = CFM_SIZE;
    cf.yHeight = pointSize * 20;

    SendMessageW(
        hwndEdit,
        EM_SETCHARFORMAT,
        SCF_SELECTION,
        (LPARAM)&cf);
}

void SetSelectedHighlight(
    HWND hwndEdit,
    COLORREF color)
{
    CHARFORMAT2W cf = {};
    cf.cbSize = sizeof(CHARFORMAT2W);
    cf.dwMask = CFM_BACKCOLOR;

    if (color == CLR_NONE)
    {
        cf.dwEffects = CFE_AUTOBACKCOLOR;
    }
    else
    {
        cf.dwEffects = 0;
        cf.crBackColor = color;
    }

    SendMessageW(
        hwndEdit,
        EM_SETCHARFORMAT,
        SCF_SELECTION,
        (LPARAM)&cf);
}

void SetTextColor(COLORREF color)
{
    CHARFORMAT2W cf = {};
    cf.cbSize = sizeof(CHARFORMAT2W);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;

    SendMessageW(
        hEdit,
        EM_SETCHARFORMAT,
        SCF_SELECTION,
        (LPARAM)&cf);
}

static void ToggleEffect(
    DWORD mask,
    DWORD effect)
{
    CHARFORMAT2W current = {};
    current.cbSize = sizeof(CHARFORMAT2W);
    current.dwMask = mask;

    SendMessageW(
        hEdit,
        EM_GETCHARFORMAT,
        SCF_SELECTION,
        (LPARAM)&current);

    CHARFORMAT2W cf = {};
    cf.cbSize = sizeof(CHARFORMAT2W);
    cf.dwMask = mask;

    if (current.dwEffects & effect)
        cf.dwEffects = 0;
    else
        cf.dwEffects = effect;

    SendMessageW(
        hEdit,
        EM_SETCHARFORMAT,
        SCF_SELECTION,
        (LPARAM)&cf);
}

void ToggleBold()
{
    ToggleEffect(CFM_BOLD, CFE_BOLD);
}

void ToggleItalic()
{
    ToggleEffect(CFM_ITALIC, CFE_ITALIC);
}

void ToggleUnderline()
{
    ToggleEffect(CFM_UNDERLINE, CFE_UNDERLINE);
}

void ToggleStrike()
{
    ToggleEffect(CFM_STRIKEOUT, CFE_STRIKEOUT);
}

void ClearFormat()
{
    CHARFORMAT2W cf = {};
    cf.cbSize = sizeof(CHARFORMAT2W);

    cf.dwMask =
        CFM_FACE |
        CFM_SIZE |
        CFM_BOLD |
        CFM_ITALIC |
        CFM_UNDERLINE |
        CFM_STRIKEOUT |
        CFM_COLOR |
        CFM_BACKCOLOR;

    wcscpy_s(
        cf.szFaceName,
        LF_FACESIZE,
        L"微软雅黑");

    cf.yHeight = 12 * 20;
    cf.crTextColor = RGB(0, 0, 0);
    cf.crBackColor = RGB(255, 255, 255);
    cf.dwEffects = CFE_AUTOBACKCOLOR;

    SendMessageW(
        hEdit,
        EM_SETCHARFORMAT,
        SCF_SELECTION,
        (LPARAM)&cf);
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

void FindTextInEdit(
    HWND hwndEdit,
    const std::wstring& searchText)
{
    if (searchText.empty())
    {
        MessageBoxW(
            g_hFindWindow,
            L"请输入要查找的文字。",
            L"查找",
            MB_OK | MB_ICONINFORMATION);

        return;
    }

    CHARRANGE range = {};

    SendMessageW(
        hwndEdit,
        EM_EXGETSEL,
        0,
        (LPARAM)&range);

    FINDTEXTW ft = {};
    ft.chrg.cpMin = range.cpMax;
    ft.chrg.cpMax = -1;
    ft.lpstrText = searchText.c_str();

    LONG result = (LONG)SendMessageW(
        hwndEdit,
        EM_FINDTEXTW,
        FR_DOWN,
        (LPARAM)&ft);

    if (result == -1)
    {
        ft.chrg.cpMin = 0;
        ft.chrg.cpMax = -1;

        result = (LONG)SendMessageW(
            hwndEdit,
            EM_FINDTEXTW,
            FR_DOWN,
            (LPARAM)&ft);
    }

    if (result != -1)
    {
        CHARRANGE foundRange = {};

        foundRange.cpMin = result;
        foundRange.cpMax =
            result + (LONG)searchText.length();

        SendMessageW(
            hwndEdit,
            EM_EXSETSEL,
            0,
            (LPARAM)&foundRange);

        SendMessageW(
            hwndEdit,
            EM_SCROLLCARET,
            0,
            0);
    }
    else
    {
        MessageBoxW(
            g_hFindWindow,
            L"没有找到指定文字。",
            L"查找",
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

    SendMessageW(
        hEdit,
        EM_EXGETSEL,
        0,
        (LPARAM)&range);

    LONG line = (LONG)SendMessageW(
        hEdit,
        EM_LINEFROMCHAR,
        (WPARAM)range.cpMin,
        0);

    LONG lineStart = (LONG)SendMessageW(
        hEdit,
        EM_LINEINDEX,
        (WPARAM)line,
        0);

    int column = 1;

    if (lineStart >= 0 && range.cpMin >= lineStart)
        column = (int)(range.cpMin - lineStart) + 1;

    int charCount = GetWindowTextLengthW(hEdit);

    wchar_t part1[64] = {};
    wchar_t part2[64] = {};

    swprintf_s(
        part1,
        L"行：%ld  列：%d",
        line + 1,
        column);

    swprintf_s(
        part2,
        L"字符数：%d",
        charCount);

    const wchar_t* part3 =
        g_isModified ? L"已修改" : L"就绪";

    SendMessageW(
        hStatus,
        SB_SETTEXTW,
        0,
        (LPARAM)part1);

    SendMessageW(
        hStatus,
        SB_SETTEXTW,
        1,
        (LPARAM)part2);

    SendMessageW(
        hStatus,
        SB_SETTEXTW,
        2,
        (LPARAM)part3);
}

// ============================================================
// 注册表：记忆上次打开的文件
// ============================================================

void SaveLastFilePathToRegistry()
{
    if (g_currentFilePath.empty())
        return;

    HKEY hKey = NULL;

    if (RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\LightNote",
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        NULL) == ERROR_SUCCESS)
    {
        RegSetValueExW(
            hKey,
            L"LastFile",
            0,
            REG_SZ,
            (const BYTE*)g_currentFilePath.c_str(),
            (DWORD)(
                (g_currentFilePath.length() + 1)
                * sizeof(wchar_t)));

        RegCloseKey(hKey);
    }
}

std::wstring LoadLastFilePathFromRegistry()
{
    std::wstring result;
    HKEY hKey = NULL;

    if (RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\LightNote",
        0,
        KEY_READ,
        &hKey) == ERROR_SUCCESS)
    {
        wchar_t buffer[MAX_PATH * 2] = {};
        DWORD size = sizeof(buffer);
        DWORD type = 0;

        if (RegQueryValueExW(
            hKey,
            L"LastFile",
            NULL,
            &type,
            (LPBYTE)buffer,
            &size) == ERROR_SUCCESS
            && type == REG_SZ)
        {
            result = buffer;
        }

        RegCloseKey(hKey);
    }

    return result;
}

void ClearLastFilePathFromRegistry()
{
    HKEY hKey = NULL;

    if (RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\LightNote",
        0,
        KEY_WRITE,
        &hKey) == ERROR_SUCCESS)
    {
        RegDeleteValueW(hKey, L"LastFile");
        RegCloseKey(hKey);
    }
}

// ============================================================
// 打开文件
// ============================================================

bool OpenNoteByPath(const std::wstring& path)
{
    if (path.empty() || hEdit == NULL)
        return false;

    g_isLoadingFile = true;

    bool success = false;

    if (IsTxtPath(path))
    {
        HANDLE hFile = CreateFileW(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER fileSize = {};

            if (GetFileSizeEx(hFile, &fileSize)
                && fileSize.QuadPart >= 0
                && fileSize.QuadPart <= 100LL * 1024 * 1024)
            {
                DWORD size = (DWORD)fileSize.QuadPart;

                std::vector<char> bytes((size_t)size);

                DWORD totalRead = 0;
                bool readOK = true;

                while (totalRead < size)
                {
                    DWORD chunkRead = 0;

                    if (!ReadFile(
                        hFile,
                        bytes.data() + totalRead,
                        size - totalRead,
                        &chunkRead,
                        NULL)
                        || chunkRead == 0)
                    {
                        readOK = false;
                        break;
                    }

                    totalRead += chunkRead;
                }

                if (readOK)
                {
                    size_t offset = 0;

                    // UTF-8 BOM
                    if (size >= 3
                        && (BYTE)bytes[0] == 0xEF
                        && (BYTE)bytes[1] == 0xBB
                        && (BYTE)bytes[2] == 0xBF)
                    {
                        offset = 3;
                    }

                    int byteCount =
                        (int)(size - offset);

                    if (byteCount == 0)
                    {
                        SetWindowTextW(hEdit, L"");
                        success = true;
                    }
                    else
                    {
                        UINT codePage = CP_UTF8;

                        int charCount = MultiByteToWideChar(
                            CP_UTF8,
                            MB_ERR_INVALID_CHARS,
                            bytes.data() + offset,
                            byteCount,
                            NULL,
                            0);

                        // 无效 UTF-8 时尝试系统 ANSI 编码
                        if (charCount == 0)
                        {
                            codePage = CP_ACP;

                            charCount = MultiByteToWideChar(
                                CP_ACP,
                                0,
                                bytes.data() + offset,
                                byteCount,
                                NULL,
                                0);
                        }

                        if (charCount > 0)
                        {
                            std::vector<wchar_t> text(
                                (size_t)charCount + 1,
                                L'\0');

                            int converted = MultiByteToWideChar(
                                codePage,
                                codePage == CP_UTF8
                                ? MB_ERR_INVALID_CHARS : 0,
                                bytes.data() + offset,
                                byteCount,
                                text.data(),
                                charCount);

                            if (converted == charCount)
                            {
                                SetWindowTextW(
                                    hEdit,
                                    text.data());

                                success = true;
                            }
                        }
                    }
                }
            }

            CloseHandle(hFile);
        }
    }
    else
    {
        HANDLE hFile = CreateFileW(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            // 先选中全部内容，避免插入旧文档中间
            SendMessageW(hEdit, EM_SETSEL, 0, -1);

            EDITSTREAM es = {};
            es.dwCookie = (DWORD_PTR)hFile;
            es.pfnCallback = LoadRtfCallback;

            SendMessageW(
                hEdit,
                EM_STREAMIN,
                SF_RTF,
                (LPARAM)&es);

            success = (es.dwError == 0);

            CloseHandle(hFile);
        }
    }

    g_isLoadingFile = false;

    if (!success)
    {
        MessageBoxW(
            g_hMainWnd,
            L"文件打开失败。请检查文件是否存在、是否损坏，或文件格式是否正确。",
            L"LightNote",
            MB_OK | MB_ICONERROR);

        return false;
    }

    g_currentFilePath = path;

    SetModified(false);

    CHARRANGE cr = { 0, 0 };

    SendMessageW(
        hEdit,
        EM_EXSETSEL,
        0,
        (LPARAM)&cr);

    SendMessageW(
        hEdit,
        EM_EMPTYUNDOBUFFER,
        0,
        0);

    // 处理消息队列中可能残留的 EN_CHANGE，避免刚打开就变脏
    {
        MSG msg;
        while (PeekMessageW(&msg, hEdit, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        SetModified(false);
    }

    SaveLastFilePathToRegistry();
    UpdateStatusBar();

    return true;
}

// ============================================================
// 新建文档
// ============================================================

void NewDocument(HWND hwnd)
{
    if (hEdit == NULL)
        return;

    if (!ConfirmSaveChanges(hwnd))
        return;

    g_isLoadingFile = true;

    SetWindowTextW(hEdit, L"");

    // 恢复默认格式
    CHARFORMATW cf = {};
    cf.cbSize = sizeof(CHARFORMATW);
    cf.dwMask = CFM_FACE | CFM_SIZE;

    wcscpy_s(cf.szFaceName, LF_FACESIZE, L"微软雅黑");
    cf.yHeight = 12 * 20;

    SendMessageW(hEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

    g_isLoadingFile = false;

    g_currentFilePath.clear();

    SetModified(false);

    SendMessageW(hEdit, EM_EMPTYUNDOBUFFER, 0, 0);

    // 清掉队列里残留的 EN_CHANGE
    {
        MSG msg;
        while (PeekMessageW(&msg, hEdit, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        SetModified(false);
    }

    UpdateStatusBar();
}

// ============================================================
// 打开最近的文件
// ============================================================

void OpenRecentFile(HWND hwnd)
{
    std::wstring lastFile = LoadLastFilePathFromRegistry();

    if (lastFile.empty())
    {
        MessageBoxW(
            hwnd,
            L"没有最近打开的文件记录。",
            L"LightNote",
            MB_OK | MB_ICONINFORMATION);

        return;
    }

    if (!ConfirmSaveChanges(hwnd))
        return;

    if (!OpenNoteByPath(lastFile))
    {
        if (MessageBoxW(
            hwnd,
            L"最近的文件已不存在或无法打开。\n是否清除最近文件记录？",
            L"LightNote",
            MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            ClearLastFilePathFromRegistry();
        }
    }
}

// ============================================================
// 文件菜单
// ============================================================

HMENU CreateAppMenu()
{
    HMENU hMenuBar = CreateMenu();

    HMENU hFile = CreatePopupMenu();

    AppendMenuW(
        hFile, MF_STRING,
        ID_MENU_FILE_NEW,
        L"新建(&N)\tCtrl+N");

    AppendMenuW(
        hFile, MF_STRING,
        ID_MENU_FILE_OPEN,
        L"打开(&O)...\tCtrl+O");

    // 打开最近的文件
    {
        std::wstring lastFile = LoadLastFilePathFromRegistry();

        if (!lastFile.empty())
        {
            const wchar_t* name = wcsrchr(lastFile.c_str(), L'\\');
            name = name ? name + 1 : lastFile.c_str();

            std::wstring menuText = L"打开最近的文件：";
            menuText += name;

            AppendMenuW(
                hFile,
                MF_STRING,
                ID_MENU_FILE_OPEN_RECENT,
                menuText.c_str());
        }
        else
        {
            AppendMenuW(
                hFile,
                MF_STRING | MF_GRAYED,
                ID_MENU_FILE_OPEN_RECENT,
                L"打开最近的文件");
        }
    }

    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        hFile, MF_STRING,
        ID_MENU_FILE_SAVE,
        L"保存(&S)\tCtrl+S");

    AppendMenuW(
        hFile, MF_STRING,
        ID_MENU_FILE_SAVEAS,
        L"另存为(&A)...");

    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        hFile, MF_STRING,
        ID_MENU_FILE_EXPORT_TXT,
        L"导出为 TXT(&E)...");

    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        hFile, MF_STRING,
        ID_MENU_FILE_EXIT,
        L"退出(&X)\tAlt+F4");

    AppendMenuW(
        hMenuBar,
        MF_POPUP,
        (UINT_PTR)hFile,
        L"文件(&F)");

    HMENU hEditMenu = CreatePopupMenu();

    AppendMenuW(
        hEditMenu, MF_STRING,
        ID_MENU_EDIT_UNDO,
        L"撤销(&U)\tCtrl+Z");

    AppendMenuW(
        hEditMenu, MF_STRING,
        ID_MENU_EDIT_REDO,
        L"重做(&R)\tCtrl+Y");

    AppendMenuW(hEditMenu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        hEditMenu, MF_STRING,
        ID_MENU_EDIT_CUT,
        L"剪切(&T)\tCtrl+X");

    AppendMenuW(
        hEditMenu, MF_STRING,
        ID_MENU_EDIT_COPY,
        L"复制(&C)\tCtrl+C");

    AppendMenuW(
        hEditMenu, MF_STRING,
        ID_MENU_EDIT_PASTE,
        L"粘贴(&P)\tCtrl+V");

    AppendMenuW(hEditMenu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        hEditMenu, MF_STRING,
        ID_MENU_EDIT_SELECTALL,
        L"全选(&A)\tCtrl+A");

    AppendMenuW(hEditMenu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        hEditMenu, MF_STRING,
        ID_MENU_EDIT_FIND,
        L"查找(&F)...\tCtrl+F");

    AppendMenuW(
        hMenuBar,
        MF_POPUP,
        (UINT_PTR)hEditMenu,
        L"编辑(&E)");

    HMENU hFormat = CreatePopupMenu();

    AppendMenuW(
        hFormat, MF_STRING,
        ID_MENU_FORMAT_FONT,
        L"字体(&F)...");

    AppendMenuW(
        hFormat, MF_STRING,
        ID_MENU_FORMAT_SIZE,
        L"字号(&S)...");

    AppendMenuW(hFormat, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        hFormat, MF_STRING,
        ID_MENU_FORMAT_BOLD,
        L"加粗(&B)\tCtrl+B");

    AppendMenuW(
        hFormat, MF_STRING,
        ID_MENU_FORMAT_ITALIC,
        L"斜体(&I)\tCtrl+I");

    AppendMenuW(
        hFormat, MF_STRING,
        ID_MENU_FORMAT_UNDERLINE,
        L"下划线(&U)\tCtrl+U");

    AppendMenuW(
        hFormat, MF_STRING,
        ID_MENU_FORMAT_STRIKE,
        L"删除线(&K)");

    AppendMenuW(hFormat, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        hFormat, MF_STRING,
        ID_MENU_FORMAT_TEXTCOLOR,
        L"文字颜色(&C)...");

    AppendMenuW(
        hFormat, MF_STRING,
        ID_MENU_FORMAT_HIGHLIGHT,
        L"高亮颜色(&H)...");

    AppendMenuW(hFormat, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        hFormat, MF_STRING,
        ID_MENU_FORMAT_CLEAR,
        L"清除格式(&L)");

    AppendMenuW(
        hMenuBar,
        MF_POPUP,
        (UINT_PTR)hFormat,
        L"格式(&O)");

    HMENU hView = CreatePopupMenu();

    AppendMenuW(
        hView,
        MF_STRING | MF_CHECKED,
        ID_MENU_VIEW_STATUSBAR,
        L"状态栏(&S)");

    AppendMenuW(
        hView,
        MF_STRING,
        ID_MENU_VIEW_WORDWRAP,
        L"自动换行(&W)");

    AppendMenuW(hView, MF_SEPARATOR, 0, NULL);

    AppendMenuW(
        hView, MF_STRING,
        ID_MENU_VIEW_ZOOMIN,
        L"放大字号\tCtrl+=");

    AppendMenuW(
        hView, MF_STRING,
        ID_MENU_VIEW_ZOOMOUT,
        L"缩小字号\tCtrl+-");

    AppendMenuW(
        hView, MF_STRING,
        ID_MENU_VIEW_ZOOMRESET,
        L"重置字号\tCtrl+0");

    AppendMenuW(
        hMenuBar,
        MF_POPUP,
        (UINT_PTR)hView,
        L"查看(&V)");

    HMENU hHelp = CreatePopupMenu();

    AppendMenuW(
        hHelp,
        MF_STRING,
        ID_MENU_HELP_ABOUT,
        L"关于 LightNote(&A)...");

    AppendMenuW(
        hMenuBar,
        MF_POPUP,
        (UINT_PTR)hHelp,
        L"帮助(&H)");

    return hMenuBar;
}

// ============================================================
// 创建工具栏
// ============================================================

void CreateToolbar(HWND hwnd)
{
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES;

    InitCommonControlsEx(&icc);

    hToolbar = CreateWindowExW(
        0,
        TOOLBARCLASSNAMEW,
        NULL,
        WS_CHILD | WS_VISIBLE |
        TBSTYLE_FLAT |
        TBSTYLE_TOOLTIPS |
        CCS_NODIVIDER |
        CCS_NORESIZE |
        CCS_NOPARENTALIGN,
        0, 0, 0, 0,
        hwnd,
        (HMENU)(INT_PTR)ID_TOOLBAR,
        g_hInstance,
        NULL);

    if (hToolbar == NULL)
        return;

    SendMessageW(
        hToolbar,
        TB_BUTTONSTRUCTSIZE,
        sizeof(TBBUTTON),
        0);

    SendMessageW(
        hToolbar,
        TB_SETBITMAPSIZE,
        0,
        MAKELPARAM(0, 0));

    TBBUTTON buttons[] =
    {
        { 0, ID_BUTTON_OPEN, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"打开" },

        { 0, ID_BUTTON_SAVE, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"保存" },

        { 0, 0, TBSTATE_ENABLED,
          BTNS_SEP, {0}, 0, 0 },

        { 0, ID_BUTTON_UNDO, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"撤销" },

        { 0, ID_BUTTON_REDO, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"重做" },

        { 0, 0, TBSTATE_ENABLED,
          BTNS_SEP, {0}, 0, 0 },

        { 0, ID_BUTTON_CUT, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"剪切" },

        { 0, ID_BUTTON_COPY, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"复制" },

        { 0, ID_BUTTON_PASTE, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"粘贴" },

        { 0, 0, TBSTATE_ENABLED,
          BTNS_SEP, {0}, 0, 0 },

        { 0, ID_BUTTON_FIND, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"查找" },

        { 0, 0, TBSTATE_ENABLED,
          BTNS_SEP, {0}, 0, 0 },

        { 0, ID_BUTTON_BOLD, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"B" },

        { 0, ID_BUTTON_ITALIC, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"I" },

        { 0, ID_BUTTON_UNDERLINE, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"U" },

        { 0, 0, TBSTATE_ENABLED,
          BTNS_SEP, {0}, 0, 0 },

        { 0, ID_BUTTON_HIGHLIGHT, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"高亮" },

        { 0, ID_BUTTON_TEXTCOLOR, TBSTATE_ENABLED,
          BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"颜色" }
    };

    SendMessageW(
        hToolbar,
        TB_ADDBUTTONSW,
        sizeof(buttons) / sizeof(TBBUTTON),
        (LPARAM)buttons);

    SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);
}

// ============================================================
// 工具栏字体和字号下拉框布局
// ============================================================

void PositionToolbarCombos(HWND hwnd)
{
    if (hToolbar == NULL
        || g_hFontCombo == NULL
        || g_hSizeCombo == NULL)
    {
        return;
    }

    RECT rcTool = {};

    GetWindowRect(hToolbar, &rcTool);

    MapWindowPoints(
        HWND_DESKTOP,
        hwnd,
        (LPPOINT)&rcTool,
        2);

    int toolH = rcTool.bottom - rcTool.top;

    int fontW = 130;
    int sizeW = 70;
    int gap = 6;
    int rightMargin = 10;

    int sizeX = rcTool.right - rightMargin - sizeW;
    int fontX = sizeX - gap - fontW;
    int comboY = rcTool.top + (toolH - 24) / 2;

    MoveWindow(
        g_hFontCombo,
        fontX,
        comboY,
        fontW,
        200,
        TRUE);

    MoveWindow(
        g_hSizeCombo,
        sizeX,
        comboY,
        sizeW,
        200,
        TRUE);
}

// ============================================================
// 统一布局
// ============================================================

void LayoutChildren(HWND hwnd)
{
    RECT rcClient = {};

    GetClientRect(hwnd, &rcClient);

    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    int toolbarHeight = 0;

    if (hToolbar != NULL)
    {
        SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);

        RECT rcTool = {};
        GetWindowRect(hToolbar, &rcTool);

        toolbarHeight = rcTool.bottom - rcTool.top;
    }

    int statusHeight = 0;

    if (g_showStatusBar && hStatus != NULL)
    {
        SendMessageW(hStatus, WM_SIZE, 0, 0);

        RECT rcStatus = {};
        GetWindowRect(hStatus, &rcStatus);

        statusHeight = rcStatus.bottom - rcStatus.top;
    }

    if (hEdit != NULL)
    {
        MoveWindow(
            hEdit,
            0,
            toolbarHeight,
            width,
            max(0, height - toolbarHeight - statusHeight),
            TRUE);
    }

    PositionToolbarCombos(hwnd);
}

// ============================================================
// 查找窗口过程
// ============================================================

LRESULT CALLBACK FindWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        CreateWindowW(
            L"STATIC",
            L"查找内容：",
            WS_CHILD | WS_VISIBLE,
            15, 18, 80, 25,
            hwnd,
            NULL,
            g_hInstance,
            NULL);

        g_hFindEdit = CreateWindowW(
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE |
            WS_BORDER | ES_AUTOHSCROLL,
            95, 15, 210, 28,
            hwnd,
            (HMENU)(INT_PTR)ID_FIND_EDIT,
            g_hInstance,
            NULL);

        CreateWindowW(
            L"BUTTON",
            L"查找",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            315, 15, 70, 28,
            hwnd,
            (HMENU)(INT_PTR)ID_FIND_BUTTON,
            g_hInstance,
            NULL);

        SetFocus(g_hFindEdit);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_FIND_BUTTON)
        {
            wchar_t buffer[256] = L"";

            GetWindowTextW(
                g_hFindEdit,
                buffer,
                256);

            FindTextInEdit(hEdit, buffer);
            return 0;
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_RETURN)
        {
            wchar_t buffer[256] = L"";

            GetWindowTextW(
                g_hFindEdit,
                buffer,
                256);

            FindTextInEdit(hEdit, buffer);
            return 0;
        }
        break;

    case WM_CLOSE:
    {
        HWND parent = GetParent(hwnd);

        g_hFindWindow = NULL;
        g_hFindEdit = NULL;

        DestroyWindow(hwnd);

        if (parent != NULL)
        {
            EnableWindow(parent, TRUE);
            SetForegroundWindow(parent);
        }

        return 0;
    }
    }

    return DefWindowProcW(
        hwnd, message, wParam, lParam);
}

// ============================================================
// 显示查找窗口
// ============================================================

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
        L"LightNoteFindWindow",
        L"查找",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        420,
        95,
        hwndParent,
        NULL,
        g_hInstance,
        NULL);

    if (g_hFindWindow != NULL)
    {
        EnableWindow(hwndParent, FALSE);

        ShowWindow(g_hFindWindow, SW_SHOW);
        UpdateWindow(g_hFindWindow);
    }
}

// ============================================================
// 打开文件对话框
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

    if (!GetOpenFileNameW(&ofn))
        return;

    // 打开另一个文件前，先处理当前文件的未保存修改
    if (!ConfirmSaveChanges(hwnd))
        return;

    OpenNoteByPath(fileName);
}

// ============================================================
// 另存为
// ============================================================

bool SaveNoteAs(HWND hwnd)
{
    OPENFILENAMEW ofn = {};
    wchar_t fileName[MAX_PATH] = L"我的笔记.rtf";

    if (!g_currentFilePath.empty())
    {
        if (wcscpy_s(
            fileName,
            MAX_PATH,
            g_currentFilePath.c_str()) != 0)
        {
            return false;
        }
    }

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
        return false;

    std::wstring oldPath = g_currentFilePath;
    g_currentFilePath = fileName;

    if (!SaveNote(hwnd))
    {
        g_currentFilePath = oldPath;
        return false;
    }

    return true;
}

// ============================================================
// 保存
// ============================================================

bool SaveNote(HWND hwnd)
{
    if (g_currentFilePath.empty())
        return SaveNoteAs(hwnd);

    bool success = false;

    if (IsTxtPath(g_currentFilePath))
    {
        success = SavePlainTextToFile(g_currentFilePath);
    }
    else
    {
        success = SaveRichTextToFile(g_currentFilePath);
    }

    if (!success)
    {
        MessageBoxW(
            hwnd,
            L"保存失败，请检查文件路径、写入权限或磁盘空间。",
            L"LightNote",
            MB_OK | MB_ICONERROR);

        return false;
    }

    SetModified(false);
    SaveLastFilePathToRegistry();

    return true;
}

// ============================================================
// 自动保存
//
// 自动保存写入单独的恢复文件，不直接覆盖用户的原文件。
// 因此，用户编辑后退出时仍能看到保存确认提示。
// ============================================================

void AutoSave()
{
    if (!g_isModified || hEdit == NULL)
        return;

    if (g_currentFilePath.empty())
    {
        // 新文件尚未指定正式路径时，不自动保存到正式文件
        return;
    }

    g_autoSavePath = g_currentFilePath + L".autosave";

    bool success = false;

    if (IsTxtPath(g_currentFilePath))
    {
        // TXT 的恢复文件仍采用 UTF-8 纯文本
        success = SavePlainTextToFile(g_autoSavePath);
    }
    else
    {
        // RTF 的恢复文件采用 RTF 格式
        success = SaveRichTextToFile(g_autoSavePath);
    }

    // 注意：恢复文件保存成功后，不清除 g_isModified。
    // 原文件尚未通过用户的手动保存确认，因此退出时仍会询问。
    (void)success;
}

// ============================================================
// 导出 TXT
// ============================================================

void ExportTxt(HWND hwnd)
{
    OPENFILENAMEW ofn = {};
    wchar_t fileName[MAX_PATH] = L"导出.txt";

    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hwnd;

    ofn.lpstrFilter =
        L"文本文件 (*.txt)\0*.txt\0"
        L"所有文件 (*.*)\0*.*\0";

    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"txt";

    if (!GetSaveFileNameW(&ofn))
        return;

    if (!SavePlainTextToFile(fileName))
    {
        MessageBoxW(
            hwnd,
            L"TXT 导出失败，请检查路径、权限或磁盘空间。",
            L"LightNote",
            MB_OK | MB_ICONERROR);

        return;
    }

    MessageBoxW(
        hwnd,
        L"已导出为 UTF-8 TXT 纯文本文件。\n字体、颜色、高亮等格式不会保留。",
        L"LightNote",
        MB_OK | MB_ICONINFORMATION);
}

// ============================================================
// 退出前确认保存
// ============================================================

bool ConfirmSaveChanges(HWND hwnd)
{
    if (!g_isModified)
        return true;

    int result = MessageBoxW(
        hwnd,
        L"当前笔记尚未保存，是否保存？",
        L"LightNote",
        MB_YESNOCANCEL | MB_ICONQUESTION);

    if (result == IDCANCEL)
    {
        // 取消：继续编辑
        return false;
    }

    if (result == IDNO)
    {
        // 不保存：允许关闭
        return true;
    }

    if (result == IDYES)
    {
        // 只有保存成功才能继续关闭
        // 如果用户取消另存为，SaveNoteAs 返回 false
        return SaveNote(hwnd);
    }

    return false;
}

// ============================================================
// 更新查看菜单勾选状态
// ============================================================

void UpdateMenuCheck()
{
    if (g_hMainWnd == NULL)
        return;

    HMENU hMenu = GetMenu(g_hMainWnd);

    if (hMenu == NULL)
        return;

    HMENU hView = GetSubMenu(hMenu, 3);

    if (hView == NULL)
        return;

    CheckMenuItem(
        hView,
        ID_MENU_VIEW_STATUSBAR,
        MF_BYCOMMAND |
        (g_showStatusBar ? MF_CHECKED : MF_UNCHECKED));

    CheckMenuItem(
        hView,
        ID_MENU_VIEW_WORDWRAP,
        MF_BYCOMMAND |
        (g_wordWrap ? MF_CHECKED : MF_UNCHECKED));
}

// ============================================================
// 字体颜色选择
// ============================================================

void ChooseTextColor(HWND hwnd)
{
    static COLORREF customColors[16] =
    {
        RGB(0, 0, 0),
        RGB(255, 0, 0),
        RGB(0, 128, 0),
        RGB(0, 0, 255),
        RGB(255, 128, 0),
        RGB(128, 0, 255),
        RGB(0, 128, 255),
        RGB(128, 128, 0),
        RGB(255, 0, 255),
        RGB(0, 255, 255),
        RGB(128, 0, 0),
        RGB(0, 128, 128),
        RGB(192, 192, 192),
        RGB(128, 128, 128),
        RGB(255, 255, 255),
        RGB(255, 255, 0)
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
        SetModified(true);
    }
}

// ============================================================
// 高亮颜色选择
// ============================================================

void ChooseHighlightColor(HWND hwnd)
{
    static COLORREF customColors[16] =
    {
        RGB(255, 255, 0),
        RGB(0, 255, 0),
        RGB(0, 255, 255),
        RGB(255, 0, 255),
        RGB(255, 0, 0),
        RGB(0, 0, 255),
        RGB(255, 128, 0),
        RGB(128, 0, 255),
        RGB(0, 128, 255),
        RGB(128, 255, 0),
        RGB(255, 192, 203),
        RGB(192, 192, 192),
        RGB(255, 255, 255),
        RGB(0, 0, 0),
        RGB(255, 165, 0),
        RGB(173, 216, 230)
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
        SetModified(true);
    }
}

// ============================================================
// 主窗口过程
// ============================================================

LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        g_hMainWnd = hwnd;

        HINSTANCE hInst =
            ((LPCREATESTRUCT)lParam)->hInstance;

        SetMenu(hwnd, CreateAppMenu());

        // Rich Edit 编辑区
        hEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"RichEdit20W",
            L"",
            WS_CHILD | WS_VISIBLE |
            ES_MULTILINE |
            ES_AUTOVSCROLL |
            WS_VSCROLL |
            ES_NOHIDESEL,
            0, 30, 1000, 500,
            hwnd,
            (HMENU)(INT_PTR)1000,
            hInst,
            NULL);

        if (hEdit == NULL)
        {
            MessageBoxW(
                hwnd,
                L"Rich Edit 控件创建失败。",
                L"LightNote",
                MB_OK | MB_ICONERROR);

            return -1;
        }

        // 监听光标选择变化 + 文本内容变化
        // 必须用 ENM_CHANGE，Rich Edit 才会通过 WM_NOTIFY 发 EN_CHANGE
        LRESULT eventMask = SendMessageW(
            hEdit,
            EM_GETEVENTMASK,
            0,
            0);

        SendMessageW(
            hEdit,
            EM_SETEVENTMASK,
            0,
            (LPARAM)(eventMask | ENM_SELCHANGE | ENM_CHANGE));

        CHARFORMATW cf = {};
        cf.cbSize = sizeof(CHARFORMATW);
        cf.dwMask = CFM_FACE | CFM_SIZE;

        wcscpy_s(
            cf.szFaceName,
            LF_FACESIZE,
            L"微软雅黑");

        cf.yHeight = 12 * 20;

        SendMessageW(
            hEdit,
            EM_SETCHARFORMAT,
            SCF_ALL,
            (LPARAM)&cf);

        CreateToolbar(hwnd);

        // 字体下拉框
        g_hFontCombo = CreateWindowW(
            L"COMBOBOX",
            L"",
            WS_CHILD | WS_VISIBLE |
            WS_BORDER |
            CBS_DROPDOWNLIST |
            WS_VSCROLL,
            0, 0, 130, 200,
            hwnd,
            (HMENU)(INT_PTR)ID_COMBO_FONT,
            hInst,
            NULL);

        SendMessageW(
            g_hFontCombo,
            CB_ADDSTRING,
            0,
            (LPARAM)L"微软雅黑");

        SendMessageW(
            g_hFontCombo,
            CB_ADDSTRING,
            0,
            (LPARAM)L"宋体");

        SendMessageW(
            g_hFontCombo,
            CB_ADDSTRING,
            0,
            (LPARAM)L"黑体");

        SendMessageW(
            g_hFontCombo,
            CB_ADDSTRING,
            0,
            (LPARAM)L"Arial");

        SendMessageW(
            g_hFontCombo,
            CB_ADDSTRING,
            0,
            (LPARAM)L"Times New Roman");

        SendMessageW(
            g_hFontCombo,
            CB_SETCURSEL,
            0,
            0);

        // 字号下拉框
        g_hSizeCombo = CreateWindowW(
            L"COMBOBOX",
            L"",
            WS_CHILD | WS_VISIBLE |
            WS_BORDER |
            CBS_DROPDOWNLIST |
            WS_VSCROLL,
            0, 0, 70, 200,
            hwnd,
            (HMENU)(INT_PTR)ID_COMBO_SIZE,
            hInst,
            NULL);

        const wchar_t* sizes[] =
        {
            L"10", L"12", L"14",
            L"16", L"18", L"20",
            L"24", L"28", L"32"
        };

        for (int i = 0; i < 9; i++)
        {
            SendMessageW(
                g_hSizeCombo,
                CB_ADDSTRING,
                0,
                (LPARAM)sizes[i]);
        }

        SendMessageW(
            g_hSizeCombo,
            CB_SETCURSEL,
            1,
            0);

        g_hControlFont = CreateFontW(
            -12, 0, 0, 0,
            FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_SWISS,
            L"Segoe UI");

        SendMessageW(
            g_hFontCombo,
            WM_SETFONT,
            (WPARAM)g_hControlFont,
            TRUE);

        SendMessageW(
            g_hSizeCombo,
            WM_SETFONT,
            (WPARAM)g_hControlFont,
            TRUE);

        // 状态栏
        hStatus = CreateWindowExW(
            0,
            STATUSCLASSNAMEW,
            NULL,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            hwnd,
            (HMENU)(INT_PTR)ID_STATUS,
            hInst,
            NULL);

        if (hStatus != NULL)
        {
            int parts[3] = { 260, 420, -1 };

            SendMessageW(
                hStatus,
                SB_SETPARTS,
                3,
                (LPARAM)parts);
        }

        LayoutChildren(hwnd);
        UpdateStatusBar();

        // 自动恢复文件每 2 秒更新一次
        SetTimer(hwnd, ID_TIMER_AUTOSAVE, 2000, NULL);

        return 0;
    }

    case WM_SIZE:
        LayoutChildren(hwnd);
        return 0;

    case WM_TIMER:
        if (wParam == ID_TIMER_AUTOSAVE)
            AutoSave();

        return 0;

    case WM_NOTIFY:
    {
        LPNMHDR pnm = (LPNMHDR)lParam;

        if (pnm == NULL)
            return 0;

        // 工具栏提示
        if (pnm->code == TTN_GETDISPINFOW
            && pnm->hwndFrom == hToolbar)
        {
            LPNMTTDISPINFOW info =
                (LPNMTTDISPINFOW)lParam;

            static wchar_t tip[128] = L"";
            const wchar_t* text = L"";

            switch ((int)info->hdr.idFrom)
            {
            case ID_BUTTON_OPEN:
                text = L"打开文件 (Ctrl+O)";
                break;

            case ID_BUTTON_SAVE:
                text = L"保存 (Ctrl+S)";
                break;

            case ID_BUTTON_UNDO:
                text = L"撤销 (Ctrl+Z)";
                break;

            case ID_BUTTON_REDO:
                text = L"重做 (Ctrl+Y)";
                break;

            case ID_BUTTON_CUT:
                text = L"剪切 (Ctrl+X)";
                break;

            case ID_BUTTON_COPY:
                text = L"复制 (Ctrl+C)";
                break;

            case ID_BUTTON_PASTE:
                text = L"粘贴 (Ctrl+V)";
                break;

            case ID_BUTTON_FIND:
                text = L"查找 (Ctrl+F)";
                break;

            case ID_BUTTON_BOLD:
                text = L"加粗 (Ctrl+B)";
                break;

            case ID_BUTTON_ITALIC:
                text = L"斜体 (Ctrl+I)";
                break;

            case ID_BUTTON_UNDERLINE:
                text = L"下划线 (Ctrl+U)";
                break;

            case ID_BUTTON_HIGHLIGHT:
                text = L"高亮颜色";
                break;

            case ID_BUTTON_TEXTCOLOR:
                text = L"字体颜色";
                break;
            }

            wcscpy_s(tip, text);
            info->lpszText = tip;

            return TRUE;
        }

        // 编辑区通知
        if (pnm->hwndFrom == hEdit)
        {
            if (pnm->code == EN_SELCHANGE)
            {
                UpdateStatusBar();
                return 0;
            }

            // Rich Edit 通过 ENM_CHANGE 才会发出 EN_CHANGE
            if (pnm->code == EN_CHANGE
                && !g_isLoadingFile)
            {
                SetModified(true);
                return 0;
            }
        }

        return 0;
    }

    case WM_INITMENUPOPUP:
        UpdateMenuCheck();
        return 0;

    case WM_COMMAND:
    {

        // 监听 Rich Edit 文本变化
        if (LOWORD(wParam) == 1000
            && HIWORD(wParam) == EN_CHANGE
            && !g_isLoadingFile)
        {
            SetModified(true);
        }


        switch (LOWORD(wParam))
        {
        case ID_MENU_FILE_NEW:
            NewDocument(hwnd);
            return 0;

        case ID_BUTTON_OPEN:
        case ID_MENU_FILE_OPEN:
            OpenNote(hwnd);
            return 0;

        case ID_MENU_FILE_OPEN_RECENT:
            OpenRecentFile(hwnd);
            return 0;

        case ID_BUTTON_SAVE:
        case ID_MENU_FILE_SAVE:
        case ID_ACCEL_SAVE:
            SaveNote(hwnd);
            return 0;

        case ID_MENU_FILE_SAVEAS:
            SaveNoteAs(hwnd);
            return 0;

        case ID_MENU_FILE_EXPORT_TXT:
            ExportTxt(hwnd);
            return 0;

        case ID_MENU_FILE_EXIT:
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
            return 0;

        case ID_BUTTON_UNDO:
        case ID_MENU_EDIT_UNDO:
            SendMessageW(hEdit, EM_UNDO, 0, 0);
            UpdateStatusBar();
            return 0;

        case ID_BUTTON_REDO:
        case ID_MENU_EDIT_REDO:
            SendMessageW(hEdit, EM_REDO, 0, 0);
            UpdateStatusBar();
            return 0;

        case ID_BUTTON_CUT:
        case ID_MENU_EDIT_CUT:
            SendMessageW(hEdit, WM_CUT, 0, 0);
            return 0;

        case ID_BUTTON_COPY:
        case ID_MENU_EDIT_COPY:
            SendMessageW(hEdit, WM_COPY, 0, 0);
            return 0;

        case ID_BUTTON_PASTE:
        case ID_MENU_EDIT_PASTE:
            SendMessageW(hEdit, WM_PASTE, 0, 0);
            return 0;

        case ID_BUTTON_FIND:
        case ID_MENU_EDIT_FIND:
        case ID_ACCEL_FIND:
            ShowFindWindow(hwnd);
            return 0;

        case ID_MENU_EDIT_SELECTALL:
            SelectAll();
            return 0;

        case ID_BUTTON_BOLD:
        case ID_MENU_FORMAT_BOLD:
            ToggleBold();
            SetModified(true);
            return 0;

        case ID_BUTTON_ITALIC:
        case ID_MENU_FORMAT_ITALIC:
            ToggleItalic();
            SetModified(true);
            return 0;

        case ID_BUTTON_UNDERLINE:
        case ID_MENU_FORMAT_UNDERLINE:
            ToggleUnderline();
            SetModified(true);
            return 0;

        case ID_MENU_FORMAT_STRIKE:
            ToggleStrike();
            SetModified(true);
            return 0;

        case ID_BUTTON_HIGHLIGHT:
        case ID_MENU_FORMAT_HIGHLIGHT:
            ChooseHighlightColor(hwnd);
            return 0;

        case ID_BUTTON_TEXTCOLOR:
        case ID_MENU_FORMAT_TEXTCOLOR:
            ChooseTextColor(hwnd);
            return 0;

        case ID_COMBO_FONT:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int index = (int)SendMessageW(
                    g_hFontCombo,
                    CB_GETCURSEL,
                    0,
                    0);

                if (index >= 0)
                {
                    wchar_t fontName[LF_FACESIZE] = L"";

                    SendMessageW(
                        g_hFontCombo,
                        CB_GETLBTEXT,
                        index,
                        (LPARAM)fontName);

                    SetSelectedFont(hEdit, fontName);
                    SetModified(true);
                }
            }

            return 0;

        case ID_COMBO_SIZE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int index = (int)SendMessageW(
                    g_hSizeCombo,
                    CB_GETCURSEL,
                    0,
                    0);

                if (index >= 0)
                {
                    wchar_t sizeText[20] = L"";

                    SendMessageW(
                        g_hSizeCombo,
                        CB_GETLBTEXT,
                        index,
                        (LPARAM)sizeText);

                    int pointSize = _wtoi(sizeText);

                    if (pointSize > 0)
                    {
                        SetSelectedFontSize(
                            hEdit,
                            pointSize);

                        SetModified(true);
                    }
                }
            }

            return 0;

        case ID_MENU_FORMAT_FONT:
        {
            LOGFONTW lf = {};
            lf.lfHeight = -12;

            wcscpy_s(
                lf.lfFaceName,
                LF_FACESIZE,
                L"微软雅黑");

            CHOOSEFONTW cf = {};
            cf.lStructSize = sizeof(cf);
            cf.hwndOwner = hwnd;
            cf.lpLogFont = &lf;
            cf.Flags =
                CF_SCREENFONTS |
                CF_INITTOLOGFONTSTRUCT;

            if (ChooseFontW(&cf))
            {
                SetSelectedFont(
                    hEdit,
                    lf.lfFaceName);

                SetModified(true);
            }

            return 0;
        }

        case ID_MENU_FORMAT_SIZE:
            if (g_hSizeCombo != NULL)
                SetFocus(g_hSizeCombo);

            return 0;

        case ID_MENU_FORMAT_CLEAR:
            ClearFormat();
            SetModified(true);
            return 0;

        case ID_MENU_VIEW_STATUSBAR:
            g_showStatusBar = !g_showStatusBar;

            if (hStatus != NULL)
            {
                ShowWindow(
                    hStatus,
                    g_showStatusBar ? SW_SHOW : SW_HIDE);
            }

            LayoutChildren(hwnd);
            UpdateMenuCheck();
            return 0;

        case ID_MENU_VIEW_WORDWRAP:
            g_wordWrap = !g_wordWrap;

            // 开启自动换行时不使用水平滚动条
            if (g_wordWrap)
            {
                SendMessageW(
                    hEdit,
                    EM_SETTARGETDEVICE,
                    0,
                    1);
            }
            else
            {
                SendMessageW(
                    hEdit,
                    EM_SETTARGETDEVICE,
                    0,
                    0);
            }

            UpdateMenuCheck();
            return 0;

        case ID_MENU_VIEW_ZOOMIN:
        {
            CHARFORMAT2W cf = {};
            cf.cbSize = sizeof(CHARFORMAT2W);
            cf.dwMask = CFM_SIZE;

            SendMessageW(
                hEdit,
                EM_GETCHARFORMAT,
                SCF_SELECTION,
                (LPARAM)&cf);

            int newSize = (int)(cf.yHeight / 20) + 2;

            if (newSize > 200)
                newSize = 200;

            SetSelectedFontSize(hEdit, newSize);
            SetModified(true);
            return 0;
        }

        case ID_MENU_VIEW_ZOOMOUT:
        {
            CHARFORMAT2W cf = {};
            cf.cbSize = sizeof(CHARFORMAT2W);
            cf.dwMask = CFM_SIZE;

            SendMessageW(
                hEdit,
                EM_GETCHARFORMAT,
                SCF_SELECTION,
                (LPARAM)&cf);

            int newSize = (int)(cf.yHeight / 20) - 2;

            if (newSize < 6)
                newSize = 6;

            SetSelectedFontSize(hEdit, newSize);
            SetModified(true);
            return 0;
        }

        case ID_MENU_VIEW_ZOOMRESET:
            SetSelectedFontSize(hEdit, 12);
            SetModified(true);
            return 0;

        case ID_MENU_HELP_ABOUT:
            MessageBoxW(
                hwnd,
                L"LightNote 1.0\n\n"
                L"轻量富文本记事本\n"
                L"支持 RTF 保存、TXT 导出、文字颜色、高亮、查找和自动恢复文件。\n\n"
                L"© 2026",
                L"关于 LightNote",
                MB_OK | MB_ICONINFORMATION);

            return 0;
        }

        break;
    }

    // ========================================================
    // 关闭窗口
    // ========================================================

    case WM_CLOSE:
    {
        if (!ConfirmSaveChanges(hwnd))
            return 0;

        KillTimer(hwnd, ID_TIMER_AUTOSAVE);

        // 用户确认关闭后，删除自动恢复文件
        if (!g_autoSavePath.empty())
        {
            DeleteFileW(g_autoSavePath.c_str());
            g_autoSavePath.clear();
        }

        DestroyWindow(hwnd);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER_AUTOSAVE);

        if (g_hControlFont != NULL)
        {
            DeleteObject(g_hControlFont);
            g_hControlFont = NULL;
        }

        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam);
}

// ============================================================
// 程序入口
// ============================================================

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow)
{
    (void)hPrevInstance;
    (void)pCmdLine;

    g_hInstance = hInstance;

    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES;

    InitCommonControlsEx(&icc);

    // 加载 Rich Edit
    LoadLibraryW(L"Riched20.dll");

    WNDCLASSW wc = {};

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"LightNoteWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc))
    {
        MessageBoxW(
            NULL,
            L"窗口类注册失败。",
            L"LightNote",
            MB_OK | MB_ICONERROR);

        return 1;
    }

    HWND hwnd = CreateWindowW(
        L"LightNoteWindow",
        L"LightNote - 学习笔记",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1000,
        650,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hwnd == NULL)
    {
        MessageBoxW(
            NULL,
            L"主窗口创建失败。",
            L"LightNote",
            MB_OK | MB_ICONERROR);

        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 不再自动打开上次文件。启动为空白新文档。
    // 用户可以通过“文件 → 打开最近的文件”手动打开。

    UpdateStatusBar();

    ACCEL accels[] =
    {
        { FCONTROL | FVIRTKEY, 'N', ID_MENU_FILE_NEW },
        { FCONTROL | FVIRTKEY, 'S', ID_ACCEL_SAVE },
        { FCONTROL | FVIRTKEY, 'O', ID_MENU_FILE_OPEN },
        { FCONTROL | FVIRTKEY, 'F', ID_ACCEL_FIND },
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
        { FCONTROL | FVIRTKEY, VK_OEM_PLUS, ID_MENU_VIEW_ZOOMIN },
        { FCONTROL | FVIRTKEY, VK_OEM_MINUS, ID_MENU_VIEW_ZOOMOUT }
    };

    HACCEL hAccel = CreateAcceleratorTableW(
        accels,
        sizeof(accels) / sizeof(accels[0]));

    MSG msg = {};

    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        if (hAccel == NULL
            || !TranslateAcceleratorW(hwnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (hAccel != NULL)
        DestroyAcceleratorTable(hAccel);

    return (int)msg.wParam;
}