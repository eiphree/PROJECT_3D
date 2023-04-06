#include "stdafx.h"
#include "Win32Application.h"

HWND Win32Application::m_hwnd = nullptr;

int Win32Application::Run(D3D12HelloTriangle* pSample, HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"Project3D";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, static_cast<LONG>(pSample->GetWidth()), static_cast<LONG>(pSample->GetHeight()) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);


    m_hwnd = CreateWindow(
        windowClass.lpszClassName,
        pSample->GetTitle(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,       
        nullptr,        
        hInstance,
        pSample);

    pSample->OnInit(m_hwnd);

    ShowWindow(m_hwnd, nCmdShow);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    pSample->OnDestroy();

    return static_cast<char>(msg.wParam);
}

LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    D3D12HelloTriangle* pSample = reinterpret_cast<D3D12HelloTriangle*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    UINT_PTR IDT_TIMER1 = 1;

    switch (message)
    {
    case WM_CREATE:
        {
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        }
        return 0;


    case WM_PAINT:
        if (pSample)
        {
            pSample->OnUpdate();
            pSample->OnRender();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            PostQuitMessage(0);
        else
        {
            if (wParam == 'A')
                pSample->SetKeyboard(0, TRUE);
            if (wParam == 'W')
                pSample->SetKeyboard(1, TRUE);
            if (wParam == 'D')
                pSample->SetKeyboard(2, TRUE);
            if (wParam == 'S')
                pSample->SetKeyboard(3, TRUE);
        }
        return 0;
     case WM_KEYUP:
         if (wParam == 'A')
             pSample->SetKeyboard(0, FALSE);
         if (wParam == 'W')
             pSample->SetKeyboard(1, FALSE);
         if (wParam == 'D')
             pSample->SetKeyboard(2, FALSE);
         if (wParam == 'S')
             pSample->SetKeyboard(3, FALSE);
         return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
