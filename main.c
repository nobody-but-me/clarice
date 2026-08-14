	
// don't look at it, please....

#include <raylib.h>
#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT WINDOW_WIDTH / 4*3

#define BUFFER_SIZE 1024

#define BOTTOM_MARGIN 15.0f
#define LEFT_MARGIN 15.0f
#define TOP_MARGIN 5.0f

#define FONT_SPACING 2.0f
#define FONT_SCALE 24.0f

#define LINE_NUMBERS true
#define TAB_SIZE 4

typedef struct
{	
	bool initialized;
	size_t capacity;
	int gap_start, gap_end;
	char *buffer;
} text_buffer;

typedef struct
{
	size_t capacity;
	int current_line, lines;
	text_buffer *gbs;
	
	Font editor_font;
} editor;

static Vector2 calculate_glyph(text_buffer *_gap_buffer, Font _font)
{
	size_t length = _gap_buffer->gap_start;
	char dest[length];
	
	strncpy(dest, _gap_buffer->buffer, _gap_buffer->gap_start);
	dest[length] = '\0';
	
	Vector2 text_measure = MeasureTextEx(_font, dest, FONT_SCALE, FONT_SPACING);
	return text_measure;
}

static void calculate_utf8_char(editor *_editor, const char *_string, size_t *_char_length, bool subsequent)
{
	int cx;
	if (subsequent)
		cx = _editor->gbs[_editor->current_line].gap_start + 1;
	else
		cx = _editor->gbs[_editor->current_line].gap_start - 1;
	while (cx > 0 && (_string[cx] & 0xC0) == 0x80)
	{
		if (subsequent)
			cx++;
		else
			cx--;
	}
	*_char_length = (_editor->gbs[_editor->current_line].gap_start - cx);
	if (subsequent)
		*_char_length *= -1;	
	return;
}

int gb_physical_position(text_buffer *_gap_buffer, int _logical)
{
	int gap_size = _gap_buffer->gap_end - _gap_buffer->gap_start;
	if (_logical < _gap_buffer->gap_start)
		return _logical;
	return _logical + gap_size;
}

void gb_grow(text_buffer *_gap_buffer, size_t _new_capacity);

void gb_init(editor *_editor, size_t _capacity)
{
	_editor->gbs = malloc(_capacity * sizeof(text_buffer));
	if (_editor->gbs == NULL)
	{
		fprintf(stderr, "null array of buffers.\n");
		exit(EXIT_FAILURE);
	}
	gb_grow(&_editor->gbs[0], _capacity);
	_editor->gbs[0].initialized = true;
	_editor->current_line = 0;
	_editor->lines = 0;
	return;
}

void gb_free(editor *_editor)
{
	for (int i = 0; i < _editor->lines; ++i)
	{
		if (_editor->gbs[i].buffer != NULL)
			free(_editor->gbs[i].buffer);
		_editor->gbs[i].buffer = NULL;
		_editor->gbs[i].gap_start = 0;
		_editor->gbs[i].capacity = 0;
		_editor->gbs[i].gap_end = 0;
	}
	return;
}

size_t gb_size(const text_buffer *_gap_buffer)
{
	return _gap_buffer->capacity - (_gap_buffer->gap_end - _gap_buffer->gap_start);
}
size_t gb_length(const text_buffer *_gap_buffer)
{
	return strlen(_gap_buffer->buffer);
}

void gb_move_to_index(text_buffer *_gap_buffer, int _index);
void gb_move_left(text_buffer *_gap_buffer)
{
	if (_gap_buffer->gap_start == 0)
		return;
	_gap_buffer->gap_start--;
	_gap_buffer->gap_end--;
	_gap_buffer->buffer[_gap_buffer->gap_end] = _gap_buffer->buffer[_gap_buffer->gap_start];
	return;
}
void gb_move_right(text_buffer *_gap_buffer)
{
	if (_gap_buffer->gap_end == _gap_buffer->capacity)
		return;
	_gap_buffer->buffer[_gap_buffer->gap_start] = _gap_buffer->buffer[_gap_buffer->gap_end];
	_gap_buffer->gap_start++;
	_gap_buffer->gap_end++;
	return;
}
void gb_move_up(editor *_editor)
{
	if (_editor->current_line != 0)
	{
		_editor->current_line--;
		// TODO: detect if the current character is part of utf8 and move the right length;
		gb_move_to_index(&_editor->gbs[_editor->current_line], _editor->gbs[_editor->current_line + 1].gap_start);
	}
	return;
}
void gb_move_down(editor *_editor)
{
	if (_editor->current_line != _editor->lines)
	{
		_editor->current_line++;
		gb_move_to_index(&_editor->gbs[_editor->current_line], _editor->gbs[_editor->current_line - 1].gap_start);
	}
	return;
}

void gb_move_to_index(text_buffer *_gap_buffer, int _index)
{
	if (_index < 0)
		return;
	if (_index > _gap_buffer->gap_start)
		for (int i = _gap_buffer->gap_start; i < _index; ++i)
			gb_move_right(_gap_buffer);
	else
		for (int i = _gap_buffer->gap_start; i > _index; --i)
			gb_move_left(_gap_buffer);
	return;
}

