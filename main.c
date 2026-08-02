
#include <raylib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define BUFFER_SIZE 2048

#define FONT_SPACING 2.0f
#define FONT_SCALE 25.0f

#define LEFT_MARGIN 16.0f
#define TOP_MARGIN 16.0f

char buffer[BUFFER_SIZE][BUFFER_SIZE];
size_t buffer_size;

unsigned int cursor_x = 0, cursor_y = 0;
unsigned int lines = 1;
bool dirty = false;

static void init_window(void)
{
	const int WIDTH = 800;
	const int HEIGHT = WIDTH / 4*3;
	InitWindow(WIDTH, HEIGHT, "Clarice");
	SetTargetFPS(60);
	return;
}

static Vector2 calculate_glyph(Font _font)
{
	size_t length = cursor_x;
	char dest[length];
	
	strncpy(dest, buffer[cursor_y], cursor_x);
	dest[length] = '\0';
	
//	Vector2 text_measure = MeasureTextEx(_font, buffer[cursor_y], FONT_SCALE, FONT_SPACING);
	Vector2 text_measure = MeasureTextEx(_font, dest, FONT_SCALE, FONT_SPACING);
	dirty = false;
	
	return text_measure;
}

// that's dumb I know sorry for that
static inline void calculate_utf8_char(size_t *char_start, size_t *char_length, bool subsequent)
{
	if (subsequent)
		*char_start = cursor_x + 1;
	else
		*char_start = cursor_x - 1;
	while (*char_start > 0 && (buffer[cursor_y][*char_start] & 0xC0) == 0x80)
	{
		if (subsequent)
			*char_start += 1;
		else
			*char_start -= 1;
	}
	*char_length = cursor_x - *char_start;
	// HAHAHA I have a lot of fun
	if (subsequent)
		*char_length *= -1;
	return;
}

int main(int argc, char **argv)
{
	init_window();
	
	// BOOOOOOOOOOOOOOOOORING
	int count = 0; int codepoints[256];
	for (int i = 32; i <= 255; i++)
		codepoints[count++] = i;
	
	Font _font = LoadFontEx("./mechanical.otf", 24, codepoints, count);
	if (!IsFontValid(_font))
	{
		fprintf(stderr, "error: invalid font.\n");
		return -1;
	}
	buffer_size = strlen(buffer[cursor_y]);
	cursor_x = buffer_size;
	
	Vector2 char_size;
	while (!WindowShouldClose())
	{
		int key = GetCharPressed();
		while (key > 0)
		{
			int utf8_size = 0;
			const char *utf8 = CodepointToUTF8(key, &utf8_size);
			
			if (buffer_size + utf8_size < (BUFFER_SIZE - 1))
			{
				size_t length = strlen(buffer[cursor_y]);
				memmove(buffer[cursor_y] + cursor_x + utf8_size, buffer[cursor_y] + cursor_x, length - cursor_x + 1);
				memcpy(buffer[cursor_y] + cursor_x, utf8, utf8_size);
				
				buffer_size += utf8_size;
				buffer[cursor_y][buffer_size] = '\0';
				cursor_x += utf8_size;
				dirty = true;
			}
			key = GetCharPressed();
		}
		if (IsKeyPressed(KEY_BACKSPACE) && buffer_size > 0)
		{
			size_t char_length, char_start;
			calculate_utf8_char(&char_start, &char_length, false);
			memmove(buffer[cursor_y] + char_start, buffer[cursor_y] + cursor_x, strlen(buffer[cursor_y]) + 1);
			buffer_size -= char_length;
			cursor_x = char_start;
			
			dirty = true;
		}
		if (IsKeyPressed(KEY_ENTER) || (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_M)))
		{
			if (lines < BUFFER_SIZE - 2)
			{
				for (int i = lines; i > cursor_y; --i)
				{
					strcpy(buffer[i], buffer[i - 1]);
				}
				buffer[cursor_y + 1][0] = '\0';
				lines++; cursor_y++;
				buffer_size = strlen(buffer[cursor_y]);
				cursor_x = buffer_size;
				char_size = calculate_glyph(_font);
			}
		}
		if (IsKeyDown(KEY_LEFT_CONTROL))
		{
			if (IsKeyPressed(KEY_N) && cursor_y < lines - 1)
			{
				cursor_y++;
				buffer_size = strlen(buffer[cursor_y]);
				cursor_x = buffer_size;
				char_size = calculate_glyph(_font);
			}
			else if (IsKeyPressed(KEY_P) && cursor_y > 0)
			{
				cursor_y--;
				buffer_size = strlen(buffer[cursor_y]);
				cursor_x = buffer_size;
				char_size = calculate_glyph(_font);
			}
			if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_F))
			{
				size_t char_length, char_start;
				if (IsKeyPressed(KEY_B) && cursor_x > 0)
				{
					calculate_utf8_char(&char_start, &char_length, false);
					cursor_x = char_start;
				}
				else if (IsKeyPressed(KEY_F) && cursor_x < buffer_size)
				{
					calculate_utf8_char(&char_start, &char_length, true);
					cursor_x += char_length;
				}
				char_size = calculate_glyph(_font);
			}
		}
		if (dirty == true)
			char_size = calculate_glyph(_font);
		BeginDrawing();
			ClearBackground(BLACK);
			for (int i = 0; i < lines; ++i)
				DrawTextEx(_font, buffer[i], (Vector2){15.0f, i * FONT_SCALE + TOP_MARGIN}, FONT_SCALE, FONT_SPACING, WHITE);
			DrawRectangle(char_size.x + LEFT_MARGIN, FONT_SCALE * cursor_y + TOP_MARGIN, FONT_SPACING, FONT_SCALE, RED);
		EndDrawing();
	}
	return 0;
}


