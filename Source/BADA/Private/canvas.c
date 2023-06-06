#include <stdlib.h>

#include "canvas.h"

#define MAX_STACK_SIZE (256)

static uint32_t* s_canvas;
static size_t s_rows;
static size_t s_cols;

static history_t s_undo_stack[MAX_STACK_SIZE];
static size_t s_num_undo_stack;

static history_t s_redo_stack[MAX_STACK_SIZE];
static size_t s_num_redo_stack;

static uint32_t get_pixel(size_t x, size_t y);

static void malloc_history(history_t* history, size_t num_pixel);
static void free_history(history_t* history);

static void clear_undo_stack(void);
static void clear_redo_stack(void);

void set_canvas(uint32_t* canvas, size_t rows, size_t cols)
{
    s_canvas = canvas;
    s_rows = rows;
    s_cols = cols;
}

void release_canvas(void)
{
    clear_undo_stack();
    clear_redo_stack();

    s_canvas = NULL;
    s_rows = 0;
    s_cols = 0;
}

void draw_pixel(size_t x, size_t y, uint32_t rgb_color)
{
    if (s_canvas == NULL) {
        return;
    }

    clear_redo_stack();

    history_t* history = &s_undo_stack[s_num_undo_stack++];
    malloc_history(history, 1);

    history->x[0] = x;
    history->y[0] = y;
    history->prev_pixel[0] = get_pixel(x, y);

    s_canvas[y * s_cols + x] = rgb_color;
}

void remove_pixel(size_t x, size_t y)
{
    if (s_canvas == NULL) {
        return;
    }

    clear_redo_stack();

    history_t* history = &s_undo_stack[s_num_undo_stack++];
    malloc_history(history, 1);

    history->x[0] = x;
    history->y[0] = y;
    history->prev_pixel[0] = get_pixel(x, y);

    s_canvas[y * s_cols + x] = 0xFFFFFF;
}

void fill_canvas(uint32_t rgb_color)
{
    if (s_canvas == NULL) {
        return;
    }

    clear_redo_stack();

    history_t* history = &s_undo_stack[s_num_undo_stack++];
    malloc_history(history, s_rows * s_cols);

    size_t i = 0;
    for (size_t y = 0; y < s_rows; ++y) {
        for (size_t x = 0; x < s_cols; ++x) {
            history->x[i] = x;
            history->y[i] = y;
            history->prev_pixel[i] = get_pixel(x, y);
            ++i;

            s_canvas[y * s_cols + x] = rgb_color;
        }
    }
}

void draw_horizontal_line(size_t y, uint32_t rgb_color)
{
    if (s_canvas == NULL) {
        return;
    }

    clear_redo_stack();

    history_t* history = &s_undo_stack[s_num_undo_stack++];
    malloc_history(history, s_cols);

    for (size_t x = 0; x < s_cols; ++x) {
        history->x[x] = x;
        history->y[x] = y;
        history->prev_pixel[x] = get_pixel(x, y);

        s_canvas[y * s_cols + x] = rgb_color;
    }
}

void draw_vertical_line(size_t x, uint32_t rgb_color)
{
    if (s_canvas == NULL) {
        return;
    }

    clear_redo_stack();

    history_t* history = &s_undo_stack[s_num_undo_stack++];
    malloc_history(history, s_rows);

    for (size_t y = 0; y < s_rows; ++y) {
        history->x[y] = x;
        history->y[y] = y;
        history->prev_pixel[y] = get_pixel(x, y);

        s_canvas[y * s_cols + x] = rgb_color;
    }
}

void draw_rectangle(size_t start_x, size_t start_y, size_t end_x, size_t end_y, uint32_t rgb_color)
{
    if (s_canvas == NULL) {
        return;
    }

    clear_redo_stack();

    history_t* history = &s_undo_stack[s_num_undo_stack++];
    malloc_history(history, (end_x - start_x + 1) * (end_y - start_y + 1));

    size_t i = 0;
    for (size_t y = start_y; y <= end_y; ++y) {
        for (size_t x = start_x; x <= end_x; ++x) {
            history->x[i] = x;
            history->y[i] = y;
            history->prev_pixel[i] = get_pixel(x, y);
            ++i;

            s_canvas[y * s_cols + x] = rgb_color;
        }
    }
}

int undo(void)
{
    if (s_canvas == NULL) {
        return FALSE;
    }

    if (s_num_undo_stack == 0) {
        return FALSE;
    }

    history_t* undo_history = &s_undo_stack[s_num_undo_stack - 1];

    history_t* redo_history = &s_redo_stack[s_num_redo_stack];
    malloc_history(redo_history, undo_history->num_pixel);

    for (size_t i = 0; i < undo_history->num_pixel; ++i) {
        size_t x = undo_history->x[i];
        size_t y = undo_history->y[i];
        uint32_t pixel = undo_history->prev_pixel[i];

        redo_history->x[i] = x;
        redo_history->y[i] = y;
        redo_history->prev_pixel[i] = get_pixel(x, y);

        s_canvas[y * s_cols + x] = pixel;
    }

    free_history(undo_history);

    --s_num_undo_stack;
    ++s_num_redo_stack;

    return TRUE;
}

int redo(void)
{
    if (s_canvas == NULL) {
        return FALSE;
    }

    if (s_num_redo_stack == 0) {
        return FALSE;
    }

    history_t* redo_history = &s_redo_stack[s_num_redo_stack - 1];

    history_t* undo_history = &s_undo_stack[s_num_undo_stack];
    malloc_history(undo_history, redo_history->num_pixel);

    for (size_t i = 0; i < redo_history->num_pixel; ++i) {
        size_t x = redo_history->x[i];
        size_t y = redo_history->y[i];
        uint32_t pixel = redo_history->prev_pixel[i];

        undo_history->x[i] = x;
        undo_history->y[i] = y;
        undo_history->prev_pixel[i] = get_pixel(x, y);

        s_canvas[y * s_cols + x] = pixel;
    }

    free_history(redo_history);

    --s_num_redo_stack;
    ++s_num_undo_stack;

    return 1;
}

static uint32_t get_pixel(size_t x, size_t y)
{
    if (s_canvas == NULL) {
        return 0;
    }

    return s_canvas[y * s_cols + x];
}

static void malloc_history(history_t* history, size_t num_pixel)
{
    history->x = (size_t*)malloc(sizeof(size_t) * num_pixel);
    history->y = (size_t*)malloc(sizeof(size_t) * num_pixel);
    history->prev_pixel = (uint32_t*)malloc(sizeof(uint32_t) * num_pixel);
    history->num_pixel = num_pixel;
}

static void free_history(history_t* history)
{
    free(history->x);
    history->x = NULL;

    free(history->y);
    history->y = NULL;

    free(history->prev_pixel);
    history->prev_pixel = NULL;

    history->num_pixel = 0;
}

static void clear_undo_stack(void)
{
    for (size_t i = 0; i < s_num_undo_stack; ++i) {
        history_t* history = &s_undo_stack[i];
        free_history(history);
    }

    s_num_undo_stack = 0;
}

static void clear_redo_stack(void)
{
    for (size_t i = 0; i < s_num_redo_stack; ++i) {
        history_t* history = &s_redo_stack[i];
        free_history(history);
    }

    s_num_redo_stack = 0;
}