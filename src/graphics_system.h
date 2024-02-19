#ifndef GRAPHICS_SYSTEM
#define GRAPHICS_SYSTEM
 
#include <stddef.h>

#ifdef _WIN32
#include <d3d11.h>
#include <d3dcompiler.h>
#endif

typedef struct Color
{
	float r;
	float g;
	float b;
	float a;
} Color;

static Color Black = { .a = 1.0f };
static Color Red = { .r = 1.0f, .a = 1.0f };
static Color Green = { .g = 1.0f, .a = 1.0f };
static Color Blue = { .b = 1.0f, .a = 1.0f };
static Color White = { .r = 1.0, .g = 1.0f, .b = 1.0f, .a = 1.0f };

typedef enum { Pixel_Shader, Vertex_Shader } Shader_type;
typedef enum { Index_Buffer, Vertex_Buffer} Buffer_type;
typedef struct HWND__* HWND;

extern void graphics_system_init(HWND window_handler);
extern int graphics_system_create_vshader(const wchar_t* path_to_source,
	ID3DBlob** vertex_shader_blob, ID3D11VertexShader** vertex_shader);
extern int graphics_system_create_pshader(const wchar_t* path_to_source,
	ID3DBlob** pixel_shader_blob, ID3D11PixelShader** pixel_shader);
extern int graphics_system_create_buffer(void* data, Buffer_type buffer_type,
	size_t buffer_byte_width, ID3D11Buffer** the_buffer);
extern void graphics_system_close(void);

#endif
