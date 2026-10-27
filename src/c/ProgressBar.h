#pragma once
#include <pebble.h>

void progress_bar_draw(GContext *ctx, GRect bounds, int steps, int goal,
                       GColor color, GColor color_2x, GColor color_3x,
                       GColor bg_color, int padding, int corner_radius);