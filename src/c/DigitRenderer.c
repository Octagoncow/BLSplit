#include "DigitRenderer.h"
static GBitmap *s_spritesheet = NULL;
static int s_digit_w;
static int s_digit_h;
#define SPRITE_BLUE_OFFSET 10

void digit_renderer_init() 
{
    s_spritesheet = gbitmap_create_with_resource(RESOURCE_ID_SPRITE_SHEET_SAIRA_STENCIL);
    if (s_spritesheet) 
	{
        GRect bounds = gbitmap_get_bounds(s_spritesheet);
        s_digit_w = bounds.size.w / 20;
        s_digit_h = bounds.size.h;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Spritesheet loaded: %dx%d", s_digit_w, s_digit_h);
    } 
	else 
	{
        APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to load spritesheet resource!");
    }
}

void digit_renderer_deinit() 
{
    if (s_spritesheet)
	{
		gbitmap_destroy(s_spritesheet);
	}
}

int digit_renderer_get_digit_height() 
{
    return s_digit_h;
}

int digit_renderer_get_digit_width() 
{
    return s_digit_w;
}

void digit_renderer_draw_hours(GContext *ctx, GRect bounds, int value) 
{
    if (!s_spritesheet) 
	{
		return;
	}
    graphics_context_set_compositing_mode(ctx, GCompOpSet); // Fixes transparency

    int tens = value / 10;
    int ones = value % 10;
    int draw_w = bounds.size.w / 2;

    GBitmap *sub = gbitmap_create_as_sub_bitmap(s_spritesheet, GRect(tens * s_digit_w, 0, s_digit_w, s_digit_h));
    graphics_draw_bitmap_in_rect(ctx, sub, GRect(0, 0, draw_w, s_digit_h));
    gbitmap_destroy(sub);

    sub = gbitmap_create_as_sub_bitmap(s_spritesheet, GRect(ones * s_digit_w, 0, s_digit_w, s_digit_h));
    graphics_draw_bitmap_in_rect(ctx, sub, GRect(draw_w, 0, draw_w, s_digit_h));
    gbitmap_destroy(sub);
}

void digit_renderer_draw_minutes(GContext *ctx, GRect bounds, int value) 
{
    if (!s_spritesheet) 
	{
		return;
	}
    graphics_context_set_compositing_mode(ctx, GCompOpSet); // Fixes transparency

    int tens = value / 10;
    int ones = value % 10;
    int draw_w = bounds.size.w / 2;

    GBitmap *sub = gbitmap_create_as_sub_bitmap(s_spritesheet, GRect((SPRITE_BLUE_OFFSET + tens) * s_digit_w, 0, s_digit_w, s_digit_h));
    graphics_draw_bitmap_in_rect(ctx, sub, GRect(0, 0, draw_w, s_digit_h));
    gbitmap_destroy(sub);

    sub = gbitmap_create_as_sub_bitmap(s_spritesheet, GRect((SPRITE_BLUE_OFFSET + ones) * s_digit_w, 0, s_digit_w, s_digit_h));
    graphics_draw_bitmap_in_rect(ctx, sub, GRect(draw_w, 0, draw_w, s_digit_h));
    gbitmap_destroy(sub);
}