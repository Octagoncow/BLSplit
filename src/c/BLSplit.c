#include <pebble.h>
#include "Settings.h"
#include "DigitRenderer.h"
#include "ProgressBar.h"

//comment this out for release builds
//#define DEBUG

#define EDGE_PADDING 10

#define UNINITIALIZED -1

// Vertical gap between the hours layer and the date label beneath it,
// and between the battery label and the minutes layer above it.
#if defined(PBL_PLATFORM_EMERY)
    #define DATE_OFFSET    8
    #define BATTERY_OFFSET 8
#else
    #define DATE_OFFSET    0
    #define BATTERY_OFFSET -10
#endif

static ClaySettings settings;
static Window *s_main_window;
static Layer *s_window_layer, *s_hours_layer, *s_minutes_layer, *s_progress_layer;
static TextLayer *s_battery_layer, *s_date_layer;
static GFont s_battery_font, s_date_font;
static BitmapLayer *s_blade_layer;
static bool btPreviouslyConnected = true;
// Vibe pattern: ON for 200ms, OFF for 100ms, ON for 400ms:
static const uint32_t stepGoalAchievedVibePattern[] = { 300, 100, 300, 100, 300, 600, 300, 100, 300, 100, 300};

//'normal' blade image
static GBitmap *s_blade_bitmap;
//no Bluetooth blade image
static GBitmap *s_blade_no_bt_bitmap;

static int s_hours_val = 0, s_minutes_val = 0, s_day_val = 0, s_battery_lvl = 0, s_step_count = UNINITIALIZED;

static void hours_update_proc(Layer *layer, GContext *ctx) 
{
    digit_renderer_draw_hours(ctx, layer_get_bounds(layer), s_hours_val);
}

static void minutes_update_proc(Layer *layer, GContext *ctx) 
{
    digit_renderer_draw_minutes(ctx, layer_get_bounds(layer), s_minutes_val);
}

static void progress_update_proc(Layer *layer, GContext *ctx) 
{
    progress_bar_draw(ctx, layer_get_bounds(layer), s_step_count, settings.StepGoal, settings.StepsColor, settings.BackgroundColor, (int)EDGE_PADDING, 10);
}

static void bluetooth_callback(bool connected) 
{
    if (connected == btPreviouslyConnected || !settings.ShowBTConnection) return; // no change
	
    btPreviouslyConnected = connected;
	//set the blade image to be yellow if connected or gray if disconnected
    bitmap_layer_set_bitmap(s_blade_layer, connected ? s_blade_bitmap : s_blade_no_bt_bitmap);
}

static bool IsScreenObstructed()
{
    GRect full_bounds = layer_get_bounds(s_window_layer);
    GRect unobstructed_bounds = layer_get_unobstructed_bounds(s_window_layer);
    return !grect_equal(&full_bounds, &unobstructed_bounds);
}

static void SetDateAndBatteryVisiblity()
{
    bool obstructed = IsScreenObstructed();
    layer_set_hidden(text_layer_get_layer(s_date_layer),    !settings.ShowDate    || obstructed);
    layer_set_hidden(text_layer_get_layer(s_battery_layer), !settings.ShowBattery || obstructed);
}

static void UpdateMinutesPosition()
{
    GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
    int digit_h = digit_renderer_get_digit_height();
    GRect minutes_frame = layer_get_frame(s_minutes_layer);
    minutes_frame.origin.y = bounds.size.h - EDGE_PADDING - digit_h;
    layer_set_frame(s_minutes_layer, minutes_frame);
}


static void update_time() 
{
	time_t now = time(NULL);
    struct tm *t = localtime(&now);

    int new_hours = clock_is_24h_style() ? t->tm_hour : (t->tm_hour % 12 ? t->tm_hour % 12 : 12);
    int new_minutes = t->tm_min;
	int new_day = t->tm_mday;

    if (new_hours != s_hours_val) 
	{
        s_hours_val = new_hours;
        layer_mark_dirty(s_hours_layer);
    }

    if (new_minutes != s_minutes_val) 
	{
        s_minutes_val = new_minutes;
        layer_mark_dirty(s_minutes_layer);
    }
	
	if(new_day != s_day_val)
	{
		s_day_val = new_day;
		static char d_buf[7];
    	strftime(d_buf, sizeof(d_buf), "%a\n%d", t);
    	text_layer_set_text(s_date_layer, d_buf);
	}
}

