#include <pebble.h>
#include "debug.h"
#include "chars.h"

static Window *window_main;
static Layer *time_layer;

static GBitmap *seg_hor;
static GBitmap *seg_vert;

struct tm *now=NULL;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	now=tick_time;

	layer_mark_dirty(time_layer);
}

static void update_time_layer(Layer *this, GContext *ctx)
{
	int d, i, j;
	char *digit;

	char time[5]="1234";

	if (now==NULL)
		return;

	snprintf(time, 5, "%02d%02d", now->tm_hour, now->tm_min);

	// these are relative to the graphics layer, vertical is always 0
	int choffsets[4]={1,37,73,109};
	// offsets relative to character
	static int hoffsets[3]={0,4,29};
	static int voffsets[9]={0,0,0,4,32,4,36,64,36};

	for (d=0;d<4;d++) {			// digit
		digit=get_number(time[d]);

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
	seg_hor	= gbitmap_create_with_resource(RESOURCE_ID_HORIZONTAL_SEGMENT);
	seg_vert = gbitmap_create_with_resource(RESOURCE_ID_VERTICAL_SEGMENT);
}

static void window_main_unload(Window *window)
{
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

	time_layer = layer_create(GRect(0,50,144,69));

	layer_add_child(window_get_root_layer(window_main), time_layer);
	layer_set_update_proc(time_layer, update_time_layer);
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

