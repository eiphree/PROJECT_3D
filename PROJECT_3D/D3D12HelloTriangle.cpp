#include "stdafx.h"
#include "D3D12HelloTriangle.h"
#include "SceneVertices.h"
#include "vertex_shader.h"
#include "pixel_shader.h"


HRESULT D3D12HelloTriangle::LoadBitmapFromFile(
	PCWSTR uri, UINT& width, UINT& height, BYTE** ppBits
) {
	HRESULT hr;
	IWICBitmapDecoder* pDecoder = nullptr;
	IWICBitmapFrameDecode* pSource = nullptr;
	IWICFormatConverter* pConverter = nullptr;

	hr = wic_factory->CreateDecoderFromFilename(
		uri, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);

	if (SUCCEEDED(hr)) {
		hr = pDecoder->GetFrame(0, &pSource);
	}

	if (SUCCEEDED(hr)) {
		hr = wic_factory->CreateFormatConverter(&pConverter);
	}


	if (SUCCEEDED(hr)) {
		hr = pConverter->Initialize(
			pSource,
			GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeNone,
			nullptr,
			0.0f,
			WICBitmapPaletteTypeMedianCut
		);
	}

	if (SUCCEEDED(hr)) {
		hr = pConverter->GetSize(&width, &height);
	}

	if (SUCCEEDED(hr)) {
		*ppBits = new BYTE[4 * width * height];
		hr = pConverter->CopyPixels(
			nullptr, 4 * width, 4 * width * height, *ppBits
		);
	}

	if (pDecoder) pDecoder->Release();
	if (pSource) pSource->Release();
	if (pConverter) pConverter->Release();
	return hr;
}

D3D12HelloTriangle::D3D12HelloTriangle(UINT width, UINT height, std::wstring name) :
	m_width(width),
	m_height(height),
	m_title(name),
	m_frameIndex(0),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	m_rtvDescriptorSize(0)
{
	playerPos = { 0.0f, 1.5f, 0.0f };
}

void D3D12HelloTriangle::SetKeyboard(INT key, BOOL val) {
	keyboard[key] = val;
}

void D3D12HelloTriangle::OnInit(HWND hwnd)
{
	// Texture initialization
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&wic_factory)
	);

	ThrowIfFailed(LoadBitmapFromFile(
		TEXT("textures.png"), bmp_width, bmp_height, &bmp_bits
	)
	);

	LoadPipeline(hwnd);
	LoadAssets();
}

// Load the rendering pipeline dependencies.
void D3D12HelloTriangle::LoadPipeline(HWND hwnd)
{
	ComPtr<IDXGIFactory7> factory;
	ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

	ThrowIfFailed(D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&m_device)
	));

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = 0;
	swapChainDesc.Height = 0;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

