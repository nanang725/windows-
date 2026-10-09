#include <windows.h>
#include <commdlg.h>
#include <richedit.h>
#include <string>

// ==============================
// 控件 ID
// ==============================
#define ID_BUTTON_OPEN      1001
#define ID_BUTTON_SAVE      1002
#define ID_BUTTON_UNDO      1003
#define ID_BUTTON_REDO      1004
#define ID_BUTTON_CUT       1005
#define ID_BUTTON_COPY      1006
#define ID_BUTTON_PASTE     1007
#define ID_BUTTON_FIND      1008

#define ID_COMBO_FONT       1101
#define ID_COMBO_SIZE       1102

// ==============================
// 快捷键 ID
// ==============================
#define ID_ACCEL_SAVE       2001
#define ID_ACCEL_FIND       2002

// ==============================
// 查找窗口控件 ID
// ==============================
#define ID_FIND_EDIT        3001
#define ID_FIND_BUTTON      3002

// ==============================
// 全局变量
// ==============================
std::wstring g_currentFilePath = L"";
bool g_isModified = false;

HWND hEdit = NULL;
HINSTANCE g_hInstance = NULL;

// 查找窗口
HWND g_hFindWindow = NULL;
HWND g_hFindEdit = NULL;


// ============================================================
// 设置选中文字的字体
// ============================================================
void SetSelectedFont(HWND hwndEdit, const wchar_t* fontName)
{
    CHARFORMATW cf = {};

    cf.cbSize = sizeof(CHARFORMATW);
    cf.dwMask = CFM_FACE;

    wcscpy_s(
        cf.szFaceName,
        LF_FACESIZE,
        fontName
    );

    SendMessageW(
        hwndEdit,
        EM_SETCHARFORMAT,
        SCF_SELECTION,
        (LPARAM)&cf
    );
}


// ============================================================
// 设置选中文字的字号
//
// Rich Edit 中字号使用 twips
// 1 磅 = 20 twips
// ============================================================
void SetSelectedFontSize(HWND hwndEdit, int pointSize)
{
    CHARFORMATW cf = {};

    cf.cbSize = sizeof(CHARFORMATW);
    cf.dwMask = CFM_SIZE;

    cf.yHeight = pointSize * 20;

    SendMessageW(
        hwndEdit,
        EM_SETCHARFORMAT,
        SCF_SELECTION,
        (LPARAM)&cf
    );
}


// ============================================================
// 执行查找
// ============================================================
void FindTextInEdit(HWND hwndEdit, const std::wstring& searchText)
{
    if (searchText.empty())
    {
        MessageBoxW(
            g_hFindWindow,
            L"请输入要查找的文字。",
            L"查找",
            MB_OK | MB_ICONINFORMATION
        );

        return;
    }

    // 获取当前选择位置
    CHARRANGE range = {};

    SendMessageW(
        hwndEdit,
        EM_EXGETSEL,
        0,
        (LPARAM)&range
    );

    // 从当前选择结束位置继续查找
    FINDTEXTW ft = {};

    ft.chrg.cpMin = range.cpMax;
    ft.chrg.cpMax = -1;
    ft.lpstrText = searchText.c_str();

    LONG result = (LONG)SendMessageW(
        hwndEdit,
        EM_FINDTEXTW,
        FR_DOWN,
        (LPARAM)&ft
    );

    // 如果后面没找到，则从头开始查找
    if (result == -1)
    {
        ft.chrg.cpMin = 0;
        ft.chrg.cpMax = -1;

        result = (LONG)SendMessageW(
            hwndEdit,
            EM_FINDTEXTW,
            FR_DOWN,
            (LPARAM)&ft
        );
    }

    if (result != -1)
    {
        // 找到了，选中文字
        CHARRANGE foundRange = {};

        foundRange.cpMin = result;
        foundRange.cpMax =
            result + (LONG)searchText.length();

        SendMessageW(
            hwndEdit,
            EM_EXSETSEL,
            0,
            (LPARAM)&foundRange
        );

        // 滚动到选中文字
        SendMessageW(
            hwndEdit,
            EM_SCROLLCARET,
            0,
            0
        );
    }
    else
    {
        MessageBoxW(
            g_hFindWindow,
            L"没有找到指定文字。",
            L"查找",
            MB_OK | MB_ICONINFORMATION
        );
    }
}


