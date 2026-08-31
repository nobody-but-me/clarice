	
// don't look at it, please....

#include <raylib.h>
#include <string.h>

#include <ctype.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT WINDOW_WIDTH / 4*3

#define BUFFER_SIZE 1024

#define BOTTOM_MARGIN 15.0f
#define LEFT_MARGIN 15.0f
#define TOP_MARGIN 15.0f

#define LINE_NUMBERS true
#define TAB_SIZE 4

typedef struct
{	
	bool initialized;
	size_t capacity;
	size_t gap_start, gap_end;
	char *buffer;
} text_buffer;

typedef struct
{
	char *current_file;
	Camera2D camera;
	
	int font_codepoints[256];
	int font_count;
	
	size_t capacity;
	int current_line, lines;
	text_buffer *gbs;
	
	Font editor_font;
} editor;

float FONT_SPACING = 2.0f;
float FONT_SCALE = 24.0f;


#define TEXT_CHECK_SIZE 64 * 1024
static bool IsValidUTF8(const unsigned char *data, size_t size)
{
	size_t i = 0;
	while (i < size)
	{
		unsigned char c = data[i];
		if (c <= 0x7F) { i++; continue; }	
		if (c >= 0xC2 && c <= 0xDF)
		{
			if (i + 1 >= size) return false;
			if ((data[i + 1] & 0xC0) != 0x80)
				return false;
			i += 2;
			continue;
		}
		if (c >= 0xE0 && c <= 0xEF)
		{
			if (i + 2 >= size) return false;
			unsigned char c1 = data[i + 1];
			unsigned char c2 = data[i + 2];
			if ((c1 & 0xC0) != 0x80 ||
				(c2 & 0XC0) != 0x80)
				return false;
			if (c == 0xE0 && c1 < 0xA0) return false;
			if (c == 0xED && c1 < 0xA0) return false;
			i += 3;
			continue;
		}
		if (c >= 0xF0 && c <= 0xF4)
		{
			if (i + 3 >= size) return false;
			unsigned char c1 = data[i + 1];
			unsigned char c2 = data[i + 2];
			unsigned char c3 = data[i + 3];
			if ((c1 & 0xC0) != 0x80 ||
				(c2 & 0xC0) != 0x80 ||
				(c3 & 0xC0) != 0x80)
				return false;
			if (c == 0xF0 && c1 < 0x80) return false;
			if (c == 0xF4 && c1 >= 0x90) return false;
			i += 4;
			continue;
		}
		return false;
	}
	return true;
}
static bool IsTextBytes(const unsigned char *data, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		unsigned char c = data[i];
		if (c == 0x00) return false;
		if (c < 0x20 && c != '\t' &&
			c != '\n' && c != '\r') return false;
	}
	return IsValidUTF8(data, size);
}
bool IsTextFile(const char *filename)
{
	FILE *file = fopen(filename, "rb");
	if (file == NULL) return false;
	unsigned char buffer[TEXT_CHECK_SIZE];
	size_t bytesRead = fread(buffer, 1, sizeof(buffer), file); fclose(file);
	if (bytesRead == 0) return true;
	return IsTextBytes(buffer, bytesRead);
}


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

int gb_at(text_buffer *_gap_buffer, int _logical)
{
	int gap_size = _gap_buffer->gap_end - _gap_buffer->gap_start;
	if (_logical < _gap_buffer->gap_start)
		return _logical;
	return _logical + gap_size;
}

void gb_grow(text_buffer *_gap_buffer, size_t _new_capacity);
void gb_free(editor *_editor);

