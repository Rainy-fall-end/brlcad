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
#include "bu/cv.h"
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
/***** end variables shared with do.c *****/

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

int fb_setup(void) {
	/* Framebuffer is desired */
	size_t xx, yy;
	int zoom;

	/* make sure width/height are set via -g/-G */
	grid_sync_dimensions(viewsize);

	/* Ask for a fb big enough to hold the image, at least 512. */
	/* This is so MGED-invoked "postage stamps" get zoomed up big
	 * enough to see.
	 */
	xx = yy = 512;
	if (xx < width || yy < height) {
		xx = width;
		yy = height;
	}

	bu_semaphore_acquire(BU_SEM_SYSCALL);
	fbp = fb_open(framebuffer, xx, yy);
	bu_semaphore_release(BU_SEM_SYSCALL);
	if (fbp == FB_NULL) {
		fprintf(stderr, "rt:  can't open frame buffer\n");
		return 12;
	}

	bu_semaphore_acquire(BU_SEM_SYSCALL);
	/* If fb came out smaller than requested, do less work */
	size_t fbwidth = (size_t)fb_getwidth(fbp);
	size_t fbheight = (size_t)fb_getheight(fbp);
	if (width > fbwidth)
		width = fbwidth;
	if (height > fbheight)
		height = fbheight;

	/* If fb is lots bigger (>= 2X), zoom up & center */
	if (width > 0 && height > 0) {
		zoom = fbwidth / width;
		if (fbheight / height < (size_t)zoom) {
			zoom = fb_getheight(fbp) / height;
		}
		(void)fb_view(fbp, width / 2, height / 2, zoom, zoom);
	}
	bu_semaphore_release(BU_SEM_SYSCALL);

#ifdef USE_OPENCL
	clt_connect_fb(fbp);
#endif
	return 0;
}





