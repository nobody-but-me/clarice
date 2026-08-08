	
#include <raylib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define str(s) #s

#define BUFFER_SIZE 2048

#define FONT_SPACING 2.0f
#define FONT_SCALE 25.0f

#define LEFT_MARGIN 15.0f
#define TOP_MARGIN 16.0f

#define LINE_NUMBERS true

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = WINDOW_WIDTH / 4*3;

char buffer[BUFFER_SIZE][BUFFER_SIZE];
size_t buffer_size;

unsigned int cursor_x = 0, cursor_y = 0;
unsigned int lines = 1;
bool dirty = false;

static void init_window(void)
{
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Clarice text editor");
	SetTargetFPS(120);
	return;
}

static Vector2 calculate_glyph(Font _font)
{
	size_t length = cursor_x;
	char dest[length];
	
	strncpy(dest, buffer[cursor_y], cursor_x);
	dest[length] = '\0';
	
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
	
	Vector2 char_size = calculate_glyph(_font);
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
		if (IsKeyPressed(KEY_BACKSPACE))
		{
			if (cursor_x > 0 && buffer_size > 0)
			{
				size_t char_length, char_start;
				calculate_utf8_char(&char_start, &char_length, false);
				strcpy(buffer[cursor_y] + char_start, buffer[cursor_y] + cursor_x);
				buffer_size -= char_length;
				cursor_x = char_start;
				
				dirty = true;
			}
			else if (cursor_x == 0 && cursor_y > 0)
			{
				if (strlen(buffer[cursor_y - 1]) == 0)
				{
					strcpy(buffer[cursor_y - 1], buffer[cursor_y]);
					for (int i = cursor_y; i < lines - 1; ++i)
						strcpy(buffer[i], buffer[i + 1]);
					cursor_x = 0;
				} else
				{
					cursor_x = strlen(buffer[cursor_y - 1]);
					strcpy(buffer[cursor_y - 1] + strlen(buffer[cursor_y - 1]), buffer[cursor_y]);
					for (int i = cursor_y; i < lines - 1; ++i)
						strcpy(buffer[i], buffer[i + 1]);
				}
				lines--; cursor_y--;
				
				buffer_size = strlen(buffer[cursor_y]);
				char_size = calculate_glyph(_font);
			}
		}
		if (IsKeyPressed(KEY_ENTER) || (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_M)))
		{
			if (lines < BUFFER_SIZE - 2)
			{
				if (cursor_x == buffer_size)
				{
					for (int i = lines; i > cursor_y; --i)
					{
						if (strlen(buffer[i - 1]) != 0)
							strcpy(buffer[i], buffer[i - 1]);
						else
							buffer[i][0] = '\0';
					}
					buffer[cursor_y + 1][0] = '\0';
					lines++; cursor_y++;
					buffer_size = strlen(buffer[cursor_y]);
					cursor_x = buffer_size;
					char_size = calculate_glyph(_font);
				} else
				{
					for (int i = lines; i > cursor_y + 1; --i)
					{
						if (strlen(buffer[i - 1]) != 0)
							strcpy(buffer[i], buffer[i - 1]);
						else
							buffer[i][0] = '\0';
					}
					strcpy(buffer[cursor_y + 1], buffer[cursor_y] + cursor_x);
					buffer[cursor_y + 1][strlen(buffer[cursor_y + 1])] = '\0';
					buffer[cursor_y][cursor_x] = '\0';
					lines++; cursor_y++;
					
					buffer_size = strlen(buffer[cursor_y]);
					cursor_x = 0;
					char_size = calculate_glyph(_font);
				}
			}
		}
		if (IsKeyDown(KEY_LEFT_CONTROL))
		{
			if (IsKeyPressed(KEY_A) && cursor_x > 0)
			{
				cursor_x = 0;
				char_size = calculate_glyph(_font);
			}
			else if (IsKeyPressed(KEY_E) && cursor_x < buffer_size)
			{
				cursor_x = buffer_size;
				char_size = calculate_glyph(_font);
			}

			if (IsKeyPressed(KEY_N) && cursor_y < lines - 1)
			{
				cursor_y++;
				buffer_size = strlen(buffer[cursor_y]);
				if (cursor_x > buffer_size)
					cursor_x = buffer_size;
				char_size = calculate_glyph(_font);
			}			
			else if (IsKeyPressed(KEY_P) && cursor_y > 0)
			{
				cursor_y--;
				buffer_size = strlen(buffer[cursor_y]);
				if (cursor_x > buffer_size)
					cursor_x = buffer_size;
				char_size = calculate_glyph(_font);
			}
			
			if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_F))
			{
				size_t char_length, char_start;
				if (IsKeyPressed(KEY_B))
				{
					if (cursor_x > 0)
					{
						calculate_utf8_char(&char_start, &char_length, false);
						cursor_x = char_start;
					} else
					{
						cursor_y--;
						cursor_x = strlen(buffer[cursor_y]);
					}
				}
				else if (IsKeyPressed(KEY_F) && cursor_x < buffer_size)
				{
					if (cursor_x < buffer_size)
					{
						calculate_utf8_char(&char_start, &char_length, true);
						cursor_x += char_length;
					} else
					{
						cursor_y++;
						cursor_x = 0;
					}
				}
				char_size = calculate_glyph(_font);
			}
		}
		else if (IsKeyDown(KEY_LEFT_ALT))
		{
			if (IsKeyPressed(KEY_B))
			{
				if (cursor_x != 0)
				{
ALT_BACK:
					int index = cursor_x - 1;
					for (; strchr(".-\"(; ", buffer[cursor_y][index - 1]) == NULL && index != 0;)
					{
						if (index >= 1)
							--index;
						else
							index = 0;
					}
					cursor_x = index;
				} else
				{
ALT_BACK_LINE:
					if (cursor_y != 0)
					{
						cursor_y--; buffer_size = strlen(buffer[cursor_y]);
						cursor_x = buffer_size;
						if (buffer_size >= 1)
							goto ALT_BACK;
						else
							goto ALT_BACK_LINE;
					}
				}
				char_size = calculate_glyph(_font);
			}
			else if (IsKeyPressed(KEY_F))
			{
				if (cursor_x < buffer_size)
				{
ALT_FOWARD:
					int index = cursor_x + 1;
					for (; strchr(".-\"(,; ", buffer[cursor_y][index + 1]) == NULL && index != buffer_size;)
					{
						if (index <= buffer_size - 1)
							++index;
						else
							index = buffer_size - 1;
					}
					cursor_x = index + 1;
				} else
				{
ALT_FOWARD_LINE:
					if (cursor_y < lines - 1)
					{
						cursor_y++; buffer_size = strlen(buffer[cursor_y]);
						cursor_x = 0;
						if (buffer_size >= 1)
							goto ALT_FOWARD;
						else
							goto ALT_FOWARD_LINE;
					}
				}
				char_size = calculate_glyph(_font);
			}
		}
		if (dirty == true)
			char_size = calculate_glyph(_font);
		BeginDrawing();
			ClearBackground(BLACK);
			int line_number_length;
			for (int i = 0; i < lines; ++i)
			{
				if (LINE_NUMBERS)
				{
					char line_number[sizeof(int) * sizeof(char)];
					snprintf(line_number, sizeof(line_number), "%d", i);
					DrawTextEx(_font, line_number, (Vector2){5.0f, i * FONT_SCALE + TOP_MARGIN}, FONT_SCALE, FONT_SPACING, GRAY);
					if (i == lines - 1)
						line_number_length = MeasureTextEx(_font, line_number, FONT_SCALE, FONT_SPACING).x;
				}
				DrawTextEx(_font, buffer[i], 
						   (Vector2){LEFT_MARGIN + line_number_length, i * FONT_SCALE + TOP_MARGIN}, 
						   FONT_SCALE, FONT_SPACING, WHITE);
			}
			DrawRectangle(char_size.x + LEFT_MARGIN + line_number_length, FONT_SCALE * cursor_y + TOP_MARGIN + FONT_SCALE - 5,
						  MeasureTextEx(_font, "A", FONT_SCALE, FONT_SPACING).x + 5, 5, RED);
			DrawFPS(WINDOW_WIDTH - 50.0f, WINDOW_HEIGHT - 25.0f);
		EndDrawing();
	}
	return 0;
}


