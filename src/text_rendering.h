#ifndef TEXT_RENDERING_H
#define TEXT_RENDERING_H

#include "graphics_system.h"

typedef struct Txt_Font Txt_Font;

extern Txt_Font* txt_create_font(char* path_to_font_file);
extern void txt_draw(Txt_Font* font, unsigned pixel_size, char* text);

/*void* read_font_file(ID3D11Device* device, int* width, int* height, int* pitch)
{
	FT_Library library;
	*width = 0, *height = 0, *pitch = 0;

	int error = FT_Init_FreeType(&library);
	if (error != 0)
	{
		printf("Error while initializing FreeType library\n");
		return NULL;
	}

	FT_Face face;
	error = FT_New_Face(library,
		"data/fonts/lucon.ttf",
		0,
		&face);
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

	error = FT_Set_Pixel_Sizes(
		face,
		0,        
		24);
	if (error)
	{
		printf("Problem setting the pixel size (probably not scalable font format)\n");
		return NULL;
	}

	int glyph_index = FT_Get_Char_Index(face, 'A');
	error = FT_Load_Glyph(
		face,         
		glyph_index,  
			0);
	if (error)
	{
		printf("Problem loading glyph\n");
		return NULL;
	}

	error = FT_Render_Glyph(face->glyph,
		FT_RENDER_MODE_NORMAL);
	if (error)
	{
		printf("Problem rendering glyph\n");
		return NULL;
	}

	*width = face->glyph->bitmap.width;
	*height = face->glyph->bitmap.rows;
	*pitch = face->glyph->bitmap.pitch;

	return face->glyph->bitmap.buffer;
}*/

#endif