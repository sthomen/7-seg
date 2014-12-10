#include <pebble.h>
#include "debug.h"
#include "chars.h"

static Window *window_main;
static Layer *time_layer;
static Layer *colon_layer;

static GBitmap *seg_hor;
static GBitmap *seg_vert;

static bool blink=false;

struct tm *now=NULL;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	now=tick_time;

	switch (units_changed) {
		case MINUTE_UNIT:
			layer_mark_dirty(time_layer);
		case SECOND_UNIT:
			layer_mark_dirty(colon_layer);
			break;
		default:
			break;
	}
}

static void update_colon_layer(Layer *this, GContext *ctx)
{
	graphics_context_set_fill_color(ctx, GColorClear);

	if (blink) {
		graphics_fill_rect(ctx, GRect(0,0,8,8), 0, (GCornerMask)NULL);
		graphics_fill_rect(ctx, GRect(0,18,8,8), 0, (GCornerMask)NULL);

		blink=false;
	} else {
		blink=true;
	}
}

static void update_time_layer(Layer *this, GContext *ctx)
{
	int d, i, j;
	char *digit;

	char time[5]="LoAd";

	if (now!=NULL) {
		snprintf(time, 5, "%02d%02d", (clock_is_24h_style() ? now->tm_hour : now->tm_hour % 12),
			now->tm_min);
	}

	// these are relative to the graphics layer, vertical is always 0
	int choffsets[4]={1,34,80,113};
	// offsets relative to character
	static int hoffsets[3]={0,4,25};
	static int voffsets[9]={0,0,0,4,32,4,36,64,36};

	for (d=0;d<4;d++) {			// digit
		digit=get_7seg_number(time[d]);
		if (digit==NULL)
			continue;

		for (i=0;i<3;i++) {		// columns
			for (j=0;j<3;j++) {	// rows
				if (j==1) {
					if (digit[j+(i*3)]==1) 
						graphics_draw_bitmap_in_rect(ctx, seg_hor,
							GRect(choffsets[d]+hoffsets[j],voffsets[j+(i*3)],
								seg_hor->bounds.size.w,seg_hor->bounds.size.h));
				} else {
					if (digit[j+(i*3)]==1) 
						graphics_draw_bitmap_in_rect(ctx, seg_vert,
							GRect(choffsets[d]+hoffsets[j],voffsets[j+(i*3)],
								seg_vert->bounds.size.w,seg_vert->bounds.size.h));
				}
			}
		}
	}
}

static void window_main_load(Window *window)
{
	time_t t;

	seg_hor	= gbitmap_create_with_resource(RESOURCE_ID_HORIZONTAL_SEGMENT);
	seg_vert = gbitmap_create_with_resource(RESOURCE_ID_VERTICAL_SEGMENT);

	/* prep clock */
	t=time(NULL);
	now=localtime(&t);
}

static void window_main_unload(Window *window)
{
	layer_destroy(time_layer);
	layer_destroy(colon_layer);
}

static void init()
{
	static WindowHandlers window_handlers={
		.load = window_main_load,
		.unload = window_main_unload
	};

	window_main = window_create();

	window_set_window_handlers(window_main, window_handlers);
	window_stack_push(window_main, true);

	window_set_background_color(window_main, GColorBlack);

	tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

	/* setup time layer */
	time_layer = layer_create(GRect(0,50,144,69));

	layer_add_child(window_get_root_layer(window_main), time_layer);
	layer_set_update_proc(time_layer, update_time_layer);

	/* setup colon layer */
	colon_layer = layer_create(GRect(68,71,8,26));

	layer_add_child(window_get_root_layer(window_main), colon_layer);
	layer_set_update_proc(colon_layer, update_colon_layer);
}

static void deinit()
{
	tick_timer_service_unsubscribe();
	window_destroy(window_main);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}