void gb_remove_line(editor *_editor);
void gb_backspace(editor *_editor)
{
	text_buffer *_gap_buffer = &_editor->gbs[_editor->current_line];
	if (_gap_buffer->gap_start == 0)
	{
		gb_remove_line(_editor);
		return;
	}
	size_t char_length;
	calculate_utf8_char(_editor, _gap_buffer->buffer, &char_length, false);
	_gap_buffer->gap_start -= char_length;
	return;
}
void gb_clean_line(text_buffer *_gap_buffer)
{
	if (_gap_buffer->gap_start == 0)
		return;
	for (int i = gb_size(_gap_buffer); i > 0; --i)
		_gap_buffer->gap_start--;
	return;
}

void gb_grow(text_buffer *_gap_buffer, size_t _new_capacity)
{
	char *new_buffer = malloc(_new_capacity);
	if (new_buffer == NULL)
	{
		fprintf(stderr, "null new_buffer.\n");
		exit(EXIT_FAILURE);
	}
	memcpy(new_buffer, _gap_buffer->buffer, _gap_buffer->gap_start);
	
	size_t right_size = _gap_buffer->capacity - _gap_buffer->gap_end;
	size_t new_gap_end = _new_capacity - right_size;
	memcpy(new_buffer + new_gap_end, _gap_buffer->buffer + _gap_buffer->gap_end, right_size);
	free(_gap_buffer->buffer);
	
	_gap_buffer->capacity = _new_capacity;
	_gap_buffer->gap_end = new_gap_end;
	_gap_buffer->buffer = new_buffer;
	return;
}
void gb_insert(editor *_editor, char c)
{
	text_buffer *_gap_buffer = &_editor->gbs[_editor->current_line];
	if (_gap_buffer->gap_start == _gap_buffer->gap_end)
	{
		fprintf(stderr, "increasing gap size.\n");
		gb_grow(_gap_buffer, _gap_buffer->capacity * 2);
	}
	_gap_buffer->buffer[_gap_buffer->gap_start++] = c;
	return;
}
void gb_insert_string(text_buffer *_gap_buffer, const char *_string)
{
	size_t length = strlen(_string);
	for (int i = 0; i < length; ++i)
	{
		if (_gap_buffer->gap_start == _gap_buffer->gap_end)
		{
			fprintf(stderr, "increasing gap size.\n");
			gb_grow(_gap_buffer, _gap_buffer->capacity * 2);
		}
		_gap_buffer->buffer[_gap_buffer->gap_start++] = _string[i];
	}
	return;
}

char *gb_get_buffer(const text_buffer *_gap_buffer);
void gb_insert_line(editor *_editor)
{
	// too many bad code
	_editor->lines++;
	memmove(&_editor->gbs[_editor->current_line + 1],
		&_editor->gbs[_editor->current_line], (_editor->lines - _editor->current_line) * sizeof(text_buffer));
	_editor->current_line++;
	memset(&_editor->gbs[_editor->current_line], 0, sizeof(text_buffer));
	gb_grow(&_editor->gbs[_editor->current_line], BUFFER_SIZE);
	
	if (_editor->gbs[_editor->current_line - 1].gap_start < (int)(gb_size(&_editor->gbs[_editor->current_line - 1])))
	{
		char *b = gb_get_buffer(&_editor->gbs[_editor->current_line - 1]);
		const char *bb = b + _editor->gbs[_editor->current_line - 1].gap_start;
		
		gb_insert_string(&_editor->gbs[_editor->current_line], bb);
		gb_move_to_index(&_editor->gbs[_editor->current_line],
			_editor->gbs[_editor->current_line].gap_start - strlen(bb));
		
		_editor->current_line--;
		gb_move_to_index(&_editor->gbs[_editor->current_line], gb_size(&_editor->gbs[_editor->current_line]));
		for (int i = strlen(bb); i > 0; --i)
			gb_backspace(_editor);
		_editor->current_line++;
		free(b);
	}
	_editor->gbs[_editor->current_line].initialized = true;
	return;
}
void gb_remove_line(editor *_editor)
{
	// and now I activate my special: THE BAD COOOOOOODEEEEEEEEEER
	if(_editor->current_line == 0)
		return;
	if (gb_size(&_editor->gbs[_editor->current_line - 1]) > 0)
	{
		char *b = gb_get_buffer(&_editor->gbs[_editor->current_line - 1]);
		gb_insert_string(&_editor->gbs[_editor->current_line], b);
		free(b);
	}
	memmove(&_editor->gbs[_editor->current_line - 1],
		&_editor->gbs[_editor->current_line], (_editor->lines - _editor->current_line + 1) * sizeof(text_buffer));
	memset(&_editor->gbs[_editor->lines], 0, sizeof(text_buffer));
	_editor->lines--; _editor->current_line--;
	return;
}

void gb_print(const text_buffer *_gap_buffer)
{
	fwrite(_gap_buffer->buffer, 1, _gap_buffer->gap_start, stdout);
	fwrite(_gap_buffer->buffer + _gap_buffer->gap_end, 1, _gap_buffer->capacity - _gap_buffer->gap_end, stdout);
	putchar('\n');
	return;
}

