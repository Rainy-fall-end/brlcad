/*
 *			L I B F B - D U M M Y . C
 */

#include "common.h"



#include <stdio.h>

#include "machine.h"
#include "fb.h"

FBIO *
fb_open( name, w, h )
char *name;
{
	return(FBIO_NULL);
}

int
fb_close() {
	return(0);
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
