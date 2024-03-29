// DX11Tutorial01.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "main.h"

#include "Renderer.h"
#include <windowsx.h>

#define MAX_LOADSTRING 100

#define VK_W 0x57
#define VK_S 0x53
#define VK_A 0x41
#define VK_D 0x44

#define VK_UP 0x26
#define VK_DOWN 0x28
#define VK_LEFT 0x25
#define VK_RIGHT 0x27

#define VK_ONE 0x31
#define VK_TWO 0x32
#define VK_THREE 0x33

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HWND g_hWnd = NULL;
Renderer* g_pRenderer = NULL;

bool g_mousePress = false;
int g_mousePrevX = 0;
int g_mousePrevY = 0;

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DX11TUTORIAL01, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    g_pRenderer = new Renderer();
    if (!g_pRenderer->Init(g_hWnd))
    {
       return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DX11TUTORIAL01));

    MSG msg;

    // Main message loop:
    bool exit = false;
    while (!exit)
    {
       if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
       {
          if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
          {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
          }
       }
       if (msg.message == WM_QUIT)
       {
          exit = true;
       }

       g_pRenderer->Update();
       g_pRenderer->Render();
    }

    g_pRenderer->Term();
    delete g_pRenderer;
    g_pRenderer = NULL;

    g_hWnd = NULL;

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DX11TUTORIAL01));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = NULL;
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DX11TUTORIAL01);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   g_hWnd = hWnd;

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
    case WM_SIZE:
       if (g_pRenderer != NULL)
       {
          RECT rc;
          GetClientRect(hWnd, &rc);
          g_pRenderer->Resize(rc.right - rc.left, rc.bottom - rc.top);
       }
       break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_RBUTTONDOWN:
       g_mousePress = true;
       g_mousePrevX = GET_X_LPARAM(lParam);
       g_mousePrevY = GET_Y_LPARAM(lParam);
       break;

    case WM_RBUTTONUP:
       g_mousePress = false;
       break;

    case WM_MOUSEMOVE:
       if (g_mousePress)
       {
          int x = GET_X_LPARAM(lParam);
          int y = GET_Y_LPARAM(lParam);

          g_pRenderer->MouseMove(x - g_mousePrevX, y - g_mousePrevY);

          g_mousePrevX = x;
          g_mousePrevY = y;
       }
       break;

    case WM_MOUSEWHEEL:
       g_pRenderer->MouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
       break;
    
    case WM_KEYDOWN:
        if (GetAsyncKeyState(VK_W) || GetAsyncKeyState(VK_UP))
            g_pRenderer->KeyArrowPress(0, 0.5f);
        if (GetAsyncKeyState(VK_S) || GetAsyncKeyState(VK_DOWN))
            g_pRenderer->KeyArrowPress(0, -0.5f);
        if (GetAsyncKeyState(VK_A) || GetAsyncKeyState(VK_LEFT))
            g_pRenderer->KeyArrowPress(-0.25f, 0);
        if (GetAsyncKeyState(VK_D) || GetAsyncKeyState(VK_RIGHT))
            g_pRenderer->KeyArrowPress(0.25f, 0);

        if (GetAsyncKeyState(VK_ONE))
            g_pRenderer->SwitchLightMode(1);
        if (GetAsyncKeyState(VK_TWO))
            g_pRenderer->SwitchLightMode(10);
        if (GetAsyncKeyState(VK_THREE))
            g_pRenderer->SwitchLightMode(100);


    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
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