char *gb_get_buffer(const text_buffer *_gap_buffer)
{
	size_t left_size = _gap_buffer->gap_start;
	size_t right_size = _gap_buffer->capacity - _gap_buffer->gap_end;
	size_t size = left_size + right_size;
	
	char *result = malloc(size + 1);
	if (!result)
		return NULL;
	memcpy(result, _gap_buffer->buffer, left_size);
	memcpy(result + left_size, _gap_buffer->buffer + _gap_buffer->gap_end, right_size);
	result[size] = '\0';
	return result;
}

void init_font(editor *_editor)
{
	int count = 0; int codepoints[256];
	for (int i = 32; i <= 255; ++i)
		codepoints[count++] = i;
	_editor->editor_font = LoadFontEx("./mechanical.otf", 24, codepoints, count);
	if (!IsFontValid(_editor->editor_font))
	{
		fprintf(stderr, "invalid font.\n");
		exit(EXIT_FAILURE);
	}
	return;
}

void init_window(void)
{
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Clarice");
	SetWindowState(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(60);
	return;
}

void cursor_rendering(editor *_editor, Font _font)
{
	Vector2 char_size = calculate_glyph(&_editor->gbs[_editor->current_line], _font);
	DrawRectangle(char_size.x,
		FONT_SCALE * _editor->current_line + (FONT_SCALE),
		MeasureTextEx(_font, "W", FONT_SCALE, FONT_SPACING).x + 5, (FONT_SCALE / 3), RED);
//		((MeasureTextEx(_font, "A", FONT_SCALE, FONT_SPACING).x + 5) / 3), FONT_SCALE, RED);
	return;
}

void text_rendering(editor *_editor, Font _font)
{
	char *b;
	for (int i = 0; i <= _editor->lines; ++i)
	{
		b = gb_get_buffer(&_editor->gbs[i]);
		if (b == NULL)
			break;
		DrawTextEx(_font, b, (Vector2){0.0f, 5.0f + i * FONT_SCALE}, FONT_SCALE, FONT_SPACING, BLACK);
		free(b);
	}
	return;
}

void input_processing(editor *_editor)
{
	int key = GetCharPressed();
	while (key > 0)
	{
		if (_editor->gbs == NULL)
			return;
		int utf8_size = 0;
		const char *utf8 = CodepointToUTF8(key, &utf8_size);
		
		gb_insert_string(&_editor->gbs[_editor->current_line], utf8);
		
		key = GetCharPressed();
	}
	if (IsKeyPressed(KEY_BACKSPACE))
		gb_backspace(_editor);
	if (IsKeyPressed(KEY_ENTER))
		gb_insert_line(_editor);
	
// control keys
	if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
	{
		if (IsKeyPressed(KEY_M))
			gb_insert_line(_editor);
		
		if (IsKeyPressed(KEY_A))
		{
			text_buffer *current_line = &_editor->gbs[_editor->current_line];
			gb_move_to_index(current_line, 0);
		}
		else if (IsKeyPressed(KEY_E))
		{
			text_buffer *current_line = &_editor->gbs[_editor->current_line];
			gb_move_to_index(current_line, gb_size(current_line));
		}
		
		if (IsKeyPressed(KEY_P))
			gb_move_up(_editor);
		else if (IsKeyPressed(KEY_N))
			gb_move_down(_editor);
		
		if (IsKeyPressed(KEY_B))
		{
			char *b = gb_get_buffer(&_editor->gbs[_editor->current_line]);
			int cursor_x = _editor->gbs[_editor->current_line].gap_start;
			size_t char_length;
			calculate_utf8_char(_editor, b, &char_length, false);
			gb_move_to_index(&_editor->gbs[_editor->current_line], (cursor_x - char_length));
			free(b);
		}
		else if (IsKeyPressed(KEY_F))
		{
			char *b = gb_get_buffer(&_editor->gbs[_editor->current_line]);
			int cursor_x = _editor->gbs[_editor->current_line].gap_start;
			size_t char_length;
			calculate_utf8_char(_editor, b, &char_length, true);
			gb_move_to_index(&_editor->gbs[_editor->current_line], (cursor_x + char_length));
			free(b);
		}
	}
	return;
}

void main_loop(editor *_editor)
{
	while (!WindowShouldClose())
	{
		input_processing(_editor);
		BeginDrawing();
			ClearBackground(WHITE);
			text_rendering(_editor, _editor->editor_font);
			cursor_rendering(_editor, _editor->editor_font);
		EndDrawing();
	}
	EndDrawing();
	return;
}

int main(int argc, char**argv)
{
	printf("Hello, Clarice.\n");
	editor e;
	
	gb_init(&e, BUFFER_SIZE);
	const char *base_text = "Hello, Clarice.";
	gb_insert_string(&e.gbs[e.current_line], base_text);
	
	init_window();
	init_font(&e);
	
	main_loop(&e);
	
	gb_free(&e);
	return 0;
}
