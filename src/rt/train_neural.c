/*                          T R A I N _ N E U R A L. C P P
 * BRL-CAD
 *
 * Copyright (c) 1985-2024 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
 /** @file rt/train_neural.cpp
  *
  *
  */

#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <math.h>

#ifdef MPI_ENABLED
#  include <mpi.h>
#endif

#include "bio.h"

#include "bu/app.h"
#include "bu/bitv.h"
#include "bu/debug.h"
#include "bu/endian.h"
#include "bu/getopt.h"
#include "bu/log.h"
#include "bu/malloc.h"
#include "bu/parallel.h"
#include "bu/ptbl.h"
#include "bu/version.h"
#include "bu/vls.h"
#include "vmath.h"
#include "raytrace.h"
#include "dm.h"
#include "pkg.h"

  /* private */
#include "./rtuif.h"
#include "./ext.h"
#include "brlcad_ident.h"


extern void application_init(void);

extern const char title[];


/***** Variables shared with viewing model *** */
struct fb* fbp = FB_NULL;	/* Framebuffer handle */
FILE* outfp = NULL;		/* optional pixel output file */
struct icv_image* bif = NULL;

/***** end of sharing with viewing model *****/


/***** variables shared with worker() ******/
struct application APP;
int		report_progress;	/* !0 = user wants progress report */
extern int	incr_mode;		/* !0 for incremental resolution */
extern size_t	incr_nlevel;		/* number of levels */
/***** end variables shared with worker() *****/


/***** variables shared with do.c *****/
extern int	pix_start;		/* pixel to start at */
extern int	pix_end;		/* pixel to end at */
size_t		n_malloc;		/* Totals at last check */
size_t		n_free;
size_t		n_realloc;
extern int	matflag;		/* read matrix from stdin */
extern int	orientflag;		/* 1 means orientation has been set */
extern int	desiredframe;		/* frame to start at */
extern int	curframe;		/* current frame number,
					 * also shared with view.c */
extern char* outputfile;		/* name of base of output file */

void
memory_summary(void)
{
	if (rt_verbosity & VERBOSE_STATS) {
		size_t mdelta = bu_n_malloc - n_malloc;
		size_t fdelta = bu_n_free - n_free; 
		bu_log("Additional #malloc=%zu, #free=%zu, #realloc=%zu (%zu retained)\n",
			mdelta,
			fdelta,
			bu_n_realloc - n_realloc,
			mdelta - fdelta);
	}
	n_malloc = bu_n_malloc;
	n_free = bu_n_free;
	n_realloc = bu_n_realloc;
}

int main(int argc, char* argv[])
{
	return 0;
}
