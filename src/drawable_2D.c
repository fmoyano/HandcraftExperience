#define _CRT_SECURE_NO_WARNINGS

#include "drawable_2D.h"
#include "graphics_system.h"
#include <string.h>
#include <cglm/cglm.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MAX_DRAWABLES 50

extern IDXGISwapChain* swap_chain;
extern ID3D11Device* device;
extern ID3D11DeviceContext* device_context;
extern unsigned client_width;
extern unsigned client_height;

typedef struct 
{
	int use_texture;
} PS_Extra_Rendering_Info;

typedef struct
{
	mat4 world_matrix;
	mat4 view_matrix;
	mat4 projection_matrix;
} VS_Matrices;

typedef struct
{
	vec3 world_position;
	vec3 scale;
	float degrees_around_z;
} Transform;

typedef struct Drawable2D
{
	Transform transform;

	int *indices;
	float* vertices;
	
	ID3D11Buffer* vertex_buffer;
	ID3D11Buffer* index_buffer;
	unsigned vertex_buffer_stride;
	unsigned vertex_buffer_offset;

	ID3D11Texture2D* d3d_texture;
	ID3D11InputLayout* inputLayout;

	ID3D11ShaderResourceView* shader_resource_view;

	ID3D11Buffer* use_texture_cbuffer;
	ID3D11Buffer* mvp_cbuffer;
} Drawable2D;

Drawable2D drawables[MAX_DRAWABLES];
static unsigned drawable_index = 0;