// ============================================================
// 查找窗口过程函数
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
        // ==============================
        // 提示文字
        // ==============================
        CreateWindowW(
            L"STATIC",
            L"查找内容：",
            WS_CHILD | WS_VISIBLE,
            15,
            18,
            80,
            25,
            hwnd,
            NULL,
            g_hInstance,
            NULL
        );

        // ==============================
        // 查找输入框
        // ==============================
        g_hFindEdit = CreateWindowW(
            L"EDIT",
            L"",
            WS_CHILD |
            WS_VISIBLE |
            WS_BORDER |
            ES_AUTOHSCROLL,
            95,
            15,
            210,
            28,
            hwnd,
            (HMENU)ID_FIND_EDIT,
            g_hInstance,
            NULL
        );

        // ==============================
        // 查找按钮
        // ==============================
        CreateWindowW(
            L"BUTTON",
            L"查找",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON,
            315,
            15,
            70,
            28,
            hwnd,
            (HMENU)ID_FIND_BUTTON,
            g_hInstance,
            NULL
        );

        // 输入框获得焦点
        SetFocus(g_hFindEdit);

        return 0;
    }


    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_FIND_BUTTON)
        {
            wchar_t buffer[256] = L"";

            GetWindowTextW(
                g_hFindEdit,
                buffer,
                256
            );

            FindTextInEdit(
                hEdit,
                buffer
            );

            return 0;
        }

        // 回车也可以查找
        if (LOWORD(wParam) == ID_FIND_EDIT &&
            HIWORD(wParam) == EN_CHANGE)
        {
            return 0;
        }

        return 0;
    }


    case WM_KEYDOWN:
    {
        if (wParam == VK_RETURN)
        {
            wchar_t buffer[256] = L"";

            GetWindowTextW(
                g_hFindEdit,
                buffer,
                256
            );

            FindTextInEdit(
                hEdit,
                buffer
            );

            return 0;
        }

        break;
    }


    case WM_CLOSE:
    {
        g_hFindWindow = NULL;
        g_hFindEdit = NULL;

        EnableWindow(
            GetParent(hwnd),
            TRUE
        );

        DestroyWindow(hwnd);

        return 0;
    }
    }

    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam
    );
}


