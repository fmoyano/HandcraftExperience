#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdbool.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cglm/cglm.h>
#include <string.h>
#include "drawable_2D.h"
#include "graphics_system.h"

extern IDXGISwapChain* swap_chain;
extern ID3D11Device* device;
extern ID3D11DeviceContext* device_context;

wchar_t* Texture_Path = L"../data/textures/";
wchar_t* Shaders_Path = L"../data/shaders/";

unsigned client_width = 0;
unsigned client_height = 0;

ID3D11RenderTargetView* render_target_view = NULL;
ID3D11RasterizerState* rasterizer_state_solid = NULL;
ID3D11RasterizerState* rasterizer_state_wireframe = NULL;
ID3D11RasterizerState* rasterizer_state = NULL;
ID3D11BlendState* blend_state = NULL;
ID3D11Texture2D* depth_stencil_buffer = NULL;
ID3D11DepthStencilState* depth_state_enabled = NULL;
ID3D11DepthStencilState* depth_state_disabled = NULL;
ID3D11DepthStencilState* depth_state = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT init_d3d(HWND hwnd, IDXGISwapChain** swapChain, ID3D11Device** device, ID3D11DeviceContext** deviceContext, IDXGIAdapter1* adapter);
void close_d3d(void);
HRESULT compile_shader(const wchar_t* file_name, const char* profile, ID3DBlob** shaderBlob);
void change_color(float r, float g, float b, float a);
void update_simulation(float dt);


LARGE_INTEGER frequency;
LARGE_INTEGER starting_time, ending_time;
LARGE_INTEGER elapsed_milliseconds;

bool right_arrow;

wchar_t* get_path(const wchar_t* file_name, const wchar_t* folder_path)
{
	wchar_t* path = malloc(100 * sizeof(wchar_t));
	wcscpy(path, folder_path);
	return wcscat(path, file_name);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PWSTR pCmdLine, int nCmdSHow)
{
	unsigned long initial_value = __rdtsc();

	//From stack overflow: fi is not needed, we only use it because it's a required parameter
	//The important code is that stdout is redirected to CONOUT$, which is the console output stream
	BOOL console_enabled = false;
	if (console_enabled = AllocConsole())
	{
		FILE* fi = 0;
		freopen_s(&fi, "CONOUT$", "w", stdout);
	}
	console_enabled = AttachConsole(ATTACH_PARENT_PROCESS);

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&starting_time);

	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Those Big Things (D3D11)", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	if (hwnd == NULL) return -1;

	ShowWindow(hwnd, nCmdSHow);

	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	client_width = clientRect.right - clientRect.left;
	client_height = clientRect.bottom - clientRect.top;

	graphics_system_init(hwnd);
	
	Drawable2D* velociraptor = drawable2D_create_from_texture("data/textures/velociraptor.png");
	//Drawable2D* t_rex = drawable2D_create_from_texture("data/textures/t-rex.png");
	//Drawable2D* square = drawable2D_create_from_shape(Square, Red);

	device_context->lpVtbl->IASetPrimitiveTopology(device_context, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Next initialize the back buffer of the swap chain and associate it to a 
    // render target view.
	ID3D11Texture2D* backBuffer;
	HRESULT hr = swap_chain->lpVtbl->GetBuffer(swap_chain, 0, &IID_ID3D11Texture2D, &backBuffer);
	if (FAILED(hr))
	{
		return -1;
	}

	hr = device->lpVtbl->CreateRenderTargetView(device, backBuffer, NULL, &render_target_view);
	if (FAILED(hr))
	{
		return -1;
	}
	backBuffer->lpVtbl->Release(backBuffer);

	// set the render target as the back buffer
	device_context->lpVtbl->OMSetRenderTargets(device_context, 1, &render_target_view, NULL);

	D3D11_RASTERIZER_DESC rasterizer_desc_solid;
	memset(&rasterizer_desc_solid, 0, sizeof(D3D11_RASTERIZER_DESC));
	rasterizer_desc_solid.AntialiasedLineEnable = FALSE;
	rasterizer_desc_solid.CullMode = D3D11_CULL_BACK;
	rasterizer_desc_solid.DepthBias = 0;
	rasterizer_desc_solid.DepthBiasClamp = 0.0f;
	rasterizer_desc_solid.DepthClipEnable = TRUE;
	rasterizer_desc_solid.FillMode = D3D11_FILL_SOLID;
	rasterizer_desc_solid.FrontCounterClockwise = FALSE;
	rasterizer_desc_solid.MultisampleEnable = FALSE;
	rasterizer_desc_solid.ScissorEnable = FALSE;
	rasterizer_desc_solid.SlopeScaledDepthBias = 0.0f;

	D3D11_RASTERIZER_DESC rasterizer_desc_wireframe;
	memcpy(&rasterizer_desc_wireframe, &rasterizer_desc_solid, sizeof(D3D11_RASTERIZER_DESC));
	rasterizer_desc_wireframe.FillMode = D3D11_FILL_WIREFRAME;

	// Create the rasterizer state object.
	hr = device->lpVtbl->CreateRasterizerState(device, &rasterizer_desc_solid, &rasterizer_state_solid);
	if (FAILED(hr))
	{
		return -1;
	}

	hr = device->lpVtbl->CreateRasterizerState(device, &rasterizer_desc_wireframe, &rasterizer_state_wireframe);
	if (FAILED(hr))
	{
		return -1;
	}

	rasterizer_state = rasterizer_state_solid;

	D3D11_BLEND_DESC blend_state_desc;
	memset(&blend_state_desc, 0, sizeof(D3D11_BLEND_DESC));
	blend_state_desc.RenderTarget[0].BlendEnable = TRUE;
	blend_state_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blend_state_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blend_state_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend_state_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
	blend_state_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blend_state_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blend_state_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = device->lpVtbl->CreateBlendState(device, &blend_state_desc, &blend_state);
	if (FAILED(hr))
	{
		return -1;
	}

	D3D11_VIEWPORT viewport;
	viewport.Width = client_width;
	viewport.Height = client_height;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	device_context->lpVtbl->RSSetViewports(device_context, 1, &viewport);

	QueryPerformanceCounter(&ending_time);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		elapsed_milliseconds.QuadPart = ending_time.QuadPart - starting_time.QuadPart;
		elapsed_milliseconds.QuadPart *= 1000;
		elapsed_milliseconds.QuadPart /= frequency.QuadPart;

		QueryPerformanceCounter(&starting_time);

		TranslateMessage(&msg);
		DispatchMessage(&msg);

		//update_simulation((float)elapsed_milliseconds.QuadPart / 1000.0f);

		device_context->lpVtbl->
			ClearRenderTargetView(device_context, render_target_view, (const float[]) { 3.f/255.f, 167.f/255.f, 187.f/255.f, 1.0f });
		device_context->lpVtbl->RSSetState(device_context, rasterizer_state);
		device_context->lpVtbl->OMSetBlendState(device_context, blend_state, NULL, 0xffffffff);
		
		//drawable2D_draw(square);
		drawable2D_draw(velociraptor);

		//drawable2D_draw(t_rex);
		//	drawable2D_draw_all();

		swap_chain->lpVtbl->Present(swap_chain, 0, 0);

		QueryPerformanceCounter(&ending_time);
	}

	// TODO: remove original velociraptor image
	// stbi_image_free(image_data);
	close_d3d(swap_chain, device, device_context);

	if (console_enabled)
	{
		FreeConsole();
	}

	unsigned long final_value = __rdtsc();

	unsigned long diff = final_value - initial_value;

	return 0;
}

