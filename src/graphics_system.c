#include "graphics_system.h"
#include <stdio.h>
#include "fileapi.h"

extern wchar_t* Shaders_Path;

IDXGISwapChain* swap_chain = NULL;
ID3D11Device* device = NULL;
ID3D11DeviceContext* device_context = NULL;

extern ID3D11VertexShader* drawable_vertex_shader;
//extern ID3DBlob* drawable_vertex_shader_blob;
extern void* vertex_shader_data;
extern int vertex_shader_size;

extern ID3D11VertexShader* drawable_pixel_shader;

void graphics_system_init(HWND window_handler)
{
	//Finding best adapter
	IDXGIFactory1* factory = NULL;
	HRESULT hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void**)&factory);
	if (FAILED(hr))
	{
		printf("Failed to create IDXGIFactory\n");
		return -1;
	}

	size_t maxDedicatedMemory = 0;
	IDXGIAdapter1* chosen_adapter = NULL;
	IDXGIAdapter1* adapter = NULL;
	for (int i = 0; factory->lpVtbl->EnumAdapters(factory, i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC adapter_desc;
		memset(&adapter_desc, 0, sizeof(DXGI_ADAPTER_DESC));
		adapter->lpVtbl->GetDesc(adapter, &adapter_desc);
		if (maxDedicatedMemory < adapter_desc.DedicatedVideoMemory)
		{
			maxDedicatedMemory = adapter_desc.DedicatedVideoMemory;
			chosen_adapter = adapter;
		}
	}

	DXGI_SWAP_CHAIN_DESC swap_chain_desc;
	memset(&swap_chain_desc, 0, sizeof(swap_chain_desc));

	swap_chain_desc.BufferCount = 1;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.OutputWindow = window_handler;
	swap_chain_desc.SampleDesc.Count = 4;
	swap_chain_desc.Windowed = TRUE;

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_1 };

	unsigned creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	 hr = D3D11CreateDeviceAndSwapChain(chosen_adapter,
		D3D_DRIVER_TYPE_UNKNOWN, NULL, creationFlags, NULL, NULL, D3D11_SDK_VERSION, &swap_chain_desc,
		&swap_chain, &device, featureLevels, &device_context);

	 //Creating vertex and pixel shader to be shared among all drawables
	 graphics_system_create_vshader(L"data/shaders/VertexShader.hlsl", &drawable_vertex_shader);

	 graphics_system_create_pshader(L"data/shaders/PixelShader.hlsl", &drawable_pixel_shader);
}

int graphics_system_create_vshader(const wchar_t* path_to_source, ID3D11VertexShader** vertex_shader)
{
	//TODO: encapsulate this into header or util file
	//Correct treatment of errors; correct reading (looping until bytes_read == bytes_required)
	HANDLE file_handle = CreateFileA("data/shaders/vs.cso", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file_handle == NULL)
	{
		return;
	}

	vertex_shader_size = GetFileSize(file_handle, NULL);
	
	int bytes_read = 0;
	vertex_shader_data = VirtualAlloc(NULL, vertex_shader_size, MEM_COMMIT, PAGE_READWRITE);
	int read_result = ReadFile(file_handle, vertex_shader_data, vertex_shader_size, &bytes_read, NULL);
	if (read_result == 0)
	{
		return;		
	}

	CloseHandle(file_handle);

	HRESULT shader_creation = device->lpVtbl->CreateVertexShader(device, vertex_shader_data,
		vertex_shader_size, NULL, vertex_shader);
	if (FAILED(shader_creation))
	{
		return -1;
	}

	return 0;
}

int graphics_system_create_pshader(const wchar_t* path_to_source, ID3D11PixelShader** pixel_shader)
{
	//TODO: encapsulate this into header or util file
	//Correct treatment of errors; correct reading (looping until bytes_read == bytes_required)
	HANDLE file_handle = CreateFileA("data/shaders/ps.cso", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file_handle == NULL)
	{
		return;
	}

	int pixel_shader_size = GetFileSize(file_handle, NULL);
	
	int bytes_read = 0;
	void* pixel_shader_data = VirtualAlloc(NULL, pixel_shader_size, MEM_COMMIT, PAGE_READWRITE);
	int read_result = ReadFile(file_handle, pixel_shader_data, pixel_shader_size, &bytes_read, NULL);
	if (read_result == 0)
	{
		return;		
	}

	CloseHandle(file_handle);

	HRESULT shader_creation = device->lpVtbl->CreatePixelShader(device, pixel_shader_data,
		pixel_shader_size, NULL, pixel_shader);
	if (FAILED(shader_creation))
	{
		return -1;
	}
	
	return 0;
}

int graphics_system_create_buffer(void* data, Buffer_type buffer_type, size_t buffer_byte_width,
	ID3D11Buffer** the_buffer)
{
	D3D11_BUFFER_DESC buffer_desc;
	memset(&buffer_desc, 0, sizeof(D3D11_BUFFER_DESC));
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.ByteWidth = buffer_byte_width;
	buffer_desc.BindFlags = buffer_type == Vertex_Buffer ? D3D11_BIND_VERTEX_BUFFER :
		D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA buffer_data;
	memset(&buffer_data, 0, sizeof(D3D11_SUBRESOURCE_DATA));
	buffer_data.pSysMem = data;
	buffer_data.SysMemPitch = 0;
	buffer_data.SysMemSlicePitch = 0;

	HRESULT hr = device->lpVtbl->CreateBuffer(device, &buffer_desc, &buffer_data, the_buffer);
	if (FAILED(hr))
	{
		return -1;
	}

	return 0;
}


void graphics_system_close(void)
{

}