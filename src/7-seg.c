#include <pebble.h>
#include "debug.h"
#include "chars.h"

static Window *window_main;
static Layer *time_layer;
static Layer *colon_layer;
static Layer *date_layer;
static InverterLayer *invert_layer;

enum {	/* these must match constants in appinfo */
	CONFIG_INVERT = 0,
	CONFIG_HALFTONE = 1
};

struct segs_seven {
	GBitmap *horizontal;
	GBitmap *vertical;
	GBitmap *horizontal_dark;
	GBitmap *vertical_dark;
};

struct segs_fourteen {
	GBitmap *top;
	GBitmap *side;
	GBitmap *diagonal_left;
	GBitmap *diagonal_right;
	GBitmap *vertical;
	GBitmap *horizontal;
};

struct {
	struct segs_seven seven;
	struct segs_seven seven_dark;
	struct segs_fourteen fourteen;
	struct segs_fourteen fourteen_dark;
} segs;

static bool halftone=true;
static bool blink=false;
struct tm *now=NULL;

static void set_invert(bool which) {
	layer_set_hidden(inverter_layer_get_layer(invert_layer), !which);	// because the layer inverts
	layer_mark_dirty(inverter_layer_get_layer(invert_layer));

	persist_write_int(CONFIG_INVERT, (which ? 1 : 0));
}

static void set_halftone(bool which) {
	halftone=which;

	layer_mark_dirty(colon_layer);
	layer_mark_dirty(time_layer);
	layer_mark_dirty(date_layer);

	persist_write_int(CONFIG_HALFTONE, (which ? 0 : 1));
}


static void message_in_handler(DictionaryIterator *iter, void *context)
{
	Tuple *t = dict_read_first(iter);

	while (t != NULL) {
		switch (t->key) {
			case CONFIG_INVERT:
				DEBUG_INFO("INVERT: %s", t->value->cstring);
				if (strcmp(t->value->cstring, "true")==0) {
					set_invert(true);
				} else {
					set_invert(false);
				}
				break;
			case CONFIG_HALFTONE:
				DEBUG_INFO("HALFTONE: %s", t->value->cstring);
				if (strcmp(t->value->cstring, "true")==0) {
					set_halftone(true);
				} else {
					set_halftone(false);
				}
				break;
		}
		t=dict_read_next(iter);
	}
}

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
	char day[4];

	char *digit;
	struct segs_fourteen *sf;
	GBitmap *seg;

	if (now!=NULL) {
		strftime((char *)&day, 4, "%a", now);
		snprintf(date, 7, "%s%2d", day, now->tm_mday);
	}

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
			if (digit[i]==1) {
				sf=&segs.fourteen;
			} else {
				if (!halftone) {
					continue;
				}
				sf=&segs.fourteen_dark;
			}

			switch (i) {
				case 0:
				case 13:
					seg=sf->top;
					break;
				case 1:
				case 5:
				case 8:
				case 12:
					seg=sf->side;
					break;
				case 2:
				case 11:
					seg=sf->diagonal_left;
					break;
				case 4:
				case 9:
					seg=sf->diagonal_right;
					break;
				case 3:
				case 10:
					seg=sf->vertical;
					break;
				case 6:
				case 7:
					seg=sf->horizontal;
					break;
			}

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
		if (halftone) {
			graphics_draw_bitmap_in_rect(ctx, segs.seven_dark.horizontal, GRect(-4,0,12,8));
			graphics_draw_bitmap_in_rect(ctx, segs.seven_dark.horizontal, GRect(-4,18,12,8));
		}

		blink=true;
	}
}

static void update_time_layer(Layer *this, GContext *ctx)
{
	int d, i, j;
	char *digit;
	struct segs_seven *ss;
	GBitmap *seg;

	char time[5]="LoAd";

	if (now!=NULL) {
		snprintf(time, 5, "%2d%02d", (clock_is_24h_style() ? now->tm_hour : now->tm_hour % 12),
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
				if (digit[j+(i*3)]==1) {
					ss=&segs.seven;
				} else {
					if (!halftone) {
						continue;
					}
					ss=&segs.seven_dark;
				}
				if (j==1) {
					seg=ss->horizontal;
				} else {
					seg=ss->vertical;
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

	/* inversion layer */
	invert_layer = inverter_layer_create(GRect(0,0,144,168));
	layer_add_child(window_get_root_layer(window_main), inverter_layer_get_layer(invert_layer));

	/* XXX since 0 is the default if no value is set, make this the same in the js */
	set_invert((persist_read_int(CONFIG_INVERT)==0 ? false : true));

	set_halftone((persist_read_int(CONFIG_HALFTONE)==0 ? true : false));

	segs.seven.horizontal = gbitmap_create_with_resource(RESOURCE_ID_HORIZONTAL_SEGMENT);
	segs.seven.vertical = gbitmap_create_with_resource(RESOURCE_ID_VERTICAL_SEGMENT);
	segs.seven_dark.horizontal = gbitmap_create_with_resource(RESOURCE_ID_HORIZONTAL_DARK_SEGMENT);
	segs.seven_dark.vertical = gbitmap_create_with_resource(RESOURCE_ID_VERTICAL_DARK_SEGMENT);

	segs.fourteen.top = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_TOP);
	segs.fourteen.side = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_SIDE);
	segs.fourteen.diagonal_left = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_DIAGONAL_LEFT);
	segs.fourteen.diagonal_right = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_DIAGONAL_RIGHT);
	segs.fourteen.vertical = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_VERTICAL);
	segs.fourteen.horizontal = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_HORIZONTAL);

	segs.fourteen_dark.top = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_TOP_DARK);
	segs.fourteen_dark.side = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_SIDE_DARK);
	segs.fourteen_dark.diagonal_left = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_DIAGONAL_LEFT_DARK);
	segs.fourteen_dark.diagonal_right = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_DIAGONAL_RIGHT_DARK);
	segs.fourteen_dark.vertical = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_VERTICAL_DARK);
	segs.fourteen_dark.horizontal = gbitmap_create_with_resource(RESOURCE_ID_14SEGMENT_HORIZONTAL_DARK);

	/* prep clock */
	t=time(NULL);
	now=localtime(&t);
}

static void window_main_unload(Window *window)
{
	layer_destroy(time_layer);
	layer_destroy(colon_layer);
	layer_destroy(date_layer);
	layer_destroy(inverter_layer_get_layer(invert_layer));
}

static void init()
{
	static WindowHandlers window_handlers={
		.load = window_main_load,
		.unload = window_main_unload
	};

	setlocale(LC_ALL, "");

	window_main = window_create();

	window_set_window_handlers(window_main, window_handlers);
	window_stack_push(window_main, true);

	window_set_background_color(window_main, GColorBlack);

	tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

	app_message_register_inbox_received(message_in_handler);
	app_message_open(64, 64);
}

static void deinit()
{
	tick_timer_service_unsubscribe();
	window_destroy(window_main);
	app_message_deregister_callbacks();
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}

