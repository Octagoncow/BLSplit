#include <pebble.h>

//#define DEBUG //Comment out for release build

/*
Helpful commands:
"export PEBBLE_PHONE=XXX.XXX.X.XXX" (e.g. export PEBBLE_PHONE=192.168.0.237) to set default pebble install destination
"pebble build" to build the watch face or app
"pebble install" to install the new watch face or app
"pebble logs" to view debug output
"pebble install --emulator basalt" to install and use in the emulator
*/

// Left and right padding on the screen sides
#define EDGE_PADDING 20

#define SETTINGS_KEY 1

typedef struct ClaySettings 
{
	GColor BackgroundColor;
	GColor HourColor;
	GColor MinuteColor;
	GColor DateColor;
	GColor BatteryColor;
	GColor StepsColor;
	bool ShowDate;
	bool ShowBattery;
	bool ShowStepProgress;
	int StepGoal;
	bool MetricSteps;
} ClaySettings;

static ClaySettings settings;

static Window *s_main_window;
static Layer *s_window_layer;

static GFont s_battery_font;
static GFont s_date_font;
static GFont s_time_font;

static BitmapLayer *s_blade_layer;
static GBitmap *s_blade_bitmap;

static TextLayer *s_battery_layer;
static int s_battery_level;

static TextLayer *s_date_layer;
static TextLayer *s_hours_layer;
static TextLayer *s_minutes_layer;

// Step progress border layer and current step count
static Layer *s_progress_layer;
static int s_step_count = 0;

static void prv_default_settings() 
{
	settings.BackgroundColor = GColorBlack;
	settings.HourColor = GColorWhite;
	settings.MinuteColor = GColorVeryLightBlue;
	settings.DateColor = GColorWhite;
	settings.BatteryColor = GColorVeryLightBlue;
	settings.StepsColor = GColorOrange;
	settings.ShowDate = true;
	settings.ShowBattery = true;
	settings.ShowStepProgress = true;
	settings.StepGoal = 8000;
	settings.MetricSteps = false;
}

