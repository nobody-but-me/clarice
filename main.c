
// don't look at it, please....

#include <raylib.h>
#include <string.h>

#include <ctype.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <sys/stat.h>

#define TO_STRING(x) #x

#define BUFFER_SIZE 1024

#define BOTTOM_MARGIN 15.0f
#define LEFT_MARGIN 15.0f
#define RIGHT_MARGIN 0.0f
#define TOP_MARGIN 15.0f

#define LINE_NUMBERS true
#define TAB_SIZE 4

#define INITIAL_WINDOW_WIDTH 800.0f
#define INITIAL_WINDOW_HEIGHT INITIAL_WINDOW_WIDTH / 4*3

float WINDOW_WIDTH = INITIAL_WINDOW_WIDTH;
float WINDOW_HEIGHT = INITIAL_WINDOW_HEIGHT;

float LINE_NUMBER_MARGIN = 0.0f;

#define SEARCHING_MODE 1
#define NORMAL_MODE    0

bool finished_searching = false;
int current_mode = NORMAL_MODE;

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
    
    int dirty;
} editor;

typedef struct
{
    size_t column;
    size_t line;
} search_result;

float FONT_SPACING = 2.0f;
float FONT_SCALE = 24.0f;

#define BAR_SCALE FONT_SCALE + 5.0f

void update_window_size()
{
    WINDOW_WIDTH = GetScreenWidth();
    WINDOW_HEIGHT = GetScreenHeight();
    return;
}

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
            c != '\n' && c != '\r')
            return false;
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

