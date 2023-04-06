#pragma once

#include "ExceptionHandler.h"
#include <wincodec.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class D3D12HelloTriangle
{
public:
    D3D12HelloTriangle(UINT width, UINT height, std::wstring name);

    void OnInit(HWND hwnd);
    void OnUpdate();
    void OnRender();
    void OnDestroy();

    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }
    const WCHAR* GetTitle() const { return m_title.c_str(); }

    void SetKeyboard(INT key, BOOL val);

    struct vertex_t {
        FLOAT position[3];
        FLOAT color[4];
        FLOAT tex_coord[2];
    };

private:

    struct vs_const_buffer_t {
        XMFLOAT4X4 matWorldViewProj;
        XMFLOAT4 padding[(256 - sizeof(XMFLOAT4X4)) / sizeof(XMFLOAT4)];
    };

    struct playesPos_t {
        FLOAT x;
        FLOAT y;
        FLOAT z;
    };

    UINT m_width;
    UINT m_height;
    std::wstring m_title;

    const FLOAT ROTSPEEDPERTIMER = 0.02f;
    const FLOAT MOVESPEEDPERTIMER = 0.05f;
    static const UINT FrameCount = 2;
    size_t const VERTEX_SIZE = sizeof(vertex_t) / sizeof(FLOAT);

    BOOL keyboard[4] = { FALSE, FALSE, FALSE, FALSE };
    playesPos_t playerPos;
    FLOAT angle = 0.0f;
    UINT NUM_VERTICES = 0;

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    UINT m_rtvDescriptorSize;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    ComPtr<ID3D12Resource> m_constantBuffer;
    vs_const_buffer_t m_constantBufferData;
    UINT8* m_pCbvDataBegin;
    ComPtr<ID3D12Resource> m_depthBuffer;
    ComPtr<ID3D12DescriptorHeap> m_depthHeap;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    // Texture resources
    IWICImagingFactory* wic_factory = nullptr;
    UINT const bmp_px_size = 4;
    UINT bmp_width = 0, bmp_height = 0;
    BYTE* bmp_bits = nullptr;
    ComPtr<ID3D12Resource> texture_resource;
    ComPtr<ID3D12Resource> m_texture;
    HRESULT LoadBitmapFromFile(PCWSTR uri, UINT& width, UINT& height, BYTE** ppBits);

    void LoadPipeline(HWND hwnd);
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();
};