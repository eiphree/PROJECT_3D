#include "stdafx.h"
#include "D3D12HelloTriangle.h"
#include "Win32Application.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    D3D12HelloTriangle sample(1280, 720, L"3D Scene");
    return Win32Application::Run(&sample, hInstance, nCmdShow);
}