static Vertex_Data vertices[] =
{
	{-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
	{0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
	{-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},
	{0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}
};

static unsigned indices[] =
{
	0, 3, 2,
	0, 1, 3
};

ID3D11VertexShader* drawable_vertex_shader;
ID3DBlob* drawable_vertex_shader_blob;
ID3D11VertexShader* drawable_pixel_shader;
ID3DBlob* drawable_pixel_shader_blob;

Drawable2D* drawable2D_create_from_texture(const char* texture_path)
{
	int width, height, comp;
	unsigned char* image_data = stbi_load(texture_path, &width, &height, &comp, STBI_rgb_alpha);

	//Need to map the width and height of the image to clip space, taking into account that:
	//2 horizontal units in clip space = client_width (width of the window in pixels)
	//2 vertical units in clip space = client_height (height of the window in pixels)
	float image_aspect_ratio = (float)width / (float)height;
	float image_pixel_width_1_clip_width_unit = 1.0f * client_width / 2.0f;
	float image_pixel_height = image_pixel_width_1_clip_width_unit / image_aspect_ratio;
	float image_height_clip_space = image_pixel_height * 2.0f / client_height;

	float start_y_position = 0.5f;
	vertices[0].position[1] = start_y_position;
	vertices[1].position[1] = start_y_position;
	vertices[2].position[1] = start_y_position - image_height_clip_space;
	vertices[3].position[1] = start_y_position - image_height_clip_space;

	Drawable2D drawable;
	memset(&drawable, 0, sizeof(Drawable2D));
	drawable.vertices = vertices;
	drawable.indices = indices;
	drawable.vertex_buffer_stride = sizeof(Vertex_Data);
	drawable.vertex_buffer_offset = 0;

	Transform transform = { .degrees_around_z = 45.0f };
	glm_vec3_zero(transform.world_position);
	float position_vector[] = {0.5f, 0.5f, 0.0f};
	glm_vec3(position_vector, transform.world_position);
	glm_vec3_zero(transform.scale);
	float scale_vector[] = {1.0f, 1.0f, 0.0f};
	glm_vec3(scale_vector, transform.scale);
	drawable.transform = transform;

	graphics_system_create_buffer(vertices, Vertex_Buffer, sizeof(vertices),
		&drawable.vertex_buffer);

	graphics_system_create_buffer(indices, Index_Buffer, sizeof(indices),
		&drawable.index_buffer);

	D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	HRESULT hr = device->lpVtbl->CreateInputLayout(device, layoutDesc, 3,
		drawable_vertex_shader_blob->lpVtbl->GetBufferPointer(drawable_vertex_shader_blob),
		drawable_vertex_shader_blob->lpVtbl->GetBufferSize(drawable_vertex_shader_blob), &drawable.inputLayout);

	//Create a Direct3D texture from the image data to feed the pixel shader sampler
	D3D11_TEXTURE2D_DESC texture_desc;
	texture_desc.Width = width;
	texture_desc.Height = height;
	texture_desc.ArraySize = 1;
	texture_desc.MipLevels = 1;
	texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texture_desc.Usage = D3D11_USAGE_DEFAULT;
	texture_desc.CPUAccessFlags = 0;
	texture_desc.MiscFlags = 0;
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	D3D11_SUBRESOURCE_DATA texture_subresource;
	texture_subresource.pSysMem = image_data;
	texture_subresource.SysMemPitch = texture_desc.Width * 4;
	texture_subresource.SysMemSlicePitch = 0;

	hr = device->lpVtbl->CreateTexture2D(device, &texture_desc, &texture_subresource,
		&drawable.d3d_texture);
	if (FAILED(hr))
	{
		exit(-1);
	}

	PS_Extra_Rendering_Info ps_cbuffer;
	ps_cbuffer.use_texture = 1;

	D3D11_BUFFER_DESC cb_desc;
	memset(&cb_desc, 0, sizeof(D3D11_BUFFER_DESC));
	cb_desc.ByteWidth = 16;//sizeof(PS_Extra_Rendering_Info) see docs, seems 16 is the minimum;
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA init_data;
	memset(&init_data, 0, sizeof(D3D11_SUBRESOURCE_DATA));
	init_data.pSysMem = &ps_cbuffer;
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;

	hr = device->lpVtbl->CreateBuffer(device, &cb_desc, &init_data,
		&drawable.use_texture_cbuffer);

	if (FAILED(hr))
		return hr;

	{
		//Model-view-projection matrices
		VS_Matrices matrices_cbuffer;
		glm_mat4_identity(matrices_cbuffer.world_matrix);
		glm_mat4_identity(matrices_cbuffer.view_matrix);
		glm_mat4_identity(matrices_cbuffer.projection_matrix);

		glm_translate(matrices_cbuffer.world_matrix, drawable.transform.world_position);
		glm_scale(matrices_cbuffer.world_matrix, drawable.transform.scale);
		float rotation_axis[] = { 0.0f, 0.0f, 1.0f };
		glm_rotate(matrices_cbuffer.world_matrix, drawable.transform.degrees_around_z, rotation_axis);

		D3D11_BUFFER_DESC cb_matrix_desc;
		memset(&cb_matrix_desc, 0, sizeof(D3D11_BUFFER_DESC));
		cb_matrix_desc.ByteWidth = sizeof(VS_Matrices);
		cb_matrix_desc.Usage = D3D11_USAGE_DYNAMIC;
		cb_matrix_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb_matrix_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		D3D11_SUBRESOURCE_DATA init_matrix_data;
		memset(&init_matrix_data, 0, sizeof(D3D11_SUBRESOURCE_DATA));
		init_matrix_data.pSysMem = &matrices_cbuffer;
		init_matrix_data.SysMemPitch = 0;
		init_matrix_data.SysMemSlicePitch = 0;

		hr = device->lpVtbl->CreateBuffer(device, &cb_matrix_desc, &init_matrix_data,
			&drawable.mvp_cbuffer);

		if (FAILED(hr))
			return hr;
	}

	//This texture image will be sampled in the pixel shader to draw this drawable
	//So we need to create a shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
	memset(&shader_resource_view_desc, 0, sizeof(shader_resource_view_desc));
	shader_resource_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_view_desc.Texture2D.MipLevels = texture_desc.MipLevels;
	shader_resource_view_desc.Texture2D.MostDetailedMip = 0;

	hr = device->lpVtbl->CreateShaderResourceView(device, drawable.d3d_texture, 
		&shader_resource_view_desc, &drawable.shader_resource_view);
	if (FAILED(hr))
	{
		return -1;
	}
	
	drawables[drawable_index++] = drawable;
	return &drawables[drawable_index-1];
}

Drawable2D* _drawable2D_create_square(Color shape_color)
{
	Vertex_Data square_vertices[] =
	{
		{-0.25f, 0.25f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{0.25f, 0.25f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{-0.25f, -0.25f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{0.25f, -0.25f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f}
	};

	for (int i = 0; i < 4; ++i)
	{
		square_vertices[i].color[0] = shape_color.r;
		square_vertices[i].color[1] = shape_color.g;
		square_vertices[i].color[2] = shape_color.b;
		square_vertices[i].color[3] = shape_color.a;
	}

	Drawable2D drawable;
	memset(&drawable, 0, sizeof(Drawable2D));
	drawable.vertices = square_vertices;
	drawable.indices = indices;
	drawable.vertex_buffer_stride = sizeof(Vertex_Data);
	drawable.vertex_buffer_offset = 0;

	graphics_system_create_buffer(square_vertices, Vertex_Buffer, sizeof(square_vertices),
		&drawable.vertex_buffer);

	graphics_system_create_buffer(indices, Index_Buffer, sizeof(indices),
		&drawable.index_buffer);

	D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	HRESULT hr = device->lpVtbl->CreateInputLayout(device, layoutDesc, 3,
		drawable_vertex_shader_blob->lpVtbl->GetBufferPointer(drawable_vertex_shader_blob),
		drawable_vertex_shader_blob->lpVtbl->GetBufferSize(drawable_vertex_shader_blob), &drawable.inputLayout);

	PS_Extra_Rendering_Info ps_cbuffer;
	ps_cbuffer.use_texture = 0;

	D3D11_BUFFER_DESC cb_desc;
	cb_desc.ByteWidth = 16; //16 is the minimum buffer width we must specify
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cb_desc.MiscFlags = 0;
	cb_desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA init_data;
	init_data.pSysMem = &ps_cbuffer;
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;

	hr = device->lpVtbl->CreateBuffer(device, &cb_desc, &init_data,
		&drawable.use_texture_cbuffer);

	if (FAILED(hr))
		return hr;

	drawables[drawable_index++] = drawable;
	return &drawables[drawable_index - 1];
}

Drawable2D* drawable2D_create_from_shape(Shape2D shape, Color color)
{
	Drawable2D* drawable = NULL;

	if (shape == Square)
	{
		drawable = _drawable2D_create_square(color);
	}

	return drawable;
}

void drawable2D_draw(Drawable2D* drawable)
{
	device_context->lpVtbl->IASetInputLayout(device_context, drawable->inputLayout);

	device_context->lpVtbl->VSSetShader(device_context, drawable_vertex_shader, 0, 0);
	device_context->lpVtbl->PSSetShader(device_context, drawable_pixel_shader, 0, 0);
	device_context->lpVtbl->PSSetShaderResources(device_context, 0, 1, &drawable->shader_resource_view);

	device_context->lpVtbl->VSSetConstantBuffers(device_context, 0, 1, &drawable->mvp_cbuffer);
	device_context->lpVtbl->PSSetConstantBuffers(device_context, 0, 1, &drawable->use_texture_cbuffer);

	device_context->lpVtbl->IASetVertexBuffers(device_context, 0, 1,
		&drawable->vertex_buffer, &drawable->vertex_buffer_stride, &drawable->vertex_buffer_offset);
	device_context->lpVtbl->IASetIndexBuffer(device_context, drawable->index_buffer, DXGI_FORMAT_R32_UINT, 0);

	//printf("Count = %ul %ul %ul\n", sizeof(indices), sizeof(drawable->index_buffer[0]), sizeof(indices) / sizeof(drawable->index_buffer[0]));
	device_context->lpVtbl->DrawIndexed(device_context, 6, 0, 0);
}

void drawable2D_draw_all()
{
	for (int i = 0; i < drawable_index; ++i)
	{
		drawable2D_draw(&drawables[i]);
	}
}

void drawable2D_destroy(Drawable2D* drawable)
{

}