static void prv_save_settings() 
{
	persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_load_settings() 
{
	prv_default_settings();
	if (persist_exists(SETTINGS_KEY)) 
	{
		persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
	}
}

static unsigned short get_display_hour(unsigned short hour) 
{
	if (clock_is_24h_style()) 
	{
		return hour;
	}

	// Converts "0" to "12"
	unsigned short display_hour = hour % 12;
	return display_hour ? display_hour : 12;
}

static void display_time(struct tm *tick_time) 
{
	//buffer size should only need to be 3 (HH\0 or MM\0)
	static char s_hours_buf[3];
	static char s_mins_buf[3];

	snprintf(s_hours_buf, sizeof(s_hours_buf), "%02d", get_display_hour(tick_time->tm_hour));
	snprintf(s_mins_buf, sizeof(s_mins_buf), "%02d", tick_time->tm_min);

	text_layer_set_text(s_hours_layer, s_hours_buf);
	text_layer_set_text(s_minutes_layer, s_mins_buf);
}

static void display_date(struct tm *tick_time) 
{
#ifdef DEBUG
	APP_LOG(APP_LOG_LEVEL_DEBUG, "display_date start");
#endif
	// Write the current month string and day into a buffer (XXX\nXX\0)
	static char s_buffer[7];
	strftime(s_buffer, sizeof(s_buffer), "%a\n%d", tick_time);

#ifdef DEBUG
	APP_LOG(APP_LOG_LEVEL_DEBUG, "display_date: %s", s_buffer);
#endif
	// Display this date on the TextLayer
	text_layer_set_text(s_date_layer, s_buffer);

#ifdef DEBUG
	APP_LOG(APP_LOG_LEVEL_DEBUG, "display_date end");
#endif
}

// Function to update the battery percentage display
static void update_battery_display()
{
	// "100%\0" is the max value
	static char battery_text[5];
	snprintf(battery_text, sizeof(battery_text), "%d%%", s_battery_level);
	text_layer_set_text(s_battery_layer, battery_text);
}

// Battery state change callback
static void battery_callback(BatteryChargeState state)
{
	s_battery_level = state.charge_percent;
	update_battery_display();
}

// Fetch today's step total and redraw the progress border
static void update_step_count(void) 
{
	HealthMetric metric = HealthMetricStepCount;
	time_t start = time_start_of_today();
	time_t end = time(NULL);

	HealthServiceAccessibilityMask mask =
		health_service_metric_accessible(metric, start, end);

	if (mask & HealthServiceAccessibilityMaskAvailable) 
	{
#ifdef DEBUG
		s_step_count = 1000;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Step count: %d", s_step_count);
#else
		s_step_count = (int)health_service_sum(metric, start, end);
#endif

	}
	else 
	{
#ifdef DEBUG
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Health data not available");
#endif
	}

	if (s_progress_layer) 
	{
		layer_mark_dirty(s_progress_layer);
	}
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	display_time(tick_time);
	display_date(tick_time);
	update_step_count();
}

// ─── Step-progress border ────────────────────────────────────────────────────

// Draw the orange perimeter border proportional to today's step count.
// The border width equals EDGE_PADDING
// Progress order (each edge = STEP_GOAL/4 steps):
//   1. Left  edge — bottom → top
//   2. Top   edge — left  → right
//   3. Right edge — top   → bottom
//   4. Bottom edge — right → left
static void progress_layer_update_proc(Layer *layer, GContext *ctx) 
{
	GRect bounds = layer_get_bounds(layer);
	int bw = EDGE_PADDING / 4;                  // border width
	int goal = settings.StepGoal;
	int steps = s_step_count > goal ? goal : s_step_count;   // cap at goal
	int sps = goal / 4;                           // steps per segment

	graphics_context_set_fill_color(ctx, settings.StepsColor);


	// ── Segment 1: Left edge, bottom → top ──────────────────────────────────
	if (steps > 0) 
	{
		int seg = steps < sps ? steps : sps;
		int h = (seg * bounds.size.h) / sps;
		graphics_fill_rect(ctx,
			GRect(0, bounds.size.h - h, bw, h),
			0, GCornerNone);
	}

	// ── Segment 2: Top edge, left → right ───────────────────────────────────
	if (steps > sps) 
	{
		int seg = steps - sps;
		if (seg > sps) seg = sps;
		int w = (seg * bounds.size.w) / sps;
		graphics_fill_rect(ctx,
			GRect(0, 0, w, bw),
			0, GCornerNone);
	}

	// ── Segment 3: Right edge, top → bottom ─────────────────────────────────
	if (steps > sps * 2) 
	{
		int seg = steps - sps * 2;
		if (seg > sps) seg = sps;
		int h = (seg * bounds.size.h) / sps;
		graphics_fill_rect(ctx,
			GRect(bounds.size.w - bw, 0, bw, h),
			0, GCornerNone);
	}

	// ── Segment 4: Bottom edge, right → left ────────────────────────────────
	if (steps > sps * 3) 
	{
		int seg = steps - sps * 3;
		if (seg > sps) seg = sps;
		int w = (seg * bounds.size.w) / sps;
		graphics_fill_rect(ctx,
			GRect(bounds.size.w - w, bounds.size.h - bw, w, bw),
			0, GCornerNone);
	}
}

// ─────────────────────────────────────────────────────────────────────────────

static void window_load(Window *window)
{
	// Get information about the Window
	s_window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(s_window_layer);

	// ── Step-progress border — added FIRST so it sits beneath all other layers ──
	s_progress_layer = layer_create(bounds);
	layer_set_update_proc(s_progress_layer, progress_layer_update_proc);
	layer_add_child(s_window_layer, s_progress_layer);

	int hours_y_offset = 0;
	int minutes_y_offset = 0;

	// Pick time font size and vertical offsets based on platform
#if defined(PBL_PLATFORM_EMERY)
	// Load battery and date fonts
	s_battery_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAIRA_STENCIL_28));
	s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAIRA_STENCIL_28));

	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAIRA_STENCIL_99));
	// Emery has a taller screen, so we need larger offsets
	hours_y_offset = -11;
	minutes_y_offset = -5;

	// Battery label: centered vertically at the split, right-aligned
	s_battery_layer = text_layer_create(GRect((bounds.size.w / 2), bounds.size.h / 2 - 9, (bounds.size.w / 2) - EDGE_PADDING, 40));
	text_layer_set_background_color(s_battery_layer, GColorClear);
	text_layer_set_text_color(s_battery_layer, settings.BatteryColor);
	text_layer_set_font(s_battery_layer, s_battery_font);
	text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
	layer_add_child(s_window_layer, text_layer_get_layer(s_battery_layer));

	// Date label: centered vertically at the split, left side
	s_date_layer = text_layer_create(GRect(EDGE_PADDING, bounds.size.h / 2 - 19, 70, 80));
	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_text_color(s_date_layer, settings.DateColor);
	text_layer_set_font(s_date_layer, s_date_font);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
	layer_add_child(s_window_layer, text_layer_get_layer(s_date_layer));