static void update_steps() 
{
	int new_steps = 0;
    #ifdef DEBUG
	s_step_count = 4567;
	#else
	HealthMetric metric = HealthMetricStepCount;
	time_t start = time_start_of_today();
	time_t end = time(NULL);

	HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

	if (mask & HealthServiceAccessibilityMaskAvailable) 
	{
		new_steps = (int)health_service_sum(metric, start, end);
	}
	#endif
    if (new_steps != s_step_count) 
	{
		//we've just met/surpassed our step goal for the day
		if(settings.VibrateOnStepGoal && s_step_count != UNINITIALIZED && new_steps >= settings.StepGoal && s_step_count < settings.StepGoal)
		{
			//vibrate the watch
			VibePattern pat = 
			{
			  .durations = stepGoalAchievedVibePattern,
			  .num_segments = ARRAY_LENGTH(stepGoalAchievedVibePattern),
			};
			vibes_enqueue_custom_pattern(pat);

		}
        s_step_count = new_steps;
        layer_mark_dirty(s_progress_layer);
		
    }
}

static void tick_handler(struct tm *t, TimeUnits units) 
{
    update_time();
	 // only every 5 minutes
	if (t->tm_min % 5 == 0) 
	{
        update_steps();
    }
}

static void battery_handler(BatteryChargeState state) 
{
    s_battery_lvl = state.charge_percent;
    static char b_buf[5];
    snprintf(b_buf, sizeof(b_buf), "%d%%", s_battery_lvl);
    text_layer_set_text(s_battery_layer, b_buf);
}

static void ApplySettings()
{	
	// Apply color settings
    window_set_background_color(s_main_window, settings.BackgroundColor);
    text_layer_set_text_color(s_date_layer,    settings.DateColor);
    text_layer_set_text_color(s_battery_layer, settings.BatteryColor);

    // Apply visibility settings
	SetDateAndBatteryVisiblity();
    layer_set_hidden(s_progress_layer, !settings.ShowStepProgress);
	
	// Apply Bluetooth visibility
    if (settings.ShowBTConnection) 
	{
		connection_service_subscribe((ConnectionHandlers) 
									 { 
										 .pebble_app_connection_handler = bluetooth_callback 
									 });
		bluetooth_callback(connection_service_peek_pebble_app_connection());
	}
	else 
	{
		connection_service_unsubscribe();
		bitmap_layer_set_bitmap(s_blade_layer, s_blade_bitmap);
	}

    // Redraw the progress bar since color or goal may have changed
    layer_mark_dirty(s_progress_layer);
}

static void prv_unobstructed_will_change(GRect final_unobstructed_screen_area, void *context) 
{
    // Hide date and battery before the peek animation starts
    SetDateAndBatteryVisiblity();
}

static void prv_unobstructed_change(AnimationProgress progress, void *context) 
{
    // Called repeatedly during the animation. Move minutes to track the shrinking screen
    UpdateMinutesPosition();
}

static void prv_unobstructed_did_change(void *context) 
{
    // Called once after the animation finishes
    SetDateAndBatteryVisiblity();
}

static void window_load(Window *window) 
{
    s_window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(s_window_layer);

    s_progress_layer = layer_create(bounds);
    layer_set_update_proc(s_progress_layer, progress_update_proc);
    layer_add_child(s_window_layer, s_progress_layer);

    digit_renderer_init();
    int digit_h = digit_renderer_get_digit_height();
    int digit_w = digit_renderer_get_digit_width();
    int pair_w = digit_w * 2;

#if defined(PBL_PLATFORM_EMERY)
	//larger fonts for Emery
    s_battery_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAIRA_STENCIL_24));
    s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAIRA_STENCIL_24));
#else
    s_battery_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAIRA_STENCIL_18));
    s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAIRA_STENCIL_18));
