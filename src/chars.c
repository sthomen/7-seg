#include <pebble.h>
#include "debug.h"
#include "numbers.h"

char *get_number(char which)
{
	int i;

	for (i=0;i<SEGVII_MAX;i++) {
		if (which == numbers[i].character) {
			return (char *)numbers[i].data;
		}
	}
	return (char *)NULL;
}