#else
	// Load battery and date fonts
	s_battery_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAIRA_STENCIL_16));
	s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAIRA_STENCIL_16));

	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAIRA_STENCIL_69));
	// Basalt/Aplite offsets. Tweak these numbers to get the exact spacing you want!
	hours_y_offset = 0; // Negative to shift the layer UP
	minutes_y_offset = 0; // Positive to shift the layer DOWN

		// Battery label: centered vertically at the split, right-aligned
	s_battery_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 9, bounds.size.w - EDGE_PADDING, 20));
	text_layer_set_background_color(s_battery_layer, GColorClear);
	text_layer_set_text_color(s_battery_layer, settings.BatteryColor);
	text_layer_set_font(s_battery_layer, s_battery_font);
	text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
	layer_add_child(s_window_layer, text_layer_get_layer(s_battery_layer));

	// Date label: centered vertically at the split, left side
	s_date_layer = text_layer_create(GRect(EDGE_PADDING, bounds.size.h / 2 - 19, 70, 40));
	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_text_color(s_date_layer, settings.DateColor);
	text_layer_set_font(s_date_layer, s_date_font);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
	layer_add_child(s_window_layer, text_layer_get_layer(s_date_layer));
#endif

	// Hours layer: top half of screen, white text, left-aligned
	// HOURS_LEFT_PADDING offsets the layer (and therefore the text) from the left screen edge
	// Shift the Y origin by the offset, and keep height sufficient for the font
	s_hours_layer = text_layer_create(GRect(
		EDGE_PADDING,
		hours_y_offset,                               // Use offset here to move it UP
		bounds.size.w - EDGE_PADDING,
		(bounds.size.h / 2) + abs(hours_y_offset)     // Increase height so bottom isn't cut
	));
	text_layer_set_background_color(s_hours_layer, GColorClear);
	text_layer_set_text_color(s_hours_layer, settings.HourColor);
	text_layer_set_font(s_hours_layer, s_time_font);
	text_layer_set_text_alignment(s_hours_layer, GTextAlignmentLeft);
	layer_add_child(s_window_layer, text_layer_get_layer(s_hours_layer));

	// Calculate the width of the layer to be the full width minus twice the padding
	// Then shift the X-origin by the padding amount.
	int layer_width = bounds.size.w - (EDGE_PADDING * 2);

	s_minutes_layer = text_layer_create(GRect(
		EDGE_PADDING,
		(bounds.size.h / 2) + minutes_y_offset,       // Use offset here to move it DOWN
		layer_width,
		bounds.size.h / 2                             // Keep standard height
	));

	// Disable clipping so the right-leaning italic overhang can spill safely into the padding area
	layer_set_clips(text_layer_get_layer(s_minutes_layer), false);

	text_layer_set_background_color(s_minutes_layer, GColorClear);
	text_layer_set_text_color(s_minutes_layer, settings.MinuteColor);
	text_layer_set_font(s_minutes_layer, s_time_font);
	text_layer_set_text_alignment(s_minutes_layer, GTextAlignmentRight);
	layer_add_child(s_window_layer, text_layer_get_layer(s_minutes_layer));

	// Blade overlay — added last so it renders on top of the time digits.
	// Use bitmap_layer_set_compositing_mode instead of a custom update proc
	// so that GCompOpSet (transparency) is handled by BitmapLayer natively.
	s_blade_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
	s_blade_layer = bitmap_layer_create(bounds);
	bitmap_layer_set_bitmap(s_blade_layer, s_blade_bitmap);
	bitmap_layer_set_compositing_mode(s_blade_layer, GCompOpSet);
	layer_add_child(s_window_layer, bitmap_layer_get_layer(s_blade_layer));

	time_t now = time(NULL);
	struct tm *tick_time = localtime(&now);
#ifdef DEBUG
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initial time: %02d:%02d", tick_time->tm_hour, tick_time->tm_min);
#endif
	display_time(tick_time);
	display_date(tick_time);
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

	// Fetch the initial step count so the border is drawn correctly on launch
	update_step_count();
}

static void window_unload(Window *window)
{
	// Unload custom fonts
	fonts_unload_custom_font(s_battery_font);
	fonts_unload_custom_font(s_date_font);
	fonts_unload_custom_font(s_time_font);

	// Destroy time text layers
	text_layer_destroy(s_hours_layer);
	text_layer_destroy(s_minutes_layer);

	// Destroy battery and date layers
	text_layer_destroy(s_battery_layer);
	text_layer_destroy(s_date_layer);

	// Destroy blade bitmap and layer
	gbitmap_destroy(s_blade_bitmap);
	bitmap_layer_destroy(s_blade_layer);

	// Destroy step-progress border layer
	layer_destroy(s_progress_layer);
}

