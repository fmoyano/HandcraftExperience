#include "text_rendering.h"

#include <fileapi.h>

#include <cglm/cglm.h>

#include <FreeType/ft2build.h>
#include FT_FREETYPE_H

extern ID3D11VertexShader* drawable_vertex_shader;
extern ID3D11PixelShader* drawable_pixel_shader;
extern void* vertex_shader_data;
extern int vertex_shader_size;
extern ID3D11Device* device;
extern ID3D11DeviceContext* device_context;

typedef struct
{
	int use_texture;
} PS_Extra_Rendering_Info;

typedef struct
{
	float position[3];
	float color[4];
	float uv[2];
} Vertex_Data;

typedef struct Txt_Font
{
	FT_Face face;
} Txt_Font;

typedef struct
{
	mat4 world_matrix;
	mat4 view_matrix;
	mat4 projection_matrix;
} VS_Matrices;

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

extern unsigned client_width;
extern unsigned client_height;

static void txt_draw_internal(void* image_data, int width, int height, int pitch, float offset);

Txt_Font* txt_create_font(char* path_to_font_file)
{
	FT_Library library;

	int error = FT_Init_FreeType(&library);
	if (error != 0)
	{
		printf("Error while initializing FreeType library\n");
		return NULL;
	}

	Txt_Font* txt_font = VirtualAlloc(NULL, sizeof(Txt_Font), MEM_COMMIT, PAGE_READWRITE);
	error = FT_New_Face(library,
		path_to_font_file,
		0,
		&(txt_font->face));
	if (error == FT_Err_Unknown_File_Format)
	{
		printf("Font format unsupported\n");
		return NULL;
	}
	else if (error)
	{
		printf("Font file could not be read or corrupted\n");
		return NULL;
	}

	return txt_font;
}

void txt_draw(Txt_Font* font, unsigned pixel_size, char* text)
{
	int error = FT_Set_Pixel_Sizes(
		font->face,   /* handle to face object */
		0,      /* pixel_width    (0 means take the pixel_height       */
		pixel_size);
	if (error)
	{
		printf("Problem setting the pixel size (probably not scalable font format)\n");
		return;
	}

	//pen_x = 300;
	//pen_y = 200;
	/*for (n = 0; n < num_chars; n++)
	{
		error = FT_Load_Char(face, text[n], FT_LOAD_RENDER);
		if (error)
			continue;

		my_draw_bitmap(&slot->bitmap,
			pen_x + slot->bitmap_left,
			pen_y - slot->bitmap_top);

		pen_x += slot->advance.x >> 6;
	}*/

	int pen_x = 0;
	int pen_y = 0;
	float offset_x = 0.0f;
	while (*text != '\0')
	{
		int glyph_index = FT_Get_Char_Index(font->face, *text++);
		error = FT_Load_Glyph(
			font->face,          /* handle to face object */
			glyph_index,   /* glyph index           */
			0);  /* load flags, see below */
		if (error)
		{
			printf("Problem loading glyph\n");
			return;
		}

		error = FT_Render_Glyph(font->face->glyph,   /* glyph slot  */
			FT_RENDER_MODE_NORMAL);
		if (error)
		{
			printf("Problem rendering glyph\n");
			return;
		}

		txt_draw_internal(font->face->glyph->bitmap.buffer,
			font->face->glyph->bitmap.width,
			font->face->glyph->bitmap.rows,
			font->face->glyph->bitmap.pitch,
			offset_x);

		pen_x += font->face->glyph->advance.x >> 6;
		offset_x = (float)pen_x / (float)client_width;
	}
}

static void txt_draw_internal(void* image_data, int width, int height, int pitch, float pen_x, float pen_y)
{
	ID3D11Texture2D* text_texture;

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
	texture_desc.Format = DXGI_FORMAT_A8_UNORM;

	D3D11_SUBRESOURCE_DATA texture_subresource;
	texture_subresource.pSysMem = image_data;
	texture_subresource.SysMemPitch = pitch;
	texture_subresource.SysMemSlicePitch = 0;

	HRESULT hr = device->lpVtbl->CreateTexture2D(device, &texture_desc, &texture_subresource,
		&text_texture);
	if (FAILED(hr))
	{
		exit(-1);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
	memset(&shader_resource_view_desc, 0, sizeof(shader_resource_view_desc));
	shader_resource_view_desc.Format = DXGI_FORMAT_A8_UNORM;
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_view_desc.Texture2D.MipLevels = texture_desc.MipLevels;
	shader_resource_view_desc.Texture2D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* shader_resource_view;
	hr = device->lpVtbl->CreateShaderResourceView(device, text_texture,
		&shader_resource_view_desc, &shader_resource_view);
	if (FAILED(hr))
	{
		return;
	}

	float initial_x = -0.5f;
	vertices[0].position[0] = initial_x + pen_x;
	vertices[0].position[1] = 0.5f;
	vertices[1].position[0] = initial_x + 2 * (float)width / (float)client_width + pen_x;
	vertices[1].position[1] = 0.5f;
	vertices[2].position[0] = initial_x + pen_x;
	vertices[2].position[1] = 0.5f - 2 * (float)height / (float)client_height;
	vertices[3].position[0] = initial_x + 2 * (float)width / (float)client_width + pen_x;
	vertices[3].position[1] = 0.5f - 2 * (float)height / (float)client_height;

	unsigned vertex_buffer_stride = sizeof(Vertex_Data);
	unsigned vertex_buffer_offset = 0;
	ID3D11Buffer* vertex_buffer, *index_buffer;

	graphics_system_create_buffer(vertices, Vertex_Buffer, sizeof(vertices),
		&vertex_buffer);

	graphics_system_create_buffer(indices, Index_Buffer, sizeof(indices),
		&index_buffer);

	D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	ID3D11InputLayout* inputLayout;
	hr = device->lpVtbl->CreateInputLayout(device, layoutDesc, 3,
		vertex_shader_data,
		vertex_shader_size, &inputLayout);

	if (FAILED(hr))
	{
		return;
	}

	ID3D11Buffer* use_texture_cbuffer;
	{
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
			&use_texture_cbuffer);

		if (FAILED(hr))
			return hr;
	}

	ID3D11Buffer* mvp_cbuffer;
	{
		//Model-view-projection matrices
		VS_Matrices matrices_cbuffer;
		glm_mat4_identity(matrices_cbuffer.world_matrix);
		glm_mat4_identity(matrices_cbuffer.view_matrix);
		glm_mat4_identity(matrices_cbuffer.projection_matrix);

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
			&mvp_cbuffer);

		if (FAILED(hr))
			return hr;
	}

	device_context->lpVtbl->IASetInputLayout(device_context, inputLayout);

	device_context->lpVtbl->VSSetShader(device_context, drawable_vertex_shader, 0, 0);
	device_context->lpVtbl->PSSetShader(device_context, drawable_pixel_shader, 0, 0);
	device_context->lpVtbl->PSSetShaderResources(device_context, 0, 1, &shader_resource_view);

	device_context->lpVtbl->VSSetConstantBuffers(device_context, 0, 1, &mvp_cbuffer);
	device_context->lpVtbl->PSSetConstantBuffers(device_context, 0, 1, &use_texture_cbuffer);

	device_context->lpVtbl->IASetVertexBuffers(device_context, 0, 1,
		&vertex_buffer, &vertex_buffer_stride, &vertex_buffer_offset);
	device_context->lpVtbl->IASetIndexBuffer(device_context, index_buffer, DXGI_FORMAT_R32_UINT, 0);

	device_context->lpVtbl->DrawIndexed(device_context, 6, 0, 0);
}