void close_d3d(void)
{
	//TODO: free resources properly
	/*swap_chain->lpVtbl->Release(swap_chain);
	device->lpVtbl->Release(device);
	device_context->lpVtbl->Release(device_context);
	render_target_view->lpVtbl->Release(render_target_view);
	vertex_buffer->lpVtbl->Release(vertex_buffer);
	vertex_shader_blob->lpVtbl->Release(vertex_shader_blob);
	pixel_shader_blob->lpVtbl->Release(pixel_shader_blob);
	vertex_shader->lpVtbl->Release(vertex_shader);
	pixel_shader->lpVtbl->Release(pixel_shader);
	rasterizer_state_solid->lpVtbl->Release(rasterizer_state_solid);
	rasterizer_state_wireframe->lpVtbl->Release(rasterizer_state_wireframe);
	blend_state->lpVtbl->Release(blend_state);*/
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_SPACE)
		{
			rasterizer_state = (rasterizer_state == rasterizer_state_solid) ?
				rasterizer_state_wireframe :
				rasterizer_state_solid;
		}
		else if (wParam == VK_SHIFT)
		{
			//change_color(1.0f, 1.0f, 1.0f, 1.0f);
		}
		else if (wParam == VK_CONTROL)
		{
			//change_color(0.0f, 1.0f, 0.0f, 1.0f);
		}
		else if (wParam == VK_RIGHT)
		{
			right_arrow = true;
		}
		else if (wParam == VK_ESCAPE)
		{
			DestroyWindow(hwnd);
		}
		return 0;
	case WM_KEYUP:
		if (wParam == VK_RIGHT)
		{
			right_arrow = false;
		}
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/*void change_color(float r, float g, float b, float a)
{
	for (int i = 0; i < 4; ++i)
	{
		vertices[i].color[0] = r;
		vertices[i].color[1] = g;
		vertices[i].color[2] = b;
		vertices[i].color[3] = a;
	}

	device_context->lpVtbl->UpdateSubresource(device_context, vertex_buffer, 0, NULL, vertices, 0, 0);
}*/

void update_simulation(float dt)
{
	if (right_arrow)
	{
		//float new_pos = old_pos + speed * dt;
		//Modify sprite position
		//glm_mat4_identity(model_transform);
		//vec3 vector = { new_pos, 0, 0 };
		//glm_translate(model_transform, vector);
	}

}