// ============================================================
// 创建查找窗口
// ============================================================
void ShowFindWindow(HWND hwndParent)
{
    // 已经打开查找窗口
    if (g_hFindWindow != NULL)
    {
        SetForegroundWindow(g_hFindWindow);
        return;
    }

    // ==============================
    // 注册查找窗口类
    // ==============================
    WNDCLASSW wc = {};

    wc.lpfnWndProc = FindWndProc;
    wc.hInstance = g_hInstance;
    wc.lpszClassName = L"LightNoteFindWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground =
        (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    // ==============================
    // 创建查找窗口
    // ==============================
    g_hFindWindow = CreateWindowW(
        L"LightNoteFindWindow",
        L"查找",
        WS_OVERLAPPED |
        WS_CAPTION |
        WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        420,
        95,
        hwndParent,
        NULL,
        g_hInstance,
        NULL
    );

    if (g_hFindWindow != NULL)
    {
        // 暂时禁止主窗口
        EnableWindow(
            hwndParent,
            FALSE
        );

        ShowWindow(
            g_hFindWindow,
            SW_SHOW
        );

        UpdateWindow(
            g_hFindWindow
        );
    }
}


// ============================================================
// 打开笔记
// ============================================================
void OpenNote(HWND hwnd)
{
    OPENFILENAMEW ofn = {};

    wchar_t fileName[MAX_PATH] = L"";

    ofn.lStructSize =
        sizeof(OPENFILENAMEW);

    ofn.hwndOwner = hwnd;

    ofn.lpstrFilter =
        L"文本文件 (*.txt)\0*.txt\0"
        L"所有文件 (*.*)\0*.*\0";

    ofn.lpstrFile = fileName;

    ofn.nMaxFile = MAX_PATH;

    ofn.Flags =
        OFN_FILEMUSTEXIST |
        OFN_PATHMUSTEXIST;

    ofn.lpstrDefExt = L"txt";


    if (GetOpenFileNameW(&ofn))
    {
        HANDLE hFile = CreateFileW(
            fileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD fileSize =
                GetFileSize(hFile, NULL);

            if (fileSize != INVALID_FILE_SIZE)
            {
                if (fileSize > 0)
                {
                    // ==============================
                    // 读取文件
                    // ==============================
                    char* buffer =
                        new char[fileSize + 2];

                    DWORD bytesRead = 0;

                    ReadFile(
                        hFile,
                        buffer,
                        fileSize,
                        &bytesRead,
                        NULL
                    );

                    buffer[bytesRead] = 0;
                    buffer[bytesRead + 1] = 0;


                    // ==============================
                    // 判断 UTF-8 BOM
                    // ==============================
                    if (bytesRead >= 3 &&
                        (unsigned char)buffer[0] == 0xEF &&
                        (unsigned char)buffer[1] == 0xBB &&
                        (unsigned char)buffer[2] == 0xBF)
                    {
                        int wlen =
                            MultiByteToWideChar(
                                CP_UTF8,
                                0,
                                buffer + 3,
                                bytesRead - 3,
                                NULL,
                                0
                            );

                        wchar_t* wtext =
                            new wchar_t[wlen + 1];

                        MultiByteToWideChar(
                            CP_UTF8,
                            0,
                            buffer + 3,
                            bytesRead - 3,
                            wtext,
                            wlen
                        );

                        wtext[wlen] = L'\0';

                        SetWindowTextW(
                            hEdit,
                            wtext
                        );

                        delete[] wtext;
                    }
                    else
                    {
                        // ==============================
                        // 尝试 UTF-8
                        // ==============================
                        int wlen =
                            MultiByteToWideChar(
                                CP_UTF8,
                                0,
                                buffer,
                                bytesRead,
                                NULL,
                                0
                            );

                        if (wlen > 0)
                        {
                            wchar_t* wtext =
                                new wchar_t[wlen + 1];

                            MultiByteToWideChar(
                                CP_UTF8,
                                0,
                                buffer,
                                bytesRead,
                                wtext,
                                wlen
                            );

                            wtext[wlen] = L'\0';

                            SetWindowTextW(
                                hEdit,
                                wtext
                            );

                            delete[] wtext;
                        }
                        else
                        {
                            // ==============================
                            // UTF-8 失败，使用 ANSI
                            // ==============================
                            wlen =
                                MultiByteToWideChar(
                                    CP_ACP,
                                    0,
                                    buffer,
                                    bytesRead,
                                    NULL,
                                    0
                                );

                            wchar_t* wtext =
                                new wchar_t[wlen + 1];

                            MultiByteToWideChar(
                                CP_ACP,
                                0,
                                buffer,
                                bytesRead,
                                wtext,
                                wlen
                            );

                            wtext[wlen] = L'\0';

                            SetWindowTextW(
                                hEdit,
                                wtext
                            );

                            delete[] wtext;
                        }
                    }

                    delete[] buffer;
                }
                else
                {
                    // 空文件
                    SetWindowTextW(
                        hEdit,
                        L""
                    );
                }

                // ==============================
                // 记录当前文件路径
                // ==============================
                g_currentFilePath = fileName;

                // 打开文件不属于用户修改
                g_isModified = false;
            }

            CloseHandle(hFile);
        }
        else
        {
            MessageBoxW(
                hwnd,
                L"文件打开失败！",
                L"LightNote",
                MB_OK | MB_ICONERROR
            );
        }
    }
}


// ============================================================
// 保存笔记
//
// 第一次保存：弹出另存为
// 第二次以后：直接覆盖原文件
// ============================================================
void SaveNote(HWND hwnd)
{
    // ==============================
    // 第一次保存
    // ==============================
    if (g_currentFilePath.empty())
    {
        OPENFILENAMEW ofn = {};

        wchar_t fileName[MAX_PATH] =
            L"我的笔记.txt";

        ofn.lStructSize =
            sizeof(OPENFILENAMEW);

        ofn.hwndOwner = hwnd;

        ofn.lpstrFilter =
            L"文本文件 (*.txt)\0*.txt\0"
            L"所有文件 (*.*)\0*.*\0";

        ofn.lpstrFile = fileName;

        ofn.nMaxFile = MAX_PATH;

        ofn.Flags =
            OFN_OVERWRITEPROMPT |
            OFN_PATHMUSTEXIST;

        ofn.lpstrDefExt = L"txt";


        // 用户取消保存
        if (!GetSaveFileNameW(&ofn))
        {
            return;
        }

        // 记住第一次保存的路径
        g_currentFilePath = fileName;
    }


    // ==============================
    // 获取编辑框文字
    // ==============================
    int length =
        GetWindowTextLengthW(hEdit);

    wchar_t* text =
        new wchar_t[length + 1];

    GetWindowTextW(
        hEdit,
        text,
        length + 1
    );


    // ==============================
    // UTF-16 → UTF-8
    // ==============================
    int utf8Len =
        WideCharToMultiByte(
            CP_UTF8,
            0,
            text,
            length,
            NULL,
            0,
            NULL,
            NULL
        );

    char* utf8Text =
        new char[utf8Len + 1];

    WideCharToMultiByte(
        CP_UTF8,
        0,
        text,
        length,
        utf8Text,
        utf8Len,
        NULL,
        NULL
    );


    // ==============================
    // 创建 / 覆盖文件
    // ==============================
    HANDLE hFile = CreateFileW(
        g_currentFilePath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );


    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten = 0;

        // ==============================
        // 写入 UTF-8 BOM
        // ==============================
        unsigned char bom[] =
        {
            0xEF,
            0xBB,
            0xBF
        };

        WriteFile(
            hFile,
            bom,
            3,
            &bytesWritten,
            NULL
        );


        // ==============================
        // 写入正文
        // ==============================
        if (utf8Len > 0)
        {
            WriteFile(
                hFile,
                utf8Text,
                utf8Len,
                &bytesWritten,
                NULL
            );
        }


        CloseHandle(hFile);

        // ==============================
        // 保存成功
        // ==============================
        g_isModified = false;

        MessageBoxW(
            hwnd,
            L"笔记保存成功！",
            L"LightNote",
            MB_OK | MB_ICONINFORMATION
        );
    }
    else
    {
        MessageBoxW(
            hwnd,
            L"文件保存失败！",
            L"LightNote",
            MB_OK | MB_ICONERROR
        );
    }


    delete[] text;
    delete[] utf8Text;
}


