
#include <raylib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define BUFFER_SIZE 2048

#define FONT_SPACING 2.0f
#define FONT_SCALE 25.0f

char buffer[BUFFER_SIZE][BUFFER_SIZE];
size_t buffer_size;

unsigned int cursor_x = 0, cursor_y = 0;
unsigned int lines = 1;
bool dirty = false;

static void init_window(void)
{
	const int WIDTH = 800;
	const int HEIGHT = WIDTH / 4*3;
	InitWindow(WIDTH, HEIGHT, "Pessoa");
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
			// that's pretty dumb, but works (for now);
			if (buffer[cursor_y][buffer_size - 1] < 0)
				buffer_size -= 2;
			else
				buffer_size--;
			buffer[cursor_y][buffer_size] = '\0';
			cursor_x = buffer_size;
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
			if (IsKeyPressed(KEY_B))
			{
				if (buffer[cursor_y][cursor_x - 1] < 0)
					cursor_x -= 2;
				else
					cursor_x--;
				char_size = calculate_glyph(_font);
			}
		}
		
		if (dirty == true)
			char_size = calculate_glyph(_font);
//		printf("%d\n", cursor_x);
		BeginDrawing();
			ClearBackground(BLACK);
			for (int i = 0; i < lines; ++i)
			{
				DrawTextEx(_font, buffer[i], (Vector2){15.0f, i * FONT_SCALE}, FONT_SCALE, FONT_SPACING, WHITE);
			}
			DrawRectangle(char_size.x + 16.0f, FONT_SCALE * cursor_y, 16, FONT_SCALE, RED);
		EndDrawing();
	}
	return 0;
}


