#include <pebble.h>


/*
Helpful commands:
"export PEBBLE_PHONE=XXX.XXX.X.XXX" (e.g. export PEBBLE_PHONE=192.168.0.237) to set default pebble install destination
"pebble build" to build the watch face or app
"pebble install" to install the new watch face or app
"pebble logs" to view debug output
"pebble install --emulator basalt" to install and use in the emulator
*/

// Left padding for the hours layer (offsets the layer from the left screen edge)
#define HOURS_LEFT_PADDING 20

// Right padding for the minutes layer (shrinks the layer away from the right screen edge)
#define MINUTES_RIGHT_PADDING 20

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

static unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }

  // Converts "0" to "12"
  unsigned short display_hour = hour % 12;
  return display_hour ? display_hour : 12;
}

static void display_time(struct tm *tick_time) {
	//buffer size should only need to be 3 (HH\0 or MM\0)but for some reason I get odd behavior if this is set to 3. 8 is an arbitrary value
  static char s_hours_buf[8];
  static char s_mins_buf[8];
	
	snprintf(s_hours_buf, sizeof(s_hours_buf), "%02d", get_display_hour(tick_time->tm_hour));
  snprintf(s_mins_buf,  sizeof(s_mins_buf),  "%02d", tick_time->tm_min);
  
  text_layer_set_text(s_hours_layer,   s_hours_buf);
  text_layer_set_text(s_minutes_layer, s_mins_buf);
}

static void display_date(struct tm *tick_time) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "display_date start");
  // Write the current month string and day into a buffer (XXX\nXX\0)
  static char s_buffer[7];
  strftime(s_buffer, sizeof(s_buffer), "%a\n%d", tick_time);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "display_date: %s", s_buffer);
  // Display this date on the TextLayer
  text_layer_set_text(s_date_layer, s_buffer);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "display_date end");
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

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  display_time(tick_time);
  display_date(tick_time);
}

static void window_load(Window *window)
{
  // Get information about the Window
  s_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(s_window_layer);

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
  s_battery_layer = text_layer_create(GRect((bounds.size.w/2), bounds.size.h / 2 - 9, (bounds.size.w/2) - MINUTES_RIGHT_PADDING, 40));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorVeryLightBlue);
  text_layer_set_font(s_battery_layer, s_battery_font);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  layer_add_child(s_window_layer, text_layer_get_layer(s_battery_layer));

  // Date label: centered vertically at the split, left side
  s_date_layer = text_layer_create(GRect(HOURS_LEFT_PADDING, bounds.size.h / 2 - 19, 70, 80));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
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
  s_battery_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 9, bounds.size.w - MINUTES_RIGHT_PADDING, 20));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorVeryLightBlue);
  text_layer_set_font(s_battery_layer, s_battery_font);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  layer_add_child(s_window_layer, text_layer_get_layer(s_battery_layer));

  // Date label: centered vertically at the split, left side
  s_date_layer = text_layer_create(GRect(HOURS_LEFT_PADDING, bounds.size.h / 2 - 19, 70, 40));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
  layer_add_child(s_window_layer, text_layer_get_layer(s_date_layer));
#endif

  // Hours layer: top half of screen, white text, left-aligned
  // HOURS_LEFT_PADDING offsets the layer (and therefore the text) from the left screen edge
  // Shift the Y origin by the offset, and keep height sufficient for the font
s_hours_layer = text_layer_create(GRect(
    HOURS_LEFT_PADDING, 
    hours_y_offset,                               // Use offset here to move it UP
    bounds.size.w - HOURS_LEFT_PADDING, 
    (bounds.size.h / 2) + abs(hours_y_offset)     // Increase height so bottom isn't cut
));
  text_layer_set_background_color(s_hours_layer, GColorClear);
  text_layer_set_text_color(s_hours_layer, GColorWhite);
  text_layer_set_font(s_hours_layer, s_time_font);
  text_layer_set_text_alignment(s_hours_layer, GTextAlignmentLeft);
  layer_add_child(s_window_layer, text_layer_get_layer(s_hours_layer));

  // Calculate the width of the layer to be the full width minus twice the padding
  // Then shift the X-origin by the padding amount.
  int layer_width = bounds.size.w - (MINUTES_RIGHT_PADDING * 2);

s_minutes_layer = text_layer_create(GRect(
    MINUTES_RIGHT_PADDING, 
    (bounds.size.h / 2) + minutes_y_offset,       // Use offset here to move it DOWN
    layer_width, 
    bounds.size.h / 2                             // Keep standard height
));

  // Disable clipping so the right-leaning italic overhang can spill safely into the padding area
  layer_set_clips(text_layer_get_layer(s_minutes_layer), false);

  text_layer_set_background_color(s_minutes_layer, GColorClear);
  text_layer_set_text_color(s_minutes_layer, GColorVeryLightBlue);
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
APP_LOG(APP_LOG_LEVEL_DEBUG, "Initial time: %02d:%02d", tick_time->tm_hour, tick_time->tm_min);
  display_time(tick_time);
  display_date(tick_time);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
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
}

static void prv_init(void)
{
  s_main_window = window_create();

  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers){
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

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_main_window);

  app_event_loop();
  prv_deinit();
}