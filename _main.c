
#include <raylib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define BUFFER_SIZE 4096

#define FONT_SPACING 2.0f

#define LEFT_MARGIN 15.0f
#define TOP_MARGIN 5.0f
#define BOTTOM_MARGIN 15.0f

#define LINE_NUMBERS true

#define TAB_SIZE 4

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = WINDOW_WIDTH / 4*3;

float FONT_SCALE = 24.0f;

char buffer[BUFFER_SIZE][BUFFER_SIZE];
size_t buffer_size;

unsigned int screen_x = 0, screen_y = 0;
unsigned int scroll_x = 1, scroll_y = 1;
unsigned int cursor_x = 0, cursor_y = 0;
unsigned int lines = 1;

Camera2D camera;

char *current_file;
char *message;

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

static void open(const char *filepath)
{
	FILE *file = fopen(filepath, "r");
	if (!file)
	{
		fprintf(stderr, "file not found.\n");
		exit(EXIT_FAILURE);
		return;
	}
	if (current_file != NULL)
		free(current_file);
	current_file = strndup(filepath, strlen(filepath) + 1);
	
	while (lines < BUFFER_SIZE && fgets(buffer[lines], BUFFER_SIZE, file) != NULL)
	{
		buffer[lines][strcspn(buffer[lines], "\n")] = '\0';
		
		for (int i = 0; buffer[lines][i] != '\0'; ++i)
		{
			if (buffer[lines][i] == '	') // just because of charlie
			{
				if ((strlen(buffer[lines]) + TAB_SIZE - 1) < BUFFER_SIZE - 1)
				{
					for (int j = strlen(buffer[lines]); j >= i; --j)
						buffer[lines][j + TAB_SIZE - 1] = buffer[lines][j];
					// hardcoded idc
					buffer[lines][i] = ' ';
					buffer[lines][i + 1] = ' ';
					buffer[lines][i + 2] = ' ';
					buffer[lines][i + 3] = ' ';
					i += TAB_SIZE;
				}
			}
		}
		lines++;
	}
	fclose(file);
	cursor_y = 0; cursor_x = 0;
	buffer_size = strlen(buffer[cursor_y]);
	return;
}

// that's also very dumb!!!
static void update_scroll(void)
{
/*
	if (cursor_y > screen_y)
	{
		if (cursor_y > ((screen_y / 2) * scroll_y))
		{
			camera.target.y = cursor_y * FONT_SCALE;
			scroll_y++;
		}
		else if (cursor_y < (((screen_y * scroll_y) / 2) - screen_y))
		{
			camera.target.y = cursor_y * FONT_SCALE;
			scroll_y--;
		}
	} else
	{
		camera.target = (Vector2){WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
		scroll_y = 1;
	}
	return;
*/
}

static void exit_editor()
{
	if (current_file != NULL)
		free(current_file);
	if (message != NULL)
		free(message);
	return;
}

int main(int argc, char **argv)
{
	init_window();
	if (argc >= 2)
		open(argv[1]);
	
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
	
	camera.target = (Vector2){WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
	camera.offset = (Vector2){ WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f };
	camera.rotation = 0.0f; camera.zoom = 1.0f;
	
	// initializing screen_n variables;
	for (int i = 0; (i * FONT_SCALE + TOP_MARGIN) < (WINDOW_HEIGHT - FONT_SCALE - BOTTOM_MARGIN); ++i)
		screen_y++;
	printf("screen_y: %d.\n", screen_y);
	
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
				
				char_size = calculate_glyph(_font);
			}
			
			key = GetCharPressed();
		}
		if (IsKeyPressed(KEY_TAB))
		{
			if (buffer_size + TAB_SIZE < (BUFFER_SIZE - 1))
			{
				size_t length = strlen(buffer[cursor_y]);
				memmove(buffer[cursor_y] + cursor_x + TAB_SIZE, buffer[cursor_y] + cursor_x, length - cursor_x + 1);
				memcpy(buffer[cursor_y] + cursor_x, "    ", TAB_SIZE);
				
				buffer_size += TAB_SIZE;
				buffer[cursor_y][buffer_size] = '\0';
				cursor_x += TAB_SIZE;
				char_size = calculate_glyph(_font);
			}
		}
		if (IsKeyPressed(KEY_BACKSPACE))
		{
			if (cursor_x > 0 && buffer_size > 0)
			{
				// hardcoded idc
				if (cursor_x >= TAB_SIZE &&
					buffer[cursor_y][cursor_x - 1] == ' ' &&
					buffer[cursor_y][cursor_x - 2] == ' ' &&
					buffer[cursor_y][cursor_x - 3] == ' '
				)
				{
					strcpy(buffer[cursor_y] + (cursor_x - TAB_SIZE), buffer[cursor_y] + cursor_x);
					buffer_size -= TAB_SIZE;
					cursor_x -= TAB_SIZE;
					char_size = calculate_glyph(_font);
				} else
				{
					size_t char_length, char_start;
					calculate_utf8_char(&char_start, &char_length, false);
					strcpy(buffer[cursor_y] + char_start, buffer[cursor_y] + cursor_x);
					buffer_size -= char_length;
					cursor_x = char_start;
					char_size = calculate_glyph(_font);
				}
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
				update_scroll();
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
				update_scroll();
			}
		}
		if (IsKeyDown(KEY_LEFT_CONTROL))
		{
			if (IsKeyPressed(KEY_S))
			{
				message = strdup("saved!");
			}
			if ((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && IsKeyPressed(KEY_EQUAL))
			{
				FONT_SCALE += 2.0f;
				char_size = calculate_glyph(_font);
				printf("font_scale: %.1f.\n", FONT_SCALE);
			}
			else if (IsKeyPressed(KEY_MINUS))
			{
				FONT_SCALE -= 2.0f;
				char_size = calculate_glyph(_font);
				printf("font_scale: %.1f.\n", FONT_SCALE);
			}
			
			if (IsKeyPressed(KEY_V))
			{
				int tmp = cursor_y + (screen_y / 2);
				if (tmp < lines - 1)
					cursor_y = tmp;
				else
					cursor_y = lines - 1;
				buffer_size = strlen(buffer[cursor_y]);
				if (cursor_x > buffer_size)
					cursor_x = buffer_size;
				char_size = calculate_glyph(_font);
				update_scroll();
			}
			
			if (IsKeyPressed(KEY_L))
			{
				camera.target.y = cursor_y * FONT_SCALE;
			}
			
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
				update_scroll();
			}			
			else if (IsKeyPressed(KEY_P) && cursor_y > 0)
			{
				cursor_y--;
				buffer_size = strlen(buffer[cursor_y]);
				if (cursor_x > buffer_size)
					cursor_x = buffer_size;
				char_size = calculate_glyph(_font);
				update_scroll();
			}
			
			if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_F))
			{
				size_t char_length, char_start;
				if (IsKeyPressed(KEY_B))
				{
					if (cursor_x > 0)
					{
						if (cursor_x >= TAB_SIZE &&
							buffer[cursor_y][cursor_x - 1] == ' ' &&
							buffer[cursor_y][cursor_x - 2] == ' ' &&
							buffer[cursor_y][cursor_x - 3] == ' '
						)
							cursor_x -= TAB_SIZE;
						else if (cursor_x > 0)
						{
							calculate_utf8_char(&char_start, &char_length, false);
							cursor_x = char_start;
						}
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
						if ((cursor_x + TAB_SIZE) < buffer_size && 
								buffer[cursor_y][cursor_x + 1] == ' ' &&
								buffer[cursor_y][cursor_x + 2] == ' ' &&
								buffer[cursor_y][cursor_x + 3] == ' '
						)
							cursor_x += TAB_SIZE;
						else if (cursor_x < buffer_size)
						{
							calculate_utf8_char(&char_start, &char_length, true);
							cursor_x += char_length;
						}
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
			if (IsKeyPressed(KEY_A))
			{
				int tmp = cursor_y - (screen_y / 2);
				if (tmp > 0)
					cursor_y = tmp;
				else
					cursor_y = 0;
				buffer_size = strlen(buffer[cursor_y]);
				if (cursor_x > buffer_size)
					cursor_x = buffer_size;
				char_size = calculate_glyph(_font);
				update_scroll();
			}
			
			if (IsKeyPressed(KEY_P) && cursor_y > 0)
			{
				if (strlen(buffer[cursor_y - 1]) == 0)
				{
					strcpy(buffer[cursor_y - 1], buffer[cursor_y]);
					buffer[cursor_y][0] = '\0';
					cursor_y--; buffer_size = strlen(buffer[cursor_y]);
				} else
				{
					char tmp[sizeof(buffer[cursor_y - 1] + 1)];
					
					strcpy(tmp, buffer[cursor_y - 1]);
					strcpy(buffer[cursor_y - 1], buffer[cursor_y]);
					strcpy(buffer[cursor_y], tmp);
					
					cursor_y--; buffer_size = strlen(buffer[cursor_y]);
				}
				update_scroll();
			}
			else if (IsKeyPressed(KEY_N) && cursor_y < lines - 1)
			{
				if (strlen(buffer[cursor_y + 1]) == 0)
				{
					strcpy(buffer[cursor_y + 1], buffer[cursor_y]);
					buffer[cursor_y][0] = '\0';
					cursor_y++; buffer_size = strlen(buffer[cursor_y]);
				} else
				{
					char tmp[sizeof(buffer[cursor_y + 1] + 1)];
					
					strcpy(tmp, buffer[cursor_y + 1]);
					strcpy(buffer[cursor_y + 1], buffer[cursor_y]);
					strcpy(buffer[cursor_y], tmp);
					
					cursor_y++; buffer_size = strlen(buffer[cursor_y]);
				}
				update_scroll();
			}
			
			if (IsKeyPressed(KEY_B))
			{
				if (cursor_x != 0)
				{
ALT_BACK:
					int index = cursor_x - 1;
					for (; strchr("#.-\"(; ", buffer[cursor_y][index - 1]) == NULL && index != 0;)
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
						update_scroll();
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
					for (; strchr("#.-\"(,; ", buffer[cursor_y][index + 1]) == NULL && index != buffer_size;)
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
						update_scroll();
					}
				}
				char_size = calculate_glyph(_font);
			}
		}
		
// drawing
		BeginDrawing();
			ClearBackground(WHITE);
			BeginMode2D(camera);
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
							   FONT_SCALE, FONT_SPACING, BLACK);
				}
				DrawRectangle(char_size.x + LEFT_MARGIN + line_number_length, FONT_SCALE * cursor_y + TOP_MARGIN/* + FONT_SCALE - 5*/,
							  ((MeasureTextEx(_font, "A", FONT_SCALE, FONT_SPACING).x + 5) / 3), FONT_SCALE, RED);
			EndMode2D();
			DrawRectangle(0.0f, WINDOW_HEIGHT - (FONT_SCALE * 2), WINDOW_WIDTH, FONT_SCALE * 2, BLACK);
			
			const char *info_base = "[%s] [%d/%d][%d/%d]";
			size_t length;
			if (current_file != NULL)
				length = strlen(info_base) + (sizeof(int) * 4) + strlen(current_file);
			else
				length = strlen(info_base) + (sizeof(int) * 4);
			char *info = (char*)malloc(length);
			if (info == NULL)
				return -1;
			snprintf(info, length, info_base, (current_file == NULL) ? "new file" : current_file, cursor_y, lines - 1, cursor_x, buffer_size);
			DrawTextEx(_font, info, (Vector2){LEFT_MARGIN, WINDOW_HEIGHT - (FONT_SCALE * 2)}, FONT_SCALE, FONT_SPACING, WHITE);
			if (message != NULL)
				DrawTextEx(_font, message, (Vector2){LEFT_MARGIN, WINDOW_HEIGHT - FONT_SCALE}, FONT_SCALE, FONT_SPACING, WHITE);
//			DrawFPS(WINDOW_WIDTH - 50.0f, WINDOW_HEIGHT - 25.0f);
		EndDrawing();
	}
	exit_editor();
	return 0;
}