static Vector2 calculate_glyph(const char *_buffer, size_t _pos, Font _font)
{
    size_t length = _pos;
    char dest[length];
    strncpy(dest, _buffer, _pos);
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
    if (_logical < _gap_buffer->gap_start)
        return _gap_buffer->buffer[_logical];
    return _gap_buffer->buffer[_logical + (_gap_buffer->gap_end - _gap_buffer->gap_start)];
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
//    gb_grow(&_editor->gbs[0], _capacity);
    gb_grow(&_editor->gbs[0], 15);
    _editor->gbs[0].initialized = true;
    _editor->current_line = 0;
    _editor->lines = 0;
    
    _editor->camera.target = (Vector2){WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f };
    _editor->camera.offset = (Vector2){ WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f - (BAR_SCALE * 2)};
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
    if (IsFontValid(_editor->editor_font))
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
    for (size_t i = 0; (i * FONT_SCALE + LEFT_MARGIN) < (WINDOW_WIDTH - FONT_SCALE - RIGHT_MARGIN); ++i)
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
void gb_clean_all(editor *_editor)
{
    _editor->current_line = _editor->lines;
    gb_move_to_index(&_editor->gbs[_editor->current_line], gb_size(&_editor->gbs[_editor->current_line]));
    for (int i = _editor->lines; i >= 0; --i)
    {
        gb_clean_line(&_editor->gbs[i]);
        gb_backspace(_editor);
    }
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
void gb_insert(text_buffer *_gap_buffer, char c)
{
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
    Vector2 char_size = calculate_glyph(_editor->gbs[_editor->current_line].buffer, _editor->gbs[_editor->current_line].gap_start, _editor->editor_font);
    if (current_line->gap_start > gb_get_screen_x() + LEFT_MARGIN)
        _editor->camera.target.x = lerp(_editor->camera.target.x, LEFT_MARGIN + LINE_NUMBER_MARGIN + char_size.x, 0.3f);
    else
        _editor->camera.target.x = lerp(_editor->camera.target.x, (INITIAL_WINDOW_WIDTH / 2.0f), 0.3f);
        
    _editor->camera.target.y = lerp(_editor->camera.target.y, _editor->current_line * FONT_SCALE - BAR_SCALE, 0.3f);
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
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Clarice Text Editor");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    return;
}

void cursor_rendering(editor *_editor, Font _font)
{
    Vector2 char_size = calculate_glyph(_editor->gbs[_editor->current_line].buffer, _editor->gbs[_editor->current_line].gap_start, _font);
    float length_x = MeasureTextEx(_font, "W", FONT_SCALE, FONT_SPACING).x;
    float length_y = (FONT_SCALE / 3);
    
    DrawRectangle(LEFT_MARGIN + LINE_NUMBER_MARGIN + char_size.x,
        TOP_MARGIN + FONT_SCALE * _editor->current_line + (length_y * 2) + (0.09f * FONT_SCALE),
        length_x + 4.0f, length_y, RED);
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
/*
//        printf("%zu.\n", strlen(b));
        char expanded[strlen(b) + 1];
        size_t j = 0;
        for (size_t y = 0; b[y] != '\0' && j < strlen(b); y++)
        {
            if (b[y] == '\t')
                for (int k = 0; k < TAB_SIZE && j < strlen(b); k++)
                    expanded[j++] = ' ';
            else
                expanded[j++] = b[y];
        }
        expanded[j] = '\0';
//        printf("%s\n", expanded);
*/
        DrawTextEx(_font, b, (Vector2){LEFT_MARGIN + LINE_NUMBER_MARGIN, TOP_MARGIN + i * FONT_SCALE}, FONT_SCALE, FONT_SPACING, BLACK);
        
        if (LINE_NUMBERS)
        {
            char line_number[12];
            snprintf(line_number, sizeof(line_number), "%d", i);
            
            DrawTextEx(_font, line_number, (Vector2){LEFT_MARGIN, TOP_MARGIN + i * FONT_SCALE}, FONT_SCALE, FONT_SPACING, GRAY);
            if (i == _editor->lines)
                LINE_NUMBER_MARGIN = MeasureTextEx(_font, line_number, FONT_SCALE, FONT_SPACING).x + LEFT_MARGIN;
        }
        free(b);
    }
    return;
}

void gui_rendering(editor *_editor, editor *_searching_e, Font _font)
{
    DrawRectangle(0.0f, WINDOW_HEIGHT - FONT_SCALE - 5.0f, WINDOW_WIDTH, BAR_SCALE, RED);
    if (current_mode == NORMAL_MODE)
    {
        char status[150];//, rstatus[100];
        float lines_percentage = 0.0f;
        if (_editor->lines > 0)
            lines_percentage = (float)_editor->current_line / _editor->lines * 100.0f;
        
        const char *filename = strrchr(_editor->current_file ? _editor->current_file : "", '/');
        filename = filename ? filename + 1: _editor->current_file;
        
        snprintf(status, sizeof(status), "[%s]%s (%.1f%%)[line %u/%u][column %u/%u] ",
                            _editor->current_file ? filename : "New File",
                            (_editor->dirty > 0) ? " (modified)" : "",
                            lines_percentage,
                            _editor->current_line, _editor->lines,
                            _editor->gbs[_editor->current_line].gap_start, gb_size(&_editor->gbs[_editor->current_line]) );
        
        DrawTextEx(_font, status, 
            (Vector2){0.0f, WINDOW_HEIGHT - FONT_SCALE}, FONT_SCALE, FONT_SPACING, WHITE);
    } else
    {
        const char *search_text_prefix = "Search for: %s";
        char *b = gb_get_buffer(&_searching_e->gbs[0]);
        size_t length = strlen(b) + strlen(search_text_prefix) + 1;
        
        char final_buffer[length];
        snprintf(final_buffer, length, search_text_prefix, b);
        final_buffer[length] = '\0';
        
        if (b != NULL && final_buffer[0] != '\0')
        {
            DrawTextEx(_font, final_buffer, (Vector2){0.0f, WINDOW_HEIGHT - FONT_SCALE}, FONT_SCALE, FONT_SPACING, WHITE);
            free(b);
        }
    }
    return;
}

static bool match_at(text_buffer *_line, const char *_query, size_t _query_length, size_t _pos)
{
    for (size_t y = 0; y < _query_length; y++)
    {
        if (gb_at(_line, _pos + y) != _query[y])
            return false;
    }
    return true;
}

bool search(editor *_editor, const char *_query, size_t _start_line, size_t _start_column, search_result *_result, bool _forward)
{
    size_t query_length = strlen(_query);
    if (query_length == 0)
        return false;
    size_t i = _start_line;
    while (true)
    {
        text_buffer *current_line = &_editor->gbs[i];
        if (current_line)
        {
            size_t line_length = gb_size(current_line);
            if (line_length >= query_length)
            {
                size_t max_start = line_length - query_length;
                if (_forward)
                {
                    size_t begin = (i == _start_line) ? _start_column : 0;
                    for (size_t j = begin; j <= max_start; j++)
                    {
                        if (match_at(current_line, _query, query_length, j))
                        {
                            _result->line = i;
                            _result->column = j;
                            return true;
                        }
                    }
                }
                else
                {
                    size_t start_j = (i == _start_line) ? (_start_column < max_start ? _start_column : max_start) : max_start;
                    size_t j = start_j;
                    while (true)
                    {
                        if (match_at(current_line, _query, query_length, j))
                        {
                            _result->line = i;
                            _result->column = j;
                            return true;
                        }
                        if (j == 0)
                            break;
                        j--;
                    }
                }
            }
        }
        if (_forward)
        {
            if (i + 1 >= _editor->lines)
                break;
            i++;
        }
        else
        {
            if (i == 0)
                break;
            i--;
        }
    }
    return false;
}

void save_file(const char *_filepath, editor *_editor);

int x = 0; // that's pretty stupid

// and that's even more stupid;
bool foward_search = true;
size_t tmp_search_gs = 0;
int tmp_search_cl = 0;

void input_processing(editor *_editor, editor *_searching_e)
{
    int key = GetCharPressed();
    if (current_mode == SEARCHING_MODE)
    {
        if (_searching_e == NULL)
        {
            printf("searching editor pointer is null.\n");
            return;
        }
        search_result result;
        while (key > 0)
        {
            if (_searching_e->gbs == NULL)
                return;
            int utf8_size = 0;
            const char *utf8 = CodepointToUTF8(key, &utf8_size);
            
            gb_insert_string(&_searching_e->gbs[_searching_e->current_line], utf8);
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE))
            gb_backspace(_searching_e);
        
        if (IsKeyDown(KEY_ESCAPE))
            goto EXIT_NO_POS;
        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
        {
            if (IsKeyPressed(KEY_P))
            {
                finished_searching = false;
                foward_search = false;
            }
            else if (IsKeyPressed(KEY_N))
            {
                finished_searching = false;
                foward_search = true;
            }
            
            if (IsKeyPressed(KEY_G))
            {   EXIT_NO_POS:
                current_mode = NORMAL_MODE;
                gb_clean_line(&_searching_e->gbs[0]);
                
                gb_move_to_index(&_editor->gbs[_editor->current_line], tmp_search_gs);
                _editor->current_line = tmp_search_cl;
                finished_searching = false;
                return;
            }
        }
        if (IsKeyPressed(KEY_ENTER))
        {
            current_mode = NORMAL_MODE;
            gb_clean_line(&_searching_e->gbs[0]);
            finished_searching = false;
            return;
        }
        if (finished_searching == true)
            return;
        
        char *buffer = gb_get_buffer(&_searching_e->gbs[_searching_e->current_line]);
        if (buffer == NULL)
            return;
        if (search(_editor, buffer, foward_search ? _editor->current_line : _editor->current_line - 1, 
                gb_size(&_editor->gbs[_editor->current_line]), &result, foward_search))
        {
            _editor->current_line = result.line;
            gb_move_to_index(&_editor->gbs[_editor->current_line], result.column);
            finished_searching = true;
        }
        if (buffer != NULL)
            free(buffer);
    }
    if (x > 0)
    {
        if (IsKeyPressed(KEY_S))
            save_file(_editor->current_file, _editor);
        if (IsKeyPressed(KEY_DELETE))
        {
            gb_clean_line(&_editor->gbs[_editor->current_line]);
            _editor->dirty++;
        }
        
        x++;
        if (x >= 25)
            x = 0;
    }
    
    // processing keys
    if (current_mode == NORMAL_MODE)
    {
        while (key > 0)
        {
            if (_editor->gbs == NULL)
                return;
            int utf8_size = 0;
            const char *utf8 = CodepointToUTF8(key, &utf8_size);
            
            gb_insert_string(&_editor->gbs[_editor->current_line], utf8);
            
            key = GetCharPressed();
            _editor->dirty++;
        }
    }
    
    if (current_mode == NORMAL_MODE)
    {
        if (IsKeyPressed(KEY_BACKSPACE))
        {
            gb_backspace(_editor);
            _editor->dirty++;
        }
        if (IsKeyPressed(KEY_ENTER))
        {
            gb_insert_line(_editor);
            _editor->dirty++;
        }
        if (IsKeyPressed(KEY_TAB))
        {
            for (int i = 0; i < TAB_SIZE; ++i)
                gb_insert(&_editor->gbs[_editor->current_line], ' ');
            _editor->dirty++;
        }
    }
    
// control keys
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
    {
        if (IsKeyPressed(KEY_BACKSPACE) && current_mode == NORMAL_MODE)
        {
            int i = _editor->gbs[_editor->current_line].gap_start;
            if (_editor->gbs[_editor->current_line].buffer[i] == ' ')
            {
                while (_editor->gbs[_editor->current_line].buffer[i - 1] == ' ' && i > 0)
                {
                    gb_backspace(_editor);
                    i--;
                }
            } else
            {
//                while (_editor->gbs[_editor->current_line].buffer[i - 1] != ' ' && i > 0)
                while (strchr("[]{}*.\"\\() ", _editor->gbs[_editor->current_line].buffer[i - 1]) == NULL && i > 0)
                {
                    gb_backspace(_editor);
                    i--;
                }
            }
            _editor->dirty++;
        }
        
        if (IsKeyPressed(KEY_S) && x == 0)
        {
            current_mode = SEARCHING_MODE;
            tmp_search_gs = _editor->gbs[_editor->current_line].gap_start;
            tmp_search_cl = _editor->current_line;
        }

        if (IsKeyPressed(KEY_X))
            x = 1;
        
        if (current_mode == NORMAL_MODE)
        {
            if (IsKeyPressed(KEY_V))
            {
                for (int i = 0; i < (gb_get_screen_y() / 2); ++i)
                    gb_move_down(_editor);
            }
        }
        
        if (IsKeyPressed(KEY_EQUAL) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))
        {
            FONT_SCALE += 2.0f;
            reload_font(_editor);
        }
        if (IsKeyPressed(KEY_MINUS))
        {
            FONT_SCALE -= 2.0f;
            reload_font(_editor);
        }
        
        if (IsKeyPressed(KEY_C) && current_mode == NORMAL_MODE)
        {
            /// I have a lot of fun writing bad code that works
            text_buffer *current_line = &_editor->gbs[_editor->current_line];
            if (current_line->gap_start == gb_size(current_line))
                return;
            char c = 0;
            if (current_line->gap_start != 0)
            {
                if ((strchr("[]{}*.\"\\() ", current_line->buffer[current_line->gap_start]) != NULL &&
                    !isupper(current_line->buffer[current_line->gap_start + 1])))
                {
                    c = toupper(current_line->buffer[current_line->gap_start + 1]);
                    gb_move_right(current_line);
                    goto CAPITALIZE;
                }
                if ((strchr("[]{}*.\"\\() ", current_line->buffer[current_line->gap_start - 1]) != NULL &&
                    !isupper(current_line->buffer[current_line->gap_start])))
                {
                    c = toupper(current_line->buffer[current_line->gap_start]);
CAPITALIZE:
                    gb_move_right(current_line);
                    gb_backspace(_editor);
                    gb_insert(&_editor->gbs[_editor->current_line], c);
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
            _editor->dirty++;
        }
        
        if (current_mode == NORMAL_MODE)
        {
            if (IsKeyPressed(KEY_M))
            {
                gb_insert_line(_editor);
                _editor->dirty++;
            }
            
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
        }
            
        if (IsKeyPressed(KEY_B) && current_mode == NORMAL_MODE)
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
        else if (IsKeyPressed(KEY_F) && current_mode == NORMAL_MODE)
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
    else if ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && current_mode == NORMAL_MODE)
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
                for (; strchr("[]{}*#.-\"'/\\(); ", current_line->buffer[index - 2]) == NULL && index > 0;)
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
                for (; strchr("[]{}*#.-\"'/\\(); ", current_line->buffer[index + 2]) == NULL && index < gb_size(current_line);)
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
    _editor->dirty = 0;
    fclose(file);
    printf("'%s' saved sucessfully.\n", _filepath);
    return;
}
void open_file(const char *_filepath, editor *_editor)
{
    if (_editor->current_file != NULL)
        free(_editor->current_file);
    _editor->current_file = strdup(_filepath);
    printf("opening '%s' file.\n", _filepath);
    
    gb_clean_all(_editor);
        
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
        gb_insert_string(&_editor->gbs[_editor->current_line], t);
        gb_insert_line(_editor);
    }
    gb_move_to_index(&_editor->gbs[0], gb_size(&_editor->gbs[0]));
    _editor->current_line = 0;
    
    fclose(file);
    free(tmp);
    return;
}

