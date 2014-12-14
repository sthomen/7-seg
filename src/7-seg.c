#include <pebble.h>
#include "debug.h"
#include "chars.h"

static Window *window_main;
static Layer *time_layer;
static Layer *colon_layer;
static Layer *date_layer;

struct {
	struct {
		GBitmap *horizontal;
		GBitmap *vertical;
		GBitmap *horizontal_dark;
		GBitmap *vertical_dark;
	} seven;
	struct {
		GBitmap *top;
		GBitmap *side;
		GBitmap *diagonal_left;
		GBitmap *diagonal_right;
		GBitmap *vertical;
		GBitmap *horizontal;
	} fourteen;
} segs;

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
		case DAY_UNIT:
			layer_mark_dirty(date_layer);
			break;
		default:
			break;
	}
}

static void update_date_layer(Layer *this, GContext *ctx)
{
	int d, i;
	char date[6]="Hello";

	char *digit;
	GBitmap *seg;

	/*       0
	 * 1  2  3  4  5
	 *    6     7
	 * 8  9 10 11 12
	 *      13
	 */

	int choffsets[5]={3,31,59,87,115};
	int hoffsets[14]={2,0,4,11,15,22,2,13,0,4,11,15,22,2};
	int voffsets[14]={0,2,4,4,4,2,11,11,13,15,13,15,13,22};

	for (d=0;d<5;d++) {
		digit=get_14seg(date[d]);
		if (digit==NULL)
			return;

		for (i=0;i<14;i++) {
			switch (i) {
				case 0:
				case 13:
					seg=segs.fourteen.top;
					break;
				case 1:
				case 5:
				case 8:
				case 12:
					seg=segs.fourteen.side;
					break;
				case 2:
				case 11:
					seg=segs.fourteen.diagonal_left;
					break;
				case 4:
				case 9:
					seg=segs.fourteen.diagonal_right;
					break;
				case 3:
				case 10:
					seg=segs.fourteen.vertical;
					break;
				case 6:
				case 7:
					seg=segs.fourteen.horizontal;
					break;
			}
			if (digit[i]==1)
				graphics_draw_bitmap_in_rect(ctx, seg,
					GRect(choffsets[d]+hoffsets[i],voffsets[i],
						seg->bounds.size.w,seg->bounds.size.h));
		}
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
		graphics_draw_bitmap_in_rect(ctx, segs.seven.horizontal_dark, GRect(-4,0,12,8));
		graphics_draw_bitmap_in_rect(ctx, segs.seven.horizontal_dark, GRect(-4,18,12,8));

		blink=true;
	}
}

static void update_time_layer(Layer *this, GContext *ctx)
{
	int d, i, j;
	char *digit;
	GBitmap *seg;

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
		digit=get_7seg(time[d]);
		if (digit==NULL)
			continue;

		for (i=0;i<3;i++) {		// columns
			for (j=0;j<3;j++) {	// rows
				if (j==1) {
					if (digit[j+(i*3)]==1) {
						seg=segs.seven.horizontal;
					} else {
						seg=segs.seven.horizontal_dark;
					}
				} else {
					if (digit[j+(i*3)]==1) {
						seg=segs.seven.vertical;
					} else {
						seg=segs.seven.vertical_dark;
					}
				}
				if (!(i==0 && (j==0 || j==2)))
					graphics_draw_bitmap_in_rect(ctx, seg,
						GRect(choffsets[d]+hoffsets[j],voffsets[j+(i*3)],
							seg->bounds.size.w,seg->bounds.size.h));
			}
		}
	}
}

static void window_main_load(Window *window)
{
	time_t t;

	segs.seven.horizontal = gbitmap_create_with_resource(RESOURCE_ID_HORIZONTAL_SEGMENT);
	segs.seven.vertical = gbitmap_create_with_resource(RESOURCE_ID_VERTICAL_SEGMENT);
	segs.seven.horizontal_dark = gbitmap_create_with_resource(RESOURCE_ID_HORIZONTAL_DARK_SEGMENT);
	segs.seven.vertical_dark = gbitmap_create_with_resource(RESOURCE_ID_VERTICAL_DARK_SEGMENT);

	segs.fourteen.top = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_TOP);
	segs.fourteen.side = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_SIDE);
	segs.fourteen.diagonal_left = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_DIAGONAL_LEFT);
	segs.fourteen.diagonal_right = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_DIAGONAL_RIGHT);
	segs.fourteen.vertical = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_VERTICAL);
	segs.fourteen.horizontal = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_HORIZONTAL);

	/* prep clock */
	t=time(NULL);
	now=localtime(&t);
}

static void window_main_unload(Window *window)
{
	layer_destroy(time_layer);
	layer_destroy(colon_layer);
	layer_destroy(date_layer);
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

	/* setup date layer */
	date_layer = layer_create(GRect(0,10,144,25));

	layer_add_child(window_get_root_layer(window_main), date_layer);
	layer_set_update_proc(date_layer, update_date_layer);
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