void gb_init(editor *_editor, size_t _capacity)
{
	_editor->capacity = _capacity;
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
	
	_editor->camera.target = (Vector2){WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
	_editor->camera.offset = (Vector2){ WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f };
	_editor->camera.rotation = 0.0f; _editor->camera.zoom = 1.0f;
	
	_editor->current_file = NULL;
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
	UnloadFont(_editor->editor_font);
	if (_editor->current_file != NULL)
		free(_editor->current_file);
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

size_t gb_get_screen_y(void)
{
	size_t s = 0;
	for (size_t i = 0; (i * FONT_SCALE + TOP_MARGIN) < (WINDOW_HEIGHT - FONT_SCALE - BOTTOM_MARGIN); ++i)
		s++;
	return s;
}

size_t gb_get_screen_x(void)
{
	size_t s = 0;
	for (size_t i = 0; (i * FONT_SCALE + TOP_MARGIN) < (WINDOW_WIDTH - FONT_SCALE - LEFT_MARGIN); ++i)
		s++;
	return s;
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
		gb_move_to_index(_gap_buffer, gb_size(_gap_buffer));
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

void gb_grow_lines(editor *_editor)
{
	text_buffer *tmp = realloc(_editor->gbs, _editor->capacity * 2 * sizeof(text_buffer));
	if (tmp == NULL)
	{
		fprintf(stderr, "failed to reallocate more memory for lines.\n");
		exit(EXIT_FAILURE);
		gb_free(_editor);
		return;
	}
	_editor->gbs = tmp;
	return;
}

char *gb_get_buffer(const text_buffer *_gap_buffer);
void gb_insert_line(editor *_editor)
{
	// too much bad code help me
	_editor->lines++;
	if (_editor->lines >= _editor->capacity)
	{
		fprintf(stderr, "increasing line capacity.\n");
		gb_grow_lines(_editor);
	}
	
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
	if (_editor->current_line == 0)
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

static float lerp(float v0, float v1, float t)
{
	return v0 + t * (v1 - v0);
}

void center_screen_cursor(editor *_editor)
{
	text_buffer *current_line = &_editor->gbs[_editor->current_line];
	Vector2 char_size = calculate_glyph(&_editor->gbs[_editor->current_line], _editor->editor_font);
	if (current_line->gap_start > gb_get_screen_x())
//		_editor->camera.target.x = lerp(_editor->camera.target.x, ((c / 2) * FONT_SCALE) + (WINDOW_WIDTH / 2), 0.3f);
		_editor->camera.target.x = lerp(_editor->camera.target.x, LEFT_MARGIN + char_size.x, 0.3f);
	else
		_editor->camera.target.x = lerp(_editor->camera.target.x, (WINDOW_WIDTH / 2.0f), 0.3f);
		
	_editor->camera.target.y = lerp(_editor->camera.target.y, _editor->current_line * FONT_SCALE, 0.3f);
	return;
}

void init_font(editor *_editor)
{
	for (int i = 32; i <= 255; ++i)
		_editor->font_codepoints[_editor->font_count++] = i;
	_editor->editor_font = LoadFontEx("./mechanical.otf", FONT_SCALE, _editor->font_codepoints, _editor->font_count);
	if (!IsFontValid(_editor->editor_font))
	{
		fprintf(stderr, "invalid font.\n");
		exit(EXIT_FAILURE);
	}
	SetTextureFilter(_editor->editor_font.texture, TEXTURE_FILTER_BILINEAR);
	return;
}
int reload_font(editor *_editor)
{
	UnloadFont(_editor->editor_font);
	_editor->editor_font = LoadFontEx("./mechanical.otf", FONT_SCALE, _editor->font_codepoints, _editor->font_count);
	if (!IsFontValid(_editor->editor_font))
	{
		fprintf(stderr, "failed to reload editor font.\n");
		return -1;
	}
	SetTextureFilter(_editor->editor_font.texture, TEXTURE_FILTER_BILINEAR);
	return 0;
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
	float length_x = MeasureTextEx(_font, "W", FONT_SCALE, FONT_SPACING).x + 4.0f;
	float length_y = (FONT_SCALE / 3);
	
	DrawRectangle(LEFT_MARGIN + char_size.x,
		TOP_MARGIN + FONT_SCALE * _editor->current_line + (length_y * 2) + (0.09f * FONT_SCALE),
		length_x, length_y, RED);
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
		DrawTextEx(_font, b, (Vector2){LEFT_MARGIN, TOP_MARGIN + i * FONT_SCALE}, FONT_SCALE, FONT_SPACING, BLACK);
		free(b);
	}
	return;
}

void save_file(const char *_filepath, editor *_editor);

int x = 0; // that's pretty stupid
void input_processing(editor *_editor)
{
	int key = GetCharPressed();
	if (x > 0)
	{
		if (IsKeyPressed(KEY_S))
			save_file(_editor->current_file, _editor);
		
		x++;
		if (x >= 25)
			x = 0;
	}
	
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
	if (IsKeyPressed(KEY_TAB))
		gb_insert_string(&_editor->gbs[_editor->current_line], "    ");
	
// control keys
	if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
	{
		if (IsKeyPressed(KEY_X))
			x = 1;
		
		if (IsKeyPressed(KEY_V))
		{
			for (int i = 0; i < (gb_get_screen_y() / 2); ++i)
				gb_move_down(_editor);
		}
		
		if (IsKeyPressed(KEY_EQUAL) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))
		{
			FONT_SCALE += FONT_SCALE / 4;
			reload_font(_editor);
		}
		if (IsKeyPressed(KEY_MINUS))
		{
			FONT_SCALE -= (FONT_SCALE / 4);
			reload_font(_editor);
		}
		
		if (IsKeyPressed(KEY_C))
		{
			/// I have a lot of fun writing bad code that works
			text_buffer *current_line = &_editor->gbs[_editor->current_line];
			if (current_line->gap_start == gb_size(current_line))
				return;
			char c = 0;
			if (current_line->gap_start != 0)
			{
				if ((strchr("*.\"\\( ", current_line->buffer[current_line->gap_start]) != NULL &&
					!isupper(current_line->buffer[current_line->gap_start + 1])))
				{
					c = toupper(current_line->buffer[current_line->gap_start + 1]);
					gb_move_right(current_line);
					goto CAPITALIZE;
				}
				if ((strchr("*.\"\\( ", current_line->buffer[current_line->gap_start - 1]) != NULL &&
					!isupper(current_line->buffer[current_line->gap_start])))
				{
					c = toupper(current_line->buffer[current_line->gap_start]);
CAPITALIZE:
					gb_move_right(current_line);
					gb_backspace(_editor);
					gb_insert(_editor, c);
					goto ALT_FOWARD;
				}
			} else
			{
				if (!isupper(current_line->buffer[current_line->gap_start]))
				{
					c = toupper(current_line->buffer[current_line->gap_start]);
					goto CAPITALIZE;
				}
			}
		}
		
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
			if (_editor->gbs[_editor->current_line].gap_start == 0)
			{
				gb_move_up(_editor);
				gb_move_to_index(&_editor->gbs[_editor->current_line], gb_size(&_editor->gbs[_editor->current_line]));
			} else
			{
				char *b = gb_get_buffer(&_editor->gbs[_editor->current_line]);
				int cursor_x = _editor->gbs[_editor->current_line].gap_start;
				size_t char_length;
				calculate_utf8_char(_editor, b, &char_length, false);
				gb_move_to_index(&_editor->gbs[_editor->current_line], (cursor_x - char_length));
				free(b);
			}
		}
		else if (IsKeyPressed(KEY_F))
		{
			if (_editor->gbs[_editor->current_line].gap_start == gb_size(&_editor->gbs[_editor->current_line]))
			{
				gb_move_down(_editor);
				gb_move_to_index(&_editor->gbs[_editor->current_line], 0);
			} else
			{
				char *b = gb_get_buffer(&_editor->gbs[_editor->current_line]);
				int cursor_x = _editor->gbs[_editor->current_line].gap_start;
				size_t char_length;
				calculate_utf8_char(_editor, b, &char_length, true);
				gb_move_to_index(&_editor->gbs[_editor->current_line], (cursor_x + char_length));
				free(b);
			}
		}
	}
	else if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))
	{
		if (IsKeyPressed(KEY_A))
		{
			for (int i = 0; i < (gb_get_screen_y() / 2); ++i)
				gb_move_up(_editor);
		}
		
		if (IsKeyPressed(KEY_B))
		{
			text_buffer *current_line = &_editor->gbs[_editor->current_line];
			int index = _editor->gbs[_editor->current_line].gap_start;
			if (index > 0)
			{
				for (; strchr("*#.-\"'/\\(; ", current_line->buffer[index - 2]) == NULL && index > 0;)
					--index;
				gb_move_to_index(current_line, index - 1);
			}
			else if (index == 0 && _editor->current_line > 0)
			{
				_editor->current_line--;
				current_line = &_editor->gbs[_editor->current_line];
				gb_move_to_index(current_line, gb_size(current_line));
			}
		}
		else if (IsKeyPressed(KEY_F))
		{
ALT_FOWARD:
			text_buffer *current_line = &_editor->gbs[_editor->current_line];
			int index = _editor->gbs[_editor->current_line].gap_start;
			if (index < gb_size(current_line))
			{
				for (; strchr("*#.-\"'/\\(; ", current_line->buffer[index + 2]) == NULL && index < gb_size(current_line);)
					++index;
				gb_move_to_index(current_line, index + 2);
			}
			else if (index == gb_size(current_line) && _editor->current_line < _editor->lines)
			{
				_editor->current_line++;
				current_line = &_editor->gbs[_editor->current_line];
				gb_move_to_index(current_line, 0);
			}
		}
	}
	return;
}

