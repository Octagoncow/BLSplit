#pragma once
#include <pebble.h>

void digit_renderer_init();
void digit_renderer_deinit();

// Returns the height of the current time rendering method
int digit_renderer_get_digit_height();
// Returns the width of the current time rendering method
int digit_renderer_get_digit_width();

void digit_renderer_draw_hours(GContext *ctx, GRect bounds, int value, GColor fallback_color, GFont fallback_font);
void digit_renderer_draw_minutes(GContext *ctx, GRect bounds, int value, GColor fallback_color, GFont fallback_font);