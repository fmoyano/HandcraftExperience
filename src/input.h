#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

typedef enum
{
	Other, Key_A, Key_W, Key_S, Key_D, Key_LARROW, Key_RARROW, Key_UARROW, Key_DARROW
} Keyboard_Key;

typedef enum 
{
	Undefined, Key_Down, Key_Up, Key_Just_Down
} Event_Type;

typedef struct
{
	Keyboard_Key key;
	Event_Type event;
} Input_Data;


inline bool is_key_down(Input_Data* input_data)
{
	return input_data->event == Key_Down || input_data->event == Key_Just_Down;
}

#endif