// ============================================================
// 窗口过程函数
// ============================================================
LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
        // ========================================================
        // 创建窗口
        // ========================================================
    case WM_CREATE:
    {
        HINSTANCE hInst =
            ((LPCREATESTRUCT)lParam)->hInstance;

        // ==============================
        // 打开按钮
        // ==============================
        CreateWindowW(
            L"BUTTON",
            L"打开",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON |
            BS_FLAT,
            8,
            18,
            50,
            24,
            hwnd,
            (HMENU)ID_BUTTON_OPEN,
            hInst,
            NULL
        );


        // ==============================
        // 保存按钮
        // ==============================
        CreateWindowW(
            L"BUTTON",
            L"保存",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON |
            BS_FLAT,
            66,
            18,
            50,
            24,
            hwnd,
            (HMENU)ID_BUTTON_SAVE,
            hInst,
            NULL
        );


        // ==============================
        // 撤销按钮
        // ==============================
        CreateWindowW(
            L"BUTTON",
            L"撤销",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON |
            BS_FLAT,
            124,
            18,
            50,
            24,
            hwnd,
            (HMENU)ID_BUTTON_UNDO,
            hInst,
            NULL
        );


        // ==============================
        // 重做按钮
        // ==============================
        CreateWindowW(
            L"BUTTON",
            L"重做",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON |
            BS_FLAT,
            182,
            18,
            50,
            24,
            hwnd,
            (HMENU)ID_BUTTON_REDO,
            hInst,
            NULL
        );


        // ==============================
        // 剪切按钮
        // ==============================
        CreateWindowW(
            L"BUTTON",
            L"剪切",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON |
            BS_FLAT,
            240,
            18,
            50,
            24,
            hwnd,
            (HMENU)ID_BUTTON_CUT,
            hInst,
            NULL
        );


        // ==============================
        // 复制按钮
        // ==============================
        CreateWindowW(
            L"BUTTON",
            L"复制",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON |
            BS_FLAT,
            298,
            18,
            50,
            24,
            hwnd,
            (HMENU)ID_BUTTON_COPY,
            hInst,
            NULL
        );


        // ==============================
        // 粘贴按钮
        // ==============================
        CreateWindowW(
            L"BUTTON",
            L"粘贴",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON |
            BS_FLAT,
            356,
            18,
            50,
            24,
            hwnd,
            (HMENU)ID_BUTTON_PASTE,
            hInst,
            NULL
        );


        // ==============================
        // 查找按钮
        // ==============================
        CreateWindowW(
            L"BUTTON",
            L"查找",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON |
            BS_FLAT,
            414,
            18,
            50,
            24,
            hwnd,
            (HMENU)ID_BUTTON_FIND,
            hInst,
            NULL
        );


        // ==============================
        // 字体下拉框
        // ==============================
        HWND hFontCombo = CreateWindowW(
            L"COMBOBOX",
            L"",
            WS_CHILD |
            WS_VISIBLE |
            WS_BORDER |
            CBS_DROPDOWNLIST |
            WS_VSCROLL,
            480,
            18,
            140,
            180,
            hwnd,
            (HMENU)ID_COMBO_FONT,
            hInst,
            NULL
        );


        // ==============================
        // 添加字体
        // ==============================
        SendMessageW(
            hFontCombo,
            CB_ADDSTRING,
            0,
            (LPARAM)L"微软雅黑"
        );

        SendMessageW(
            hFontCombo,
            CB_ADDSTRING,
            0,
            (LPARAM)L"宋体"
        );

        SendMessageW(
            hFontCombo,
            CB_ADDSTRING,
            0,
            (LPARAM)L"黑体"
        );

        SendMessageW(
            hFontCombo,
            CB_ADDSTRING,
            0,
            (LPARAM)L"Arial"
        );

        SendMessageW(
            hFontCombo,
            CB_ADDSTRING,
            0,
            (LPARAM)L"Times New Roman"
        );

        SendMessageW(
            hFontCombo,
            CB_SETCURSEL,
            0,
            0
        );


        // ==============================
        // 字号下拉框
        // ==============================
        HWND hSizeCombo = CreateWindowW(
            L"COMBOBOX",
            L"",
            WS_CHILD |
            WS_VISIBLE |
            WS_BORDER |
            CBS_DROPDOWNLIST |
            WS_VSCROLL,
            630,
            18,
            70,
            180,
            hwnd,
            (HMENU)ID_COMBO_SIZE,
            hInst,
            NULL
        );


        // ==============================
        // 添加字号
        // ==============================
        const wchar_t* sizes[] =
        {
            L"10",
            L"12",
            L"14",
            L"16",
            L"18",
            L"20",
            L"24",
            L"28",
            L"32"
        };

        for (int i = 0; i < 9; i++)
        {
            SendMessageW(
                hSizeCombo,
                CB_ADDSTRING,
                0,
                (LPARAM)sizes[i]
            );
        }

        // 默认 12
        SendMessageW(
            hSizeCombo,
            CB_SETCURSEL,
            1,
            0
        );


        // ==============================
        // 设置按钮字体
        // ==============================
        HFONT hFont = CreateFontW(
            -12,
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_SWISS,
            L"Segoe UI"
        );


        // 给所有按钮和下拉框设置字体
        SendDlgItemMessageW(
            hwnd,
            ID_BUTTON_OPEN,
            WM_SETFONT,
            (WPARAM)hFont,
            TRUE
        );

        SendDlgItemMessageW(
            hwnd,
            ID_BUTTON_SAVE,
            WM_SETFONT,
            (WPARAM)hFont,
            TRUE
        );

        SendDlgItemMessageW(
            hwnd,
            ID_BUTTON_UNDO,
            WM_SETFONT,
            (WPARAM)hFont,
            TRUE
        );

        SendDlgItemMessageW(
            hwnd,
            ID_BUTTON_REDO,
            WM_SETFONT,
            (WPARAM)hFont,
            TRUE
        );

        SendDlgItemMessageW(
            hwnd,
            ID_BUTTON_CUT,
            WM_SETFONT,
            (WPARAM)hFont,
            TRUE
        );

        SendDlgItemMessageW(
            hwnd,
            ID_BUTTON_COPY,
            WM_SETFONT,
            (WPARAM)hFont,
            TRUE
        );

        SendDlgItemMessageW(
            hwnd,
            ID_BUTTON_PASTE,
            WM_SETFONT,
            (WPARAM)hFont,
            TRUE
        );

        SendDlgItemMessageW(
            hwnd,
            ID_BUTTON_FIND,
            WM_SETFONT,
            (WPARAM)hFont,
            TRUE
        );

        SendDlgItemMessageW(
            hwnd,
            ID_COMBO_FONT,
            WM_SETFONT,
            (WPARAM)hFont,
            TRUE
        );

        SendDlgItemMessageW(
            hwnd,
            ID_COMBO_SIZE,
            WM_SETFONT,
            (WPARAM)hFont,
            TRUE
        );

        return 0;
    }


    // ========================================================
    // 窗口大小改变
    // ========================================================
    case WM_SIZE:
    {
        int width =
            LOWORD(lParam);

        int height =
            HIWORD(lParam);


        if (hEdit != NULL)
        {
            MoveWindow(
                hEdit,
                0,
                55,
                width,
                height - 55,
                TRUE
            );
        }

        return 0;
    }


    // ========================================================
    // 命令消息
    // ========================================================
    case WM_COMMAND:
    {
        // ==============================
        // 编辑框内容发生变化
        // ==============================
        if (HIWORD(wParam) == EN_CHANGE &&
            (HWND)lParam == hEdit)
        {
            g_isModified = true;
        }


        switch (LOWORD(wParam))
        {
            // ====================================================
            // 打开
            // ====================================================
        case ID_BUTTON_OPEN:

            OpenNote(hwnd);

            return 0;


            // ====================================================
            // 保存
            // ====================================================
        case ID_BUTTON_SAVE:

            SaveNote(hwnd);

            return 0;


            // ====================================================
            // 撤销
            // ====================================================
        case ID_BUTTON_UNDO:

            SendMessageW(
                hEdit,
                EM_UNDO,
                0,
                0
            );

            return 0;


            // ====================================================
            // 重做
            // ====================================================
        case ID_BUTTON_REDO:

            SendMessageW(
                hEdit,
                EM_REDO,
                0,
                0
            );

            return 0;


            // ====================================================
            // 剪切
            // ====================================================
        case ID_BUTTON_CUT:

            SendMessageW(
                hEdit,
                WM_CUT,
                0,
                0
            );

            return 0;


            // ====================================================
            // 复制
            // ====================================================
        case ID_BUTTON_COPY:

            SendMessageW(
                hEdit,
                WM_COPY,
                0,
                0
            );

            return 0;


            // ====================================================
            // 粘贴
            // ====================================================
        case ID_BUTTON_PASTE:

            SendMessageW(
                hEdit,
                WM_PASTE,
                0,
                0
            );

            return 0;


            // ====================================================
            // 查找
            // ====================================================
        case ID_BUTTON_FIND:

            ShowFindWindow(hwnd);

            return 0;


            // ====================================================
            // 字体
            // ====================================================
        case ID_COMBO_FONT:

            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hCombo =
                    (HWND)lParam;

                int index =
                    (int)SendMessageW(
                        hCombo,
                        CB_GETCURSEL,
                        0,
                        0
                    );

                wchar_t fontName[LF_FACESIZE] = L"";

                SendMessageW(
                    hCombo,
                    CB_GETLBTEXT,
                    index,
                    (LPARAM)fontName
                );

                SetSelectedFont(
                    hEdit,
                    fontName
                );

                g_isModified = true;
            }

            return 0;


            // ====================================================
            // 字号
            // ====================================================
        case ID_COMBO_SIZE:

            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hCombo =
                    (HWND)lParam;

                wchar_t sizeText[20] = L"";

                int index =
                    (int)SendMessageW(
                        hCombo,
                        CB_GETCURSEL,
                        0,
                        0
                    );

                SendMessageW(
                    hCombo,
                    CB_GETLBTEXT,
                    index,
                    (LPARAM)sizeText
                );

                int pointSize =
                    _wtoi(sizeText);

                if (pointSize > 0)
                {
                    SetSelectedFontSize(
                        hEdit,
                        pointSize
                    );

                    g_isModified = true;
                }
            }

            return 0;


            // ====================================================
            // Ctrl + S
            // ====================================================
        case ID_ACCEL_SAVE:

            SaveNote(hwnd);

            return 0;


            // ====================================================
            // Ctrl + F
            // ====================================================
        case ID_ACCEL_FIND:

            ShowFindWindow(hwnd);

            return 0;
        }

        break;
    }


    // ========================================================
    // 关闭窗口
    // ========================================================
    case WM_CLOSE:
    {
        if (g_isModified)
        {
            int result =
                MessageBoxW(
                    hwnd,
                    L"文件内容已经修改，是否保存？",
                    L"LightNote",
                    MB_YESNOCANCEL |
                    MB_ICONQUESTION
                );


            // ==============================
            // 是
            // ==============================
            if (result == IDYES)
            {
                SaveNote(hwnd);

                DestroyWindow(hwnd);
            }


            // ==============================
            // 否
            // ==============================
            else if (result == IDNO)
            {
                DestroyWindow(hwnd);
            }


            // ==============================
            // 取消
            // ==============================
            else if (result == IDCANCEL)
            {
                return 0;
            }
        }
        else
        {
            DestroyWindow(hwnd);
        }

        return 0;
    }


    // ========================================================
    // 窗口销毁
    // ========================================================
    case WM_DESTROY:
    {
        PostQuitMessage(0);

        return 0;
    }
    }


    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam
    );
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
    // 保存全局实例句柄
    g_hInstance = hInstance;


    // ========================================================
    // 加载 Rich Edit 动态库
    // ========================================================
    LoadLibraryW(L"Riched20.dll");


    // ========================================================
    // 注册窗口类
    // ========================================================
    WNDCLASSW wc = {};

    wc.lpfnWndProc =
        WndProc;

    wc.hInstance =
        hInstance;

    wc.lpszClassName =
        L"LightNoteWindow";

    wc.hCursor =
        LoadCursor(
            NULL,
            IDC_ARROW
        );

    wc.hbrBackground =
        (HBRUSH)(COLOR_WINDOW + 1);


    RegisterClassW(&wc);


    // ========================================================
    // 创建主窗口
    // ========================================================
    HWND hwnd = CreateWindowW(
        L"LightNoteWindow",
        L"LightNote - 学习笔记",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900,
        600,
        NULL,
        NULL,
        hInstance,
        NULL
    );


    // ========================================================
    // 创建 Rich Edit 编辑框
    // ========================================================
    hEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"RichEdit20W",
        L"",
        WS_CHILD |
        WS_VISIBLE |
        ES_MULTILINE |
        ES_AUTOVSCROLL |
        ES_AUTOHSCROLL |
        WS_VSCROLL |
        WS_HSCROLL |
        ES_NOHIDESEL,
        0,
        55,
        900,
        545,
        hwnd,
        NULL,
        hInstance,
        NULL
    );


    // ========================================================
    // 设置编辑框默认字体
    // ========================================================
    CHARFORMATW cf = {};

    cf.cbSize =
        sizeof(CHARFORMATW);

    cf.dwMask =
        CFM_FACE |
        CFM_SIZE;

    wcscpy_s(
        cf.szFaceName,
        LF_FACESIZE,
        L"微软雅黑"
    );

    cf.yHeight =
        12 * 20;

    SendMessageW(
        hEdit,
        EM_SETCHARFORMAT,
        SCF_ALL,
        (LPARAM)&cf
    );


    // ========================================================
    // 显示窗口
    // ========================================================
    ShowWindow(
        hwnd,
        nCmdShow
    );

    UpdateWindow(hwnd);


    // ========================================================
    // 创建快捷键
    //
    // Ctrl + S = 保存
    // Ctrl + F = 查找
    // ========================================================
    ACCEL accels[2] = {};

    // Ctrl + S
    accels[0].fVirt =
        FCONTROL |
        FVIRTKEY;

    accels[0].key =
        'S';

    accels[0].cmd =
        ID_ACCEL_SAVE;


    // Ctrl + F
    accels[1].fVirt =
        FCONTROL |
        FVIRTKEY;

    accels[1].key =
        'F';

    accels[1].cmd =
        ID_ACCEL_FIND;


    HACCEL hAccel =
        CreateAcceleratorTableW(
            accels,
            2
        );


    // ========================================================
    // 消息循环
    // ========================================================
    MSG msg = {};

    while (
        GetMessageW(
            &msg,
            NULL,
            0,
            0
        ))
    {
        if (!TranslateAcceleratorW(
            hwnd,
            hAccel,
            &msg))
        {
            TranslateMessage(
                &msg
            );

            DispatchMessageW(
                &msg
            );
        }
    }


    // ========================================================
    // 清理
    // ========================================================
    DestroyAcceleratorTable(
        hAccel
    );


    return (int)msg.wParam;
}