void open_directory(const char *_directory_path, editor *_editor)
{
    struct stat s;
    if (stat(_directory_path, &s) != 0)
    {
        fprintf(stderr, "failed to check for directory path.\n");
        return;
    }
    if (!S_ISDIR(s.st_mode))
    {
        if (S_ISREG(s.st_mode))
        {
            printf("given '%s' path is not a directory. Opening file instead.\n", _directory_path);
            open_file(_directory_path, _editor);
        } else
            fprintf(stderr, "invalid path.\n");
        return;
    }
    
    if (_editor->current_file != NULL)
        free(_editor->current_file);
    _editor->current_file = strdup(_directory_path);
    printf("opening '%s' folder.\n", _directory_path);
    
    gb_clean_all(_editor);
    FILE *file = fopen(_directory_path, "r");
    if (file == NULL)
    {
        fprintf(stderr, "'%s' file not found.\n", _directory_path);
        exit(EXIT_FAILURE);
        return;
    }
    return;
}

void dropped_file_processing(editor *_editor)
{
    if (IsFileDropped())
    {
        FilePathList dropped_files = LoadDroppedFiles();
        if (IsTextFile(dropped_files.paths[0]))
            open_file(dropped_files.paths[0], _editor);
        UnloadDroppedFiles(dropped_files);
    }
    return;
}

void main_loop(editor *_editor, editor *_searching_editor)
{
    while (!WindowShouldClose())
    {
        if (IsWindowResized())
            update_window_size();
        
        dropped_file_processing(_editor);
        input_processing(_editor, _searching_editor);
        center_screen_cursor(_editor);
        BeginDrawing();
            ClearBackground(WHITE);
            BeginMode2D(_editor->camera);
                text_rendering(_editor, _editor->editor_font);
                cursor_rendering(_editor, _editor->editor_font);
            EndMode2D();
            gui_rendering(_editor, _searching_editor, _editor->editor_font);
        EndDrawing();
//        printf("%d\n", _editor->dirty);
        if (_editor->dirty >= 350)
        {
            save_file(_editor->current_file, _editor);
            _editor->dirty = 0;
        }
    }
    return;
}

int main(int argc, char**argv)
{
    editor e, searching_e;
    gb_init(&searching_e, BUFFER_SIZE);
    gb_init(&e, BUFFER_SIZE);
    
    if (argc > 1)
        open_file(argv[1], &e);
    else
    {
        const char *base_text = "Hello, Clarice.";
        gb_insert_string(&e.gbs[e.current_line], base_text);
    }
    init_window();
    
    init_font(&e);
    
    main_loop(&e, &searching_e);
    
    gb_free(&searching_e);
    gb_free(&e);
    return 0;
}
