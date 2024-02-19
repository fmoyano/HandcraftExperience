#ifndef DRAWABLE_2D
#define DRAWABLE_2D

typedef struct
{
	float position[3];
	float color[4];
	float uv[2];
} Vertex_Data;

typedef enum { Square } Shape2D;
typedef struct Drawable2D Drawable2D;
typedef struct Color Color;

extern Drawable2D* drawable2D_create_from_texture(const char* texture_path);
extern Drawable2D* drawable2D_create_from_shape(Shape2D shape, Color color);
extern void drawable2D_draw(Drawable2D* drawable);
extern void drawable2D_draw_all();

#endif