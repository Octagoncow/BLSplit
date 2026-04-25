#include "ProgressBar.h"

void progress_bar_draw(GContext *ctx, GRect bounds, int steps, int goal, GColor color, GColor bg_color, int padding, int corner_radius) {
    if (goal <= 0) return;

    int bw = padding / 2;
    int capped_steps = steps > goal ? goal : steps;
    if (capped_steps < 0) capped_steps = 0;

    // 1. Draw the full, contiguous rounded rectangle track
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_rect(ctx, bounds, corner_radius, GCornersAll);

    // 2. Hollow out the center to create the "bar" frame
    graphics_context_set_fill_color(ctx, bg_color);
    GRect inner_bounds = grect_inset(bounds, GEdgeInsets(bw));
    
    // Prevent negative inner radius
    int inner_radius = corner_radius - bw;
    if (inner_radius < 0) inner_radius = 0;
    
    graphics_fill_rect(ctx, inner_bounds, inner_radius, GCornersAll);

    // 3. Mask out the unreached portion using a sweeping radial fill
    if (capped_steps < goal) {
        graphics_context_set_fill_color(ctx, bg_color);

        // Starting angle offset (225 degrees is roughly the bottom-left corner)
        // You can change this to 0 if you want it to start at Top-Center
        int32_t offset = DEG_TO_TRIGANGLE(225);

        // Calculate the angle for the completed progress
        int32_t progress_angle = (capped_steps * TRIG_MAX_ANGLE) / goal;

        // The unreached portion to erase is from the current progress to the end
        int32_t start_angle = offset + progress_angle;
        int32_t end_angle = offset + TRIG_MAX_ANGLE;

        // Create a bounding box large enough to cover the screen corners
        int cx = bounds.origin.x + bounds.size.w / 2;
        int cy = bounds.origin.y + bounds.size.h / 2;
        int radius = bounds.size.w > bounds.size.h ? bounds.size.w : bounds.size.h;
        GRect sweep_bounds = GRect(cx - radius, cy - radius, radius * 2, radius * 2);

        // Draw the radial mask to "erase" the unreached track
        graphics_fill_radial(ctx, sweep_bounds, GOvalScaleModeFitCircle, radius, start_angle, end_angle);
    }
}