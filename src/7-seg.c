#include <pebble.h>
#include "debug.h"
#include "chars.h"

static Window *window_main;
static Layer *time_layer;
static Layer *colon_layer;
static Layer *date_layer;
static Layer *info_layer;
static InverterLayer *invert_layer;

enum {	/* these must match constants in appinfo */
	CONFIG_INVERT = 0,
	CONFIG_HALFTONE = 1,
	CONFIG_BLINK = 2,
	CONFIG_VIBRATE = 3,
	CONFIG_SHOWTOP = 4,
	CONFIG_SHOWBOTTOM = 5
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

static int8_t charge=0;

struct {
	bool vibrate;
	bool bluetooth;
	bool halftone;
	bool invert;
	bool blink;
	bool showtop;
	bool showbottom;
} settings = {		/* defaults, make sure these match the JS file */
	true,
	true,
	true,
	true,
	true,
	true,
	true
};

static bool charging=false;
static bool blink=true;
struct tm *now=NULL;

static void set_vibrate(bool which)
{
	settings.vibrate=which;
	persist_write_bool(CONFIG_VIBRATE, which);
}

static void set_showtop(bool which)
{
	settings.showtop=which;
	persist_write_bool(CONFIG_SHOWTOP, which);
	layer_mark_dirty(date_layer);
}

static void set_showbottom(bool which)
{
	settings.showbottom=which;
	persist_write_bool(CONFIG_SHOWBOTTOM, which);
	layer_mark_dirty(info_layer);
}


static void set_invert(bool which)
{
	layer_set_hidden(inverter_layer_get_layer(invert_layer), !which);
	layer_mark_dirty(inverter_layer_get_layer(invert_layer));

	persist_write_bool(CONFIG_INVERT, which);
}

static void set_halftone(bool which)
{
	settings.halftone=which;

	layer_mark_dirty(colon_layer);
	layer_mark_dirty(time_layer);
	layer_mark_dirty(date_layer);
	layer_mark_dirty(info_layer);

	persist_write_bool(CONFIG_HALFTONE, which);
}

static void set_blink(bool which)
{
	settings.blink=which;
	blink=true;
	layer_mark_dirty(colon_layer);

	persist_write_bool(CONFIG_BLINK, which);
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
			case CONFIG_BLINK:
				DEBUG_INFO("BLINK: %s", t->value->cstring);
				if (strcmp(t->value->cstring, "true")==0) {
					set_blink(true);
				} else {
					set_blink(false);
				}
				break;
			case CONFIG_VIBRATE:
				DEBUG_INFO("VIBRATE: %s", t->value->cstring);
				if (strcmp(t->value->cstring, "true")==0) {
					set_vibrate(true);
				} else {
					set_vibrate(false);
				}
				break;
			case CONFIG_SHOWTOP:
				DEBUG_INFO("SHOWTOP: %s", t->value->cstring);
				if (strcmp(t->value->cstring, "true")==0) {
					set_showtop(true);
				} else {
					set_showtop(false);
				}
				break;
			case CONFIG_SHOWBOTTOM:
				DEBUG_INFO("SHOWBOTTOM: %s", t->value->cstring);
				if (strcmp(t->value->cstring, "true")==0) {
					set_showbottom(true);
				} else {
					set_showbottom(false);
				}
				break;
		}
		t=dict_read_next(iter);
	}
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	now=tick_time;

	if (units_changed & MINUTE_UNIT) {
		layer_mark_dirty(time_layer);
	}

	if (units_changed & SECOND_UNIT) {
		layer_mark_dirty(colon_layer);
		if (settings.blink && charging)
			layer_mark_dirty(info_layer);
	}

	if (units_changed & DAY_UNIT) {
		layer_mark_dirty(date_layer);
	}
}

static void bluetooth_handler(bool connected)
{
	if (settings.vibrate)
		vibes_short_pulse();

	settings.bluetooth=connected;
	layer_mark_dirty(info_layer);
}

static void battery_handler(BatteryChargeState state)
{
	charge=state.charge_percent;
	charging=state.is_charging;

	layer_mark_dirty(info_layer);
}

/*       0
 * 1  2  3  4  5
 *    6     7
 * 8  9 10 11 12
 *      13
 */

static void draw_fourteen_segment(GContext *ctx, char *string)
{
	int d, i;

	char *digit;
	struct segs_fourteen *sf;
	GBitmap *seg;
	GRect bounds;
	
	int choffsets[5]={3,31,59,87,115};
	int hoffsets[14]={2,0,4,11,15,22,2,13,0,4,11,15,22,2};
	int voffsets[14]={0,2,4,4,4,2,11,11,13,15,13,15,13,22};

	for (d=0;d<5;d++) {
		digit=get_14seg(string[d]);
		if (digit==NULL)
			digit=get_14seg('?');

		for (i=0;i<14;i++) {
			if (digit[i]==1) {
				sf=&segs.fourteen;
			} else {
				if (!settings.halftone) {
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

			bounds=gbitmap_get_bounds(seg);

			graphics_draw_bitmap_in_rect(ctx, seg,
				GRect(choffsets[d]+hoffsets[i],voffsets[i],
					bounds.size.w,bounds.size.h));
		}
	}
}

static void update_info_layer(Layer *this, GContext *ctx)
{
	if (!settings.showbottom)
		return;

	char data[6]="World";

	if (now!=NULL) {
		if (settings.blink && charging && blink) {
			snprintf(data, 6, "  %% %c", (settings.bluetooth ? 'B' : '-'));
		} else {
			snprintf(data, 6, "%2d%% %c", (charge==100 ? 99 : charge), (settings.bluetooth ? 'B' : '-'));
		}
	}

	draw_fourteen_segment(ctx, data);
}

static void update_date_layer(Layer *this, GContext *ctx)
{
	if (!settings.showtop)
		return;

	char date[6]="Hello";
	char day[4];

	if (now!=NULL) {
		setlocale(LC_ALL, "");
		strftime((char *)&day, 4, "%a", now);
		snprintf(date, 7, "%s%s%2d", day, (strlen(day)==2 ? " " : ""), now->tm_mday);
	}

	draw_fourteen_segment(ctx, date);
}

static void update_colon_layer(Layer *this, GContext *ctx)
{
	graphics_context_set_fill_color(ctx, GColorClear);

	if (blink) {
		graphics_fill_rect(ctx, GRect(0,0,8,8), 0, (GCornerMask)NULL);
		graphics_fill_rect(ctx, GRect(0,18,8,8), 0, (GCornerMask)NULL);
	} else {
		if (settings.halftone) {
			graphics_draw_bitmap_in_rect(ctx, segs.seven_dark.horizontal, GRect(-4,0,12,8));
			graphics_draw_bitmap_in_rect(ctx, segs.seven_dark.horizontal, GRect(-4,18,12,8));
		}
	}

	if (settings.blink)
		blink=!blink;
}

static void update_time_layer(Layer *this, GContext *ctx)
{
	int d, i, j;
	char *digit;
	struct segs_seven *ss;
	GBitmap *seg;
	GRect bounds;

	char time[5]="----";

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
					if (!settings.halftone)
						continue;

					ss=&segs.seven_dark;
				}
				if (j==1) {
					seg=ss->horizontal;
				} else {
					seg=ss->vertical;
				}
				if (!(i==0 && (j==0 || j==2))) {
					bounds=gbitmap_get_bounds(seg);
					graphics_draw_bitmap_in_rect(ctx, seg,
						GRect(choffsets[d]+hoffsets[j],voffsets[j+(i*3)],
							bounds.size.w,bounds.size.h));
				}
			}
		}
	}
}

static bool read_bool(const uint32_t which, bool def)
{
	if (persist_exists(which)) {
		return persist_read_bool(which);
	}

	return def;
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

	/* setup info layer */
	info_layer = layer_create(GRect(0,133,144,25));

	layer_add_child(window_get_root_layer(window_main), info_layer);
	layer_set_update_proc(info_layer, update_info_layer);

	/* inversion layer */
	invert_layer = inverter_layer_create(GRect(0,0,144,168));
	layer_add_child(window_get_root_layer(window_main), inverter_layer_get_layer(invert_layer));


	/* set persistent values */

	set_vibrate(read_bool(CONFIG_VIBRATE, settings.vibrate));
	set_invert(read_bool(CONFIG_INVERT, settings.invert));
	set_halftone(read_bool(CONFIG_HALFTONE, settings.halftone));
	set_blink(read_bool(CONFIG_BLINK, settings.blink));
	set_showtop(read_bool(CONFIG_SHOWTOP, settings.showtop));
	set_showbottom(read_bool(CONFIG_SHOWBOTTOM, settings.showbottom));

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

	/* prep clock, but if the phone launches us, display a message */
	if (launch_reason()!=APP_LAUNCH_PHONE) {
		t=time(NULL);
		now=localtime(&t);
	}
}

static void window_main_unload(Window *window)
{
	gbitmap_destroy(segs.seven.horizontal);
	gbitmap_destroy(segs.seven.vertical);
	gbitmap_destroy(segs.seven_dark.horizontal);
	gbitmap_destroy(segs.seven_dark.vertical);

	gbitmap_destroy(segs.fourteen.top);
	gbitmap_destroy(segs.fourteen.side);
	gbitmap_destroy(segs.fourteen.diagonal_left);
	gbitmap_destroy(segs.fourteen.diagonal_right);
	gbitmap_destroy(segs.fourteen.vertical);
	gbitmap_destroy(segs.fourteen.horizontal);

	gbitmap_destroy(segs.fourteen_dark.top);
	gbitmap_destroy(segs.fourteen_dark.side);
	gbitmap_destroy(segs.fourteen_dark.diagonal_left);
	gbitmap_destroy(segs.fourteen_dark.diagonal_right);
	gbitmap_destroy(segs.fourteen_dark.vertical);
	gbitmap_destroy(segs.fourteen_dark.horizontal);

	layer_destroy(time_layer);
	layer_destroy(colon_layer);
	layer_destroy(date_layer);
	layer_destroy(info_layer);
	layer_destroy(inverter_layer_get_layer(invert_layer));
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

	/* listen for ticks */
	tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

	/* listen for bluetooth updates */
	bluetooth_connection_service_subscribe(bluetooth_handler);
	settings.bluetooth=bluetooth_connection_service_peek();

	/* listen for battery state changes */
	battery_state_service_subscribe(battery_handler);

	BatteryChargeState state=battery_state_service_peek();
	charge=state.charge_percent;

	app_message_register_inbox_received(message_in_handler);
	app_message_open(64, 64);
}

static void deinit()
{
	window_destroy(window_main);
	app_message_deregister_callbacks();
	tick_timer_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
	battery_state_service_unsubscribe();
	free(now);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}

