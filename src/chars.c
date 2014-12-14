#include <pebble.h>
#include "debug.h"
#include "7seg.h"
#include "14seg.h"

char *get_7seg(char which)
{
	int i;

	for (i=0;i<SEGVII_MAX;i++) {
		if (which == segment_7[i].character) {
			return (char *)segment_7[i].data;
		}
	}
	return (char *)NULL;
}

char *get_14seg(char which)
{
	int i;

	for (i=0;i<SEGXIV_MAX;i++) {
		if (which == segment_14[i].character) {
			return (char *)segment_14[i].data;
		}
	}
	return (char *)NULL;
}