void save_file(const char *_filepath, editor *_editor)
{
	if (_filepath == NULL)
	{
		fprintf(stderr, "file not found.\n");
		return;
	}
	FILE *file = fopen(_filepath, "w");
	if (file == NULL)
	{
		fprintf(stderr, "'%s' file not found.\n", _filepath);
		return;
	}
	for (int i = 0; i < _editor->lines; ++i)
	{
		text_buffer *current_line = &_editor->gbs[i];
		char *tmp = gb_get_buffer(current_line);
		if (tmp == NULL)
		{
			fprintf(stderr, "failed to get buffer to save file.\n");
			return;
		}
		
		fputs(tmp, file);
		fputc('\n', file);
		
		free(tmp);
	}
	fclose(file);
	printf("'%s' saved sucessfully.\n", _filepath);
	return;
}
void open_file(const char *_filepath, editor *_editor)
{
	if (_editor->current_file != NULL)
		free(_editor->current_file);
	_editor->current_file = strdup(_filepath);
	
	_editor->current_line = _editor->lines;
	gb_move_to_index(&_editor->gbs[_editor->current_line], gb_size(&_editor->gbs[_editor->current_line]));
	for (int i = _editor->lines; i >= 0; --i)
	{
		gb_clean_line(&_editor->gbs[i]);
		gb_backspace(_editor);
	}
		
	FILE *file = fopen(_filepath, "r");
	if (file == NULL)
	{
		fprintf(stderr, "'%s' file not found.\n", _filepath);
		exit(EXIT_FAILURE);
		return;
	}
	char *tmp = (char*)malloc(BUFFER_SIZE);
	if (tmp == NULL)
	{
		fprintf(stderr, "failed to allocate memory for tmp buffer.\n");
		exit(EXIT_FAILURE);
		return;
	}
	gb_move_to_index(&_editor->gbs[0], 0);
	_editor->current_line = 0;
	while (fgets(tmp, BUFFER_SIZE, file) != NULL)
	{
		tmp[strcspn(tmp, "\n")] = '\0';
		char t[BUFFER_SIZE];
		size_t j = 0;
		for (size_t  i = 0; tmp[i] != '\0' && j < BUFFER_SIZE - 1; ++i)
		{
			if (tmp[i] == '\t')
			{
				for (int k = 0; k < TAB_SIZE && j < BUFFER_SIZE - 1; ++k)
					t[j++] = ' ';
			} else
			{
				t[j++] = tmp[i];
			}
		}
		t[j] = '\0';		
		gb_insert_string(&_editor->gbs[_editor->current_line], converted);
		gb_insert_line(_editor);
	}
	gb_move_to_index(&_editor->gbs[0], gb_size(&_editor->gbs[0]));
	_editor->current_line = 0;
	
	fclose(file);
	free(tmp);
	return;
}

void dropped_file_processing(editor *_editor)
{
	if (IsFileDropped())
	{
		FilePathList dropped_files = LoadDroppedFiles();
		if (IsTextFile(dropped_files.paths[0]))
//			printf("%s.\n", dropped_files.paths[0]);
			open_file(dropped_files.paths[0], _editor);
		UnloadDroppedFiles(dropped_files);
	}
	return;
}

void main_loop(editor *_editor)
{
	while (!WindowShouldClose())
	{
		dropped_file_processing(_editor);
		input_processing(_editor);
		center_screen_cursor(_editor);
		BeginDrawing();
			ClearBackground(WHITE);
			BeginMode2D(_editor->camera);
				text_rendering(_editor, _editor->editor_font);
				cursor_rendering(_editor, _editor->editor_font);
			EndMode2D();
		EndDrawing();
	}
	return;
}

int main(int argc, char**argv)
{
	editor e; gb_init(&e, BUFFER_SIZE);
	
	const char *base_text = "Hello, Clarice.";
	gb_insert_string(&e.gbs[e.current_line], base_text);
	
	init_window();

	init_font(&e);
	
	main_loop(&e);
	
	gb_free(&e);
	return 0;
}
