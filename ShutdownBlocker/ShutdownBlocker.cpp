// ShutdownBlocker.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ShutdownBlocker.h"

#include <shellapi.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

BOOL g_should_block_shutdown = TRUE;
constexpr auto reason_str = L"The user chooses to block the shutdown.";

constexpr UINT u_spawn_msg_id = 0x01u;
constexpr UINT u_notification_area_icon_id = 0x02u;
constexpr UINT u_stop_msg_id = 0x03u;
constexpr UINT u_start_msg_id = 0x04u;

// for notification area icon callback
constexpr UINT WMAPP_NOTIFYCALLBACK = WM_APP + 1;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(PCWSTR pszClassName, WNDPROC lpfnWndProc);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// adds an icon in the notification area (system tray)
BOOL                AddNotificationIcon(HWND hwnd);
// removes the icon from the notification area
BOOL                DeleteNotificationIcon(HWND hWnd);

// shows the menu when the icon is clicked in the notification area
void ShowContextMenu(HWND hWnd, POINT pt);

/*
* Displays a message to the user saying that the program has started
*/
void DisplaySpawnNotification(HWND hWnd);
/*
* Displays a message to the user saying that the block is stopped
*/
void DisplayStopBlockingNotification(HWND hWnd);
/*
* Displays a message to the user saying that the blocking has resumed
*/
void DisplayStartBlockingNotification(HWND hWnd);


// creates the shutdown block reason if it has not been created
bool create_block_reason(HWND hWnd);
// destroys the reason
void destroy_block_reason(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SHUTDOWNBLOCKER, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(szWindowClass, WndProc);

    // Perform application initialization:
    if (!InitInstance (hInstance, SW_HIDE))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SHUTDOWNBLOCKER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(PCWSTR pszClassName, WNDPROC lpfnWndProc)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = lpfnWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInst;
    wcex.hIcon          = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName  = pszClassName;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      DWORD dwErr = GetLastError();
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        g_should_block_shutdown = true;
        create_block_reason(hWnd);

        AddNotificationIcon(hWnd);
        DisplaySpawnNotification(hWnd);

        break;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case ID_STARTBLOCKING:
                if (!g_should_block_shutdown)
                {
                    g_should_block_shutdown = true;
                    create_block_reason(hWnd);

                    DisplayStartBlockingNotification(hWnd);
                }
                break;
            case ID_STOPBLOCKING:
                if (g_should_block_shutdown)
                {
                    g_should_block_shutdown = false;
                    destroy_block_reason(hWnd);

                    DisplayStopBlockingNotification(hWnd);
                }
                break;
            case ID_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case ID_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WMAPP_NOTIFYCALLBACK:
        switch (LOWORD(lParam))
        {
        case WM_CONTEXTMENU:
            {
                POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
                ShowContextMenu(hWnd, pt);
            }
            break;
        }
        break;

    case WM_QUERYENDSESSION:
        return g_should_block_shutdown ? FALSE : TRUE;
    case WM_ENDSESSION:
        return g_should_block_shutdown ? FALSE : TRUE;

    case WM_DESTROY:
        g_should_block_shutdown = false;
        destroy_block_reason(hWnd);
        DeleteNotificationIcon(hWnd);

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

BOOL AddNotificationIcon(HWND hWnd)
{
    NOTIFYICONDATA nid = { sizeof(nid) };

    // add the icon, setting the icon, tooltip, and callback message.
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;

    nid.hWnd = hWnd;
    nid.uID = u_notification_area_icon_id;

    nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
    wcscpy_s(nid.szTip, L"Shutdown Blocker");

    Shell_NotifyIcon(NIM_ADD, &nid);

    // NOTIFYICON_VERSION_4 is prefered
    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

void ShowContextMenu(HWND hWnd, POINT pt)
{
    HMENU hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU1));
    if (hMenu)
    {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu)
        {
            // our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
            SetForegroundWindow(hWnd);

            // respect menu drop alignment
            UINT uFlags = TPM_RIGHTBUTTON;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
            {
                uFlags |= TPM_RIGHTALIGN;
            }
            else
            {
                uFlags |= TPM_LEFTALIGN;
            }            

            AppendMenuW(hSubMenu, MF_SEPARATOR, 0, nullptr);
            if (g_should_block_shutdown)
            {
                AppendMenuW(hSubMenu, MF_STRING, ID_STOPBLOCKING, L"Stop Blocking");
            }
            else
            {
                AppendMenuW(hSubMenu, MF_STRING, ID_STARTBLOCKING, L"Start Blocking");
            }

            TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hWnd, NULL);
        }
        DestroyMenu(hMenu);
    }
}

BOOL DeleteNotificationIcon(HWND hWnd)
{
    NOTIFYICONDATA nid = { sizeof(nid) };

    // add the icon, setting the icon, tooltip, and callback message.
    nid.uFlags = NULL;

    nid.hWnd = hWnd;
    nid.uID = u_notification_area_icon_id;

    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

void DisplaySpawnNotification(HWND hWnd)
{    
    //MessageBoxW(
    //    hWnd,
    //    L"Shutdown Blocker is running. All shutdowns will be blocked for the user's approval. Go to the notification area (system tray) to control the behaviour.",
    //    L"Shutdown Blocker running.",
    //    MB_OK | MB_ICONINFORMATION);

    NOTIFYICONDATA nid = { sizeof(nid) };

    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO | NIIF_LARGE_ICON;
    nid.hWnd = hWnd;
    nid.uID = u_spawn_msg_id;

    wcscpy_s(nid.szInfoTitle, L"Shutdown Blocker running.");
    wcscpy_s(nid.szInfo, L"Shutdown Blocker is running. All shutdowns will be blocked for the user's approval. Go to the notification area (system tray) to control the behaviour.");

    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));

    Shell_NotifyIcon(NIM_ADD, &nid);
}

void DisplayStopBlockingNotification(HWND hWnd)
{
    MessageBoxW(
        hWnd,
        L"Shutdown Blocker is no longer blocking shutdowns.",
        L"Blocking stopped",
        MB_OK | MB_ICONINFORMATION);
}

void DisplayStartBlockingNotification(HWND hWnd)
{
    MessageBoxW(
        hWnd,
        L"Shutdown Blocker resumed blocking shutdowns.",
        L"Blocking resumed",
        MB_OK | MB_ICONINFORMATION);
}

bool create_block_reason(HWND hWnd)
{
    return ShutdownBlockReasonCreate(hWnd, reason_str);
}

void destroy_block_reason(HWND hWnd)
{
    ShutdownBlockReasonDestroy(hWnd);
}