// Helper function to update the UI immediately when settings change
static void prv_update_display() 
{
	// Update Background
	window_set_background_color(s_main_window, settings.BackgroundColor);

	// Toggle Visibility
	layer_set_hidden(text_layer_get_layer(s_date_layer), !settings.ShowDate);
	layer_set_hidden(text_layer_get_layer(s_battery_layer), !settings.ShowBattery);
	layer_set_hidden(s_progress_layer, !settings.ShowStepProgress);

	// Update Text Colors
	text_layer_set_text_color(s_hours_layer, settings.HourColor);
	text_layer_set_text_color(s_minutes_layer, settings.MinuteColor);
	text_layer_set_text_color(s_date_layer, settings.DateColor);
	text_layer_set_text_color(s_battery_layer, settings.BatteryColor);

	// Force a redraw of the progress layer so it uses the new goal/color
	if (s_progress_layer && settings.ShowStepProgress) 
	{
		layer_mark_dirty(s_progress_layer);
	}
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) 
{
	// Read Colors
	Tuple *bg_color_t = dict_find(iterator, MESSAGE_KEY_BackgroundColor);
	if (bg_color_t) settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);

	Tuple *hour_color_t = dict_find(iterator, MESSAGE_KEY_HourColor);
	if (hour_color_t) settings.HourColor = GColorFromHEX(hour_color_t->value->int32);

	Tuple *minute_color_t = dict_find(iterator, MESSAGE_KEY_MinuteColor);
	if (minute_color_t) settings.MinuteColor = GColorFromHEX(minute_color_t->value->int32);

	Tuple *date_color_t = dict_find(iterator, MESSAGE_KEY_DateColor);
	if (date_color_t) settings.DateColor = GColorFromHEX(date_color_t->value->int32);

	Tuple *battery_color_t = dict_find(iterator, MESSAGE_KEY_BatteryColor);
	if (battery_color_t) settings.BatteryColor = GColorFromHEX(battery_color_t->value->int32);

	Tuple *steps_color_t = dict_find(iterator, MESSAGE_KEY_StepsColor);
	if (steps_color_t) settings.StepsColor = GColorFromHEX(steps_color_t->value->int32);

	// Read Booleans (Toggles)
	Tuple *show_date_t = dict_find(iterator, MESSAGE_KEY_ShowDate);
	if (show_date_t) settings.ShowDate = show_date_t->value->int32 == 1;

	Tuple *show_battery_t = dict_find(iterator, MESSAGE_KEY_ShowBattery);
	if (show_battery_t) settings.ShowBattery = show_battery_t->value->int32 == 1;

	Tuple *show_steps_t = dict_find(iterator, MESSAGE_KEY_ShowStepProgress);
	if (show_steps_t) settings.ShowStepProgress = show_steps_t->value->int32 == 1;

	Tuple *metric_steps_t = dict_find(iterator, MESSAGE_KEY_MetricSteps);
	if (metric_steps_t) settings.MetricSteps = metric_steps_t->value->int32 == 1;

	// Read Number Input (Step Goal)
	Tuple *step_goal_t = dict_find(iterator, MESSAGE_KEY_StepGoal);
	if (step_goal_t) 
	{
		// Clay inputs often transmit as strings even if typed as a number in config.js
		if (step_goal_t->type == TUPLE_CSTRING) 
		{
			settings.StepGoal = atoi(step_goal_t->value->cstring);
		}
		else 
		{
			settings.StepGoal = step_goal_t->value->int32;
		}
	}

	// Save the new settings to persistent storage
	prv_save_settings();

	// Apply the changes to the screen right now
	prv_update_display();
}

static void prv_init(void)
{
	prv_load_settings();

	// Register the AppMessage handler
	app_message_register_inbox_received(inbox_received_callback);

	// Open AppMessage with what the tutorial says to set it to
	app_message_open(256, 256);

	s_main_window = window_create();

	window_set_background_color(s_main_window, settings.BackgroundColor);
	window_set_window_handlers(s_main_window, (WindowHandlers) 
	{
		.load = window_load,
			.unload = window_unload,
	});
	const bool animated = true;
	window_stack_push(s_main_window, animated);

	// Subscribe to battery state updates
	battery_state_service_subscribe(battery_callback);

	// Get the initial battery state
	battery_callback(battery_state_service_peek());
}

static void prv_deinit(void)
{
	window_destroy(s_main_window);
}

int main(void)
{
	prv_init();
#ifdef DEBUG
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_main_window);
#endif

	app_event_loop();
	prv_deinit();
}
