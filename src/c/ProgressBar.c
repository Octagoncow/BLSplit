#include <pebble.h>
#include "ProgressBar.h"

// ---------------------------------------------------------------------------
// Compile-time Platform Angles
// ---------------------------------------------------------------------------
#if defined(PBL_PLATFORM_EMERY)
    #define PROGRESS_BAR_TR_ANGLE 7511
#else
    #define PROGRESS_BAR_TR_ANGLE 7393
#endif

// ---------------------------------------------------------------------------
// Helper function: Power-of-two (1024) bitwise math for speed
// ---------------------------------------------------------------------------
static int32_t get_corner_snapped_angle(int steps, int goal) {
    if (goal <= 0) return 0;

    int32_t fraction = (steps << 10) / goal; 
    if (fraction > 1024) fraction = 1024;

    int32_t tr = PROGRESS_BAR_TR_ANGLE;
    int32_t tl = TRIG_MAX_ANGLE - tr;
    int32_t bl = (TRIG_MAX_ANGLE / 2) + tr;
    int32_t br = (TRIG_MAX_ANGLE / 2) - tr;

    int32_t start_angle = bl;
    int32_t point1 = tl;
    int32_t point2 = tr + TRIG_MAX_ANGLE;
    int32_t point3 = br + TRIG_MAX_ANGLE;
    int32_t point4 = bl + TRIG_MAX_ANGLE;

    // Piecewise interpolation forcing 25% (256/1024) chunks into the corners.
    if (fraction <= 256) {
        return start_angle + (((point1 - start_angle) * fraction) >> 8);
    } else if (fraction <= 512) {
        return point1 + (((point2 - point1) * (fraction - 256)) >> 8);
    } else if (fraction <= 768) {
        return point2 + (((point3 - point2) * (fraction - 512)) >> 8);
    } else {
        return point3 + (((point4 - point3) * (fraction - 768)) >> 8);
    }
}

// ---------------------------------------------------------------------------
// progress_bar_draw
// ---------------------------------------------------------------------------
void progress_bar_draw(GContext *ctx, GRect bounds, int steps, int goal,
                       GColor color, GColor color_2x, GColor color_3x,
                       GColor bg_color, int padding, int corner_radius)
{
    if (goal <= 0 || steps <= 0) return;

    // Determine current color based on tier
    GColor barColor = color;
    if (steps > goal * 2) {
        barColor = color_3x;
    } else if (steps > goal) {
        barColor = color_2x;    
    }
    
    int bw = padding / 2;
    int stepProgress = steps % goal;
    bool isFullTier = (steps > 0 && stepProgress == 0);

    // 1. Draw the full track
    graphics_context_set_fill_color(ctx, barColor);
    graphics_fill_rect(ctx, bounds, corner_radius, GCornersAll);

    // 2. Hollow out the center with bg_color to reveal nothing underneath
    graphics_context_set_fill_color(ctx, bg_color);
    GRect inner_bounds = grect_inset(bounds, GEdgeInsets(bw));
    int inner_radius = corner_radius - bw;
    if (inner_radius < 0) inner_radius = 0;
    graphics_fill_rect(ctx, inner_bounds, inner_radius, GCornersAll);

    // 3. Mask out the unreached portion using bg_color (the "eraser")
    if (!isFullTier && steps < goal * 3) 
    {
        graphics_context_set_fill_color(ctx, bg_color);

        int32_t start_angle = get_corner_snapped_angle(stepProgress, goal);
        int32_t offset = (TRIG_MAX_ANGLE / 2) + PROGRESS_BAR_TR_ANGLE; 
        int32_t end_angle = offset + TRIG_MAX_ANGLE;

        int cx = bounds.origin.x + (bounds.size.w >> 1);
        int cy = bounds.origin.y + (bounds.size.h >> 1);
        
        int radius = bounds.size.w > bounds.size.h ? bounds.size.w : bounds.size.h;
        GRect sweep_bounds = GRect(cx - radius, cy - radius, radius << 1, radius << 1);

        graphics_fill_radial(ctx, sweep_bounds, GOvalScaleModeFitCircle, radius, start_angle, end_angle);
    }
}