#endif

    s_hours_layer = layer_create(GRect(EDGE_PADDING, EDGE_PADDING, pair_w, digit_h));
    layer_set_update_proc(s_hours_layer, hours_update_proc);
    layer_add_child(s_window_layer, s_hours_layer);

    s_minutes_layer = layer_create(GRect(bounds.size.w - EDGE_PADDING - pair_w, bounds.size.h - EDGE_PADDING - digit_h, pair_w, digit_h));
    layer_set_update_proc(s_minutes_layer, minutes_update_proc);
    layer_add_child(s_window_layer, s_minutes_layer);
	
	// Date: directly below the hours layer, left-aligned to it.
    GRect hours_frame = layer_get_frame(s_hours_layer);
    int date_y = hours_frame.origin.y + hours_frame.size.h + DATE_OFFSET;
    s_date_layer = text_layer_create(GRect(hours_frame.origin.x + EDGE_PADDING, date_y, pair_w, 60));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_font(s_date_layer, s_date_font);
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
    layer_add_child(s_window_layer, text_layer_get_layer(s_date_layer));
	
	// Battery: directly above the minutes layer, right-aligned to it.
    GRect minutes_frame = layer_get_frame(s_minutes_layer);
    int battery_h = 35;
    int battery_y = minutes_frame.origin.y - battery_h - BATTERY_OFFSET;
    s_battery_layer = text_layer_create(GRect(minutes_frame.origin.x-EDGE_PADDING, battery_y, pair_w, battery_h));
    text_layer_set_background_color(s_battery_layer, GColorClear);
    text_layer_set_font(s_battery_layer, s_battery_font);
    text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
    layer_add_child(s_window_layer, text_layer_get_layer(s_battery_layer));
	
    s_blade_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
	s_blade_no_bt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_NO_BT);

    s_blade_layer = bitmap_layer_create(bounds);
    bitmap_layer_set_bitmap(s_blade_layer, s_blade_bitmap);
    bitmap_layer_set_compositing_mode(s_blade_layer, GCompOpSet);
    layer_add_child(s_window_layer, bitmap_layer_get_layer(s_blade_layer));
	
	ApplySettings();

    update_time();
    battery_handler(battery_state_service_peek());
	
	// Subscribe to unobstructed area events
UnobstructedAreaHandlers unobstructed_handlers = 
{
    .will_change = prv_unobstructed_will_change,
    .change      = prv_unobstructed_change,
    .did_change  = prv_unobstructed_did_change
};
unobstructed_area_service_subscribe(unobstructed_handlers, NULL);

// Handle the case where Timeline Peek is ALREADY active when the face loads
UpdateMinutesPosition();
prv_unobstructed_did_change(NULL);
}

static void window_unload(Window *window) 
{
    digit_renderer_deinit();
    fonts_unload_custom_font(s_battery_font);
    fonts_unload_custom_font(s_date_font);
    layer_destroy(s_hours_layer);
    layer_destroy(s_minutes_layer);
    text_layer_destroy(s_battery_layer);
    text_layer_destroy(s_date_layer);
    gbitmap_destroy(s_blade_bitmap);
	gbitmap_destroy(s_blade_no_bt_bitmap);
    bitmap_layer_destroy(s_blade_layer);
    layer_destroy(s_progress_layer);
	tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    connection_service_unsubscribe();
	unobstructed_area_service_unsubscribe();
}

static void inbox_received_handler(DictionaryIterator *iter, void *ctx) 
{
    Tuple *t = dict_read_first(iter);
    while (t) 
    {
        // Colors
        if (t->key == MESSAGE_KEY_BackgroundColor)   settings.BackgroundColor   = GColorFromHEX(t->value->int32);
        if (t->key == MESSAGE_KEY_DateColor)         settings.DateColor         = GColorFromHEX(t->value->int32);
        if (t->key == MESSAGE_KEY_BatteryColor)      settings.BatteryColor      = GColorFromHEX(t->value->int32);
        if (t->key == MESSAGE_KEY_StepsColor)        settings.StepsColor        = GColorFromHEX(t->value->int32);
        // Visibility toggles
        if (t->key == MESSAGE_KEY_ShowDate)          settings.ShowDate          = (bool)t->value->int8;
        if (t->key == MESSAGE_KEY_ShowBattery)       settings.ShowBattery       = (bool)t->value->int8;
        if (t->key == MESSAGE_KEY_ShowStepProgress)  settings.ShowStepProgress  = (bool)t->value->int8;
		if (t->key == MESSAGE_KEY_ShowBTConnection)  settings.ShowBTConnection  = (bool)t->value->int8;
        // Health
        if (t->key == MESSAGE_KEY_StepGoal)          settings.StepGoal         	= (int)t->value->int32;
		if (t->key == MESSAGE_KEY_VibrateOnStepGoal) settings.VibrateOnStepGoal = (bool)t->value->int8;
		
        t = dict_read_next(iter);
    }

    settings_save(&settings);
	
	//Apply the settings that were just read in
    ApplySettings();
}

int main(void) 
{
    settings_load(&settings);
    
    app_message_register_inbox_received(inbox_received_handler);
    app_message_open(256, 256);
    
    s_main_window = window_create();
	
	window_set_background_color(s_main_window, settings.BackgroundColor);
	
    window_set_window_handlers(s_main_window, (WindowHandlers) 
							   { 
									.load = window_load, 
									.unload = window_unload 
								});
    
    window_stack_push(s_main_window, true);
    
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    battery_state_service_subscribe(battery_handler);
	update_steps();
	// Register for Bluetooth connection updates
	connection_service_subscribe((ConnectionHandlers) { .pebble_app_connection_handler = bluetooth_callback});
    app_event_loop();
}