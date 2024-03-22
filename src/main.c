#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdbool.h>
#include <d3d11.h>
#include <cglm/cglm.h>
#include <string.h>
#include "drawable_2D.h"
#include "graphics_system.h"
#include "timing.h"
#include "input.h"
#include "text_rendering.h"

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

//Timing
long long OS_frequency;
long long starting_time, ending_time;
double delta_time;

Input_Data input;

bool right_arrow;

wchar_t* get_path(const wchar_t* file_name, const wchar_t* folder_path)
{
	wchar_t* path = malloc(100 * sizeof(wchar_t));
	wcscpy(path, folder_path);
	return wcscat(path, file_name);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PWSTR pCmdLine, int nCmdSHow)
{
	OS_frequency = get_OS_timer_frequency();

	//From stack overflow: fi is not needed, we only use it because it's a required parameter
	//The important code is that stdout is redirected to CONOUT$, which is the console output stream
	BOOL console_enabled = false;
	if (console_enabled = AllocConsole())
	{
		FILE* fi = 0;
		freopen_s(&fi, "CONOUT$", "w", stdout);
	}
	console_enabled = AttachConsole(ATTACH_PARENT_PROCESS);
	
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Those Big Things (D3D11)", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	if (hwnd == NULL) return -1;

	ShowWindow(hwnd, nCmdSHow);

	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	client_width = clientRect.right - clientRect.left;
	client_height = clientRect.bottom - clientRect.top;

	graphics_system_init(hwnd);
	
	Txt_Font* lucida_font = txt_create_font("data/fonts/lucon.ttf");

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

	starting_time = ending_time = get_OS_timer();
	delta_time = 0;

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		delta_time = (double)(ending_time - starting_time) / (double)OS_frequency;

		starting_time = get_OS_timer();

		TranslateMessage(&msg);
		DispatchMessage(&msg);

		device_context->lpVtbl->
			ClearRenderTargetView(device_context, render_target_view, (const float[]) { 3.f/255.f, 167.f/255.f, 187.f/255.f, 1.0f });
		device_context->lpVtbl->RSSetState(device_context, rasterizer_state);
		device_context->lpVtbl->OMSetBlendState(device_context, blend_state, NULL, 0xffffffff);
		
		//drawable2D_update(velociraptor, delta_time);
		//drawable2D_update(t_rex, delta_time);
		
		drawable2D_draw(velociraptor);
		//drawable2D_draw(t_rex);
		txt_draw(lucida_font, 50, "holag");

		swap_chain->lpVtbl->Present(swap_chain, 0, 0);

		ending_time = get_OS_timer();
	}

	// TODO: remove original velociraptor image
	// stbi_image_free(image_data);
	close_d3d(swap_chain, device, device_context);

	if (console_enabled)
	{
		FreeConsole();
	}

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
		input.event = Key_Down;
		input.key = Other;

		if (wParam == 0x41)
			input.key = Key_A;
		else if (wParam == 0x44)
			input.key = Key_D;
		else if (wParam == 0x53)
			input.key = Key_S;
		else if (wParam == 0x57)
			input.key = Key_W;
		else if (wParam == VK_RIGHT)
		{
			input.key = Key_RARROW;
			if (!(lParam >> 30) && 0x01)
				input.event = Key_Just_Down;
		}
		else if (wParam == VK_LEFT)
		{
			input.key = Key_LARROW;
			if (!(lParam >> 30) && 0x01)
				input.event = Key_Just_Down;
		}
		else if (wParam == VK_UP)
			input.key = Key_UARROW;
		else if (wParam == VK_DOWN)
			input.key = Key_DARROW;
		else if (wParam == VK_ESCAPE)
		{
			DestroyWindow(hwnd);
		}
		return 0;

	case WM_KEYUP:
		input.event = Key_Up;
		input.key = Other;

		if (wParam == 0x41)
			input.key = Key_A;
		else if (wParam == 0x44)
			input.key = Key_D;
		else if (wParam == 0x53)
			input.key = Key_S;
		else if (wParam == 0x57)
			input.key = Key_W;
		else if (wParam == VK_RIGHT)
			input.key = Key_RARROW;
		else if (wParam == VK_LEFT)
			input.key = Key_LARROW;
		else if (wParam == VK_UP)
			input.key = Key_UARROW;
		else if (wParam == VK_DOWN)
			input.key = Key_DARROW;
		
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