// Load the sample assets.
void D3D12HelloTriangle::LoadAssets()
{
	// Create an empty root signature.
	{
		D3D12_DESCRIPTOR_RANGE descRange[] = {
			{
				.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
				.NumDescriptors = 1,
				.BaseShaderRegister = 0,
				.RegisterSpace = 0,
				.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
			},
			{
				.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				.NumDescriptors = 1,
				.BaseShaderRegister = 0,
				.RegisterSpace = 0,
				.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
			}
		};

		D3D12_ROOT_PARAMETER rootParam[] = {
			{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable = { 1, &descRange[0] },
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
			},
			{
				.ParameterType =
				  D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable = { 1, &descRange[1]},
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
			 }
		};

		D3D12_STATIC_SAMPLER_DESC tex_sampler_desc = {
			.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			.MipLODBias = 0,
			.MaxAnisotropy = 0,
			.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
			.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
			.MinLOD = 0.0f,
			.MaxLOD = D3D12_FLOAT32_MAX,
			.ShaderRegister = 0,
			.RegisterSpace = 0,
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
		};

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {
			.NumParameters = _countof(rootParam),
			.pParameters = rootParam,
			.NumStaticSamplers = 1,
			.pStaticSamplers = &tex_sampler_desc,
			.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
					D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
					D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
					D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		};

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
	}

	// Create PSO and command list
	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_BLEND_DESC blendDesc = {
			.AlphaToCoverageEnable = FALSE,
			.IndependentBlendEnable = FALSE,
			.RenderTarget = {
				{
				   .BlendEnable = FALSE,
				   .LogicOpEnable = FALSE,
				   .SrcBlend = D3D12_BLEND_ONE,
				   .DestBlend = D3D12_BLEND_ZERO,
				   .BlendOp = D3D12_BLEND_OP_ADD,
				   .SrcBlendAlpha = D3D12_BLEND_ONE,
				   .DestBlendAlpha = D3D12_BLEND_ZERO,
				   .BlendOpAlpha = D3D12_BLEND_OP_ADD,
				   .LogicOp = D3D12_LOGIC_OP_NOOP,
				   .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
				}
			 }
		};

		D3D12_RASTERIZER_DESC rasterizerDesc = {
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_BACK,
			.FrontCounterClockwise = FALSE,
			.DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
			.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
			.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
			.DepthClipEnable = TRUE,
			.MultisampleEnable = FALSE,
			.AntialiasedLineEnable = FALSE,
			.ForcedSampleCount = 0,
			.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {
			.DepthEnable = TRUE,
			.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
			.StencilEnable = FALSE,
			.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
			.StencilWriteMask = D3D12_DEFAULT_STENCIL_READ_MASK,
			.FrontFace = {
				.StencilFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilPassOp = D3D12_STENCIL_OP_KEEP,
				.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
			},
			.BackFace = {
				.StencilFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilPassOp = D3D12_STENCIL_OP_KEEP,
				.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
			}
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = { vs_main, sizeof(vs_main) };
		psoDesc.PS = { ps_main, sizeof(ps_main) };
		psoDesc.RasterizerState = rasterizerDesc;
		psoDesc.BlendState = blendDesc;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc = { .Count = 1, .Quality = 0 };
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.DepthStencilState = depthStencilDesc;

		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

		ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
		ThrowIfFailed(m_commandList->Close());
	}

	// Create vertex buffer view 
	{
		size_t const VERTEX_BUFFER_SIZE = sizeof(vertices_data);
		NUM_VERTICES = VERTEX_BUFFER_SIZE / sizeof(vertex_t);

		D3D12_HEAP_PROPERTIES heapProp = {
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};

		D3D12_RESOURCE_DESC resourceDesc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = VERTEX_BUFFER_SIZE,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, vertices_data, sizeof(vertices_data));
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(vertex_t);
		m_vertexBufferView.SizeInBytes = VERTEX_BUFFER_SIZE;
	}

	// Create the constant buffer.
	{
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 2,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			.NodeMask = 0
		};
		ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));

		const UINT constantBufferSize = sizeof(vs_const_buffer_t);

		D3D12_HEAP_PROPERTIES cbvHeapProps = {
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};

		D3D12_RESOURCE_DESC cbvResourceDesc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = constantBufferSize,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		ThrowIfFailed(m_device->CreateCommittedResource(
			&cbvHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&cbvResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBuffer)));

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = constantBufferSize;
		m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

		XMStoreFloat4x4(&m_constantBufferData.matWorldViewProj, XMMatrixIdentity());

		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
		memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
	}

	// Create depth buffor
	{
		D3D12_DESCRIPTOR_HEAP_DESC depthHeapDesc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		D3D12_HEAP_PROPERTIES depthHeapProps = {
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};

		D3D12_RESOURCE_DESC depthResourceDesc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = m_width,
			.Height = m_height,
			.DepthOrArraySize = 1,
			.MipLevels = 0,
			.Format = DXGI_FORMAT_D32_FLOAT,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
		};

		D3D12_CLEAR_VALUE clearVal = {
			.Format = DXGI_FORMAT_D32_FLOAT,
			.DepthStencil = {.Depth = 1.0f, .Stencil = 0 }
		};

		D3D12_DEPTH_STENCIL_VIEW_DESC stencilViewDesc = {
		   .Format = DXGI_FORMAT_D32_FLOAT,
		   .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		   .Flags = D3D12_DSV_FLAG_NONE,
		   .Texture2D = {}
		};

		ThrowIfFailed(m_device->CreateDescriptorHeap(&depthHeapDesc, IID_PPV_ARGS(&m_depthHeap)));
		ThrowIfFailed(m_device->CreateCommittedResource(
			&depthHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_depthBuffer)));

		m_device->CreateDepthStencilView(
			m_depthBuffer.Get(),
			&stencilViewDesc,
			m_depthHeap->GetCPUDescriptorHandleForHeapStart()
		);
	}

	// Create fence
	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
	}

	// Create texture resources
	{
		// Texture resource
		D3D12_HEAP_PROPERTIES tex_heap_prop = {
		  .Type = D3D12_HEAP_TYPE_DEFAULT,
		  .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		  .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		  .CreationNodeMask = 1,
		  .VisibleNodeMask = 1
		};
		D3D12_RESOURCE_DESC tex_resource_desc = {
		  .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		  .Alignment = 0,
		  .Width = 768, 
		  .Height = 768,
		  .DepthOrArraySize = 1,
		  .MipLevels = 1,
		  .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		  .SampleDesc = {.Count = 1, .Quality = 0 },
		  .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		  .Flags = D3D12_RESOURCE_FLAG_NONE
		};

		m_device->CreateCommittedResource(
			&tex_heap_prop,
			D3D12_HEAP_FLAG_NONE,
			&tex_resource_desc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&texture_resource));

		// Helper buffer for reading texture into GPU
		ComPtr<ID3D12Resource> texture_upload_buffer = nullptr;
		UINT64 RequiredSize = 0;
		auto Desc = texture_resource.Get()->GetDesc();
		ID3D12Device* pDevice = nullptr;
		texture_resource.Get()->GetDevice(
			__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice)
		);
		pDevice->GetCopyableFootprints(
			&Desc, 0, 1, 0, nullptr, nullptr, nullptr, &RequiredSize
		);
		pDevice->Release();

		D3D12_HEAP_PROPERTIES tex_upload_heap_prop = {
		  .Type = D3D12_HEAP_TYPE_UPLOAD,
		  .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		  .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		  .CreationNodeMask = 1,
		  .VisibleNodeMask = 1
		};
		D3D12_RESOURCE_DESC tex_upload_resource_desc = {
		  .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		  .Alignment = 0,
		  .Width = RequiredSize,
		  .Height = 1,
		  .DepthOrArraySize = 1,
		  .MipLevels = 1,
		  .Format = DXGI_FORMAT_UNKNOWN,
		  .SampleDesc = {.Count = 1, .Quality = 0 },
		  .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		  .Flags = D3D12_RESOURCE_FLAG_NONE
		};
		m_device->CreateCommittedResource(
			&tex_upload_heap_prop, D3D12_HEAP_FLAG_NONE,
			&tex_upload_resource_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&texture_upload_buffer)
		);

		D3D12_SUBRESOURCE_DATA texture_data = {
		  .pData = bmp_bits,
		  .RowPitch = (LONG_PTR)(bmp_width * bmp_px_size),
		  .SlicePitch = (LONG_PTR)(bmp_width * bmp_height * bmp_px_size)
		};

		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

		UINT const MAX_SUBRESOURCES = 1;
		RequiredSize = 0;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MAX_SUBRESOURCES];
		UINT NumRows[MAX_SUBRESOURCES];
		UINT64 RowSizesInBytes[MAX_SUBRESOURCES];
		Desc = texture_resource.Get()->GetDesc();
		pDevice = nullptr;
		texture_resource.Get()->GetDevice(
			__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice)
		);
		pDevice->GetCopyableFootprints(
			&Desc, 0, 1, 0, Layouts, NumRows,
			RowSizesInBytes, &RequiredSize
		);
		pDevice->Release();

		BYTE* map_tex_data = nullptr;
		texture_upload_buffer->Map(
			0, nullptr, reinterpret_cast<void**>(&map_tex_data)
		);
		D3D12_MEMCPY_DEST DestData = {
		  .pData = map_tex_data + Layouts[0].Offset,
		  .RowPitch = Layouts[0].Footprint.RowPitch,
		  .SlicePitch =
			SIZE_T(Layouts[0].Footprint.RowPitch) * SIZE_T(NumRows[0])
		};
		for (UINT z = 0; z < Layouts[0].Footprint.Depth; ++z) {
			auto pDestSlice =
				static_cast<UINT8*>(DestData.pData)
				+ DestData.SlicePitch * z;
			auto pSrcSlice =
				static_cast<const UINT8*>(texture_data.pData)
				+ texture_data.SlicePitch * LONG_PTR(z);
			for (UINT y = 0; y < NumRows[0]; ++y) {
				memcpy(
					pDestSlice + DestData.RowPitch * y,
					pSrcSlice + texture_data.RowPitch * LONG_PTR(y),
					static_cast<SIZE_T>(RowSizesInBytes[0])
				);
			}
		}
		texture_upload_buffer->Unmap(0, nullptr);

		D3D12_TEXTURE_COPY_LOCATION Dst = {
		  .pResource = texture_resource.Get(),
		  .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		  .SubresourceIndex = 0
		};
		D3D12_TEXTURE_COPY_LOCATION Src = {
		  .pResource = texture_upload_buffer.Get(),
		  .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
		  .PlacedFootprint = Layouts[0]
		};
		m_commandList->CopyTextureRegion(
			&Dst, 0, 0, 0, &Src, nullptr
		);
		D3D12_RESOURCE_BARRIER tex_upload_resource_barrier = {
		  .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		  .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		  .Transition = {
			.pResource = texture_resource.Get(),
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
			.StateAfter =
			  D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
		};
		m_commandList->ResourceBarrier(
			1, &tex_upload_resource_barrier
		);
		m_commandList->Close();
		ID3D12CommandList* cmd_list = m_commandList.Get();
		m_commandQueue->ExecuteCommandLists(1, &cmd_list);

		// Create SRV for the texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
		  .Format = tex_resource_desc.Format,
		  .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
		  .Shader4ComponentMapping =
			D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		  .Texture2D = {
			.MostDetailedMip = 0,
			.MipLevels = 1,
			.PlaneSlice = 0,
			.ResourceMinLODClamp = 0.0f
		  },
		};
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle =
			m_cbvHeap->
			GetCPUDescriptorHandleForHeapStart();
		cpu_desc_handle.ptr +=
			m_device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		m_device->CreateShaderResourceView(
			texture_resource.Get(), &srv_desc, cpu_desc_handle
		);

		WaitForPreviousFrame();
	}
}

// Update frame-based values.
void D3D12HelloTriangle::OnUpdate()
{
	if (keyboard[0]) {
		angle += ROTSPEEDPERTIMER;
	}
	if (keyboard[2]) {
		angle -= ROTSPEEDPERTIMER;
	}
	if (keyboard[1]) {
		playerPos.x -= (FLOAT)sin(angle) * MOVESPEEDPERTIMER;
		playerPos.z -= -(FLOAT)cos(angle) * MOVESPEEDPERTIMER;
	}
	if (keyboard[3]) {
		playerPos.x -= -(FLOAT)sin(angle) * MOVESPEEDPERTIMER;
		playerPos.z -= (FLOAT)cos(angle) * MOVESPEEDPERTIMER;
	}


	XMMATRIX wvp_matrix;

	wvp_matrix = XMMatrixMultiply(
		XMMatrixTranslation(-playerPos.x, -playerPos.y, -playerPos.z),
		XMMatrixRotationY(angle)
	);


	wvp_matrix = XMMatrixMultiply(
		wvp_matrix,
		XMMatrixPerspectiveFovLH(
			45.0f, static_cast<float>(m_width) / static_cast<float>(m_height), 1.0f, 100.0f
		)
	);
	wvp_matrix = XMMatrixTranspose(wvp_matrix);
	XMStoreFloat4x4(
		&m_constantBufferData.matWorldViewProj, 	
		wvp_matrix
	);
	memcpy(
		m_pCbvDataBegin, 		
		&m_constantBufferData, 		
		sizeof(m_constantBufferData)	
	);
}

// Render the scene.
void D3D12HelloTriangle::OnRender()
{
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void D3D12HelloTriangle::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

void D3D12HelloTriangle::PopulateCommandList()
{

	ThrowIfFailed(m_commandAllocator->Reset());

	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle =
		m_cbvHeap->
		GetGPUDescriptorHandleForHeapStart();
	m_commandList->SetGraphicsRootDescriptorTable(0, gpu_desc_handle);

	gpu_desc_handle.ptr +=
		m_device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
	m_commandList->SetGraphicsRootDescriptorTable(
		1, gpu_desc_handle
	);

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize
	);
	auto depthHandle = m_depthHeap->GetCPUDescriptorHandleForHeapStart();
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &depthHandle);

	// Record commands.
	const float clearColor[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(
		m_depthHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr
	);

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(NUM_VERTICES, 1, 0, 0);

	ThrowIfFailed(m_commandList->Close());
}

void D3D12HelloTriangle::WaitForPreviousFrame()
{
	// Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
