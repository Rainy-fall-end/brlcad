/*
 *  			M A T E R . C
 *  
 *  Code to deal with establishing and maintaining the tables which
 *  map region ID codes into worthwhile material information
 *  (colors and outboard database "handles").
 *
 *  Functions -
 *	color_addrec	Called by dir_build on startup
 *	color_map	Map one region reference to a material
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "debug.h"
#include "../h/machine.h"
#include "../h/vmath.h"
#include "../h/db.h"
#include "../h/mater.h"
#include "rtdir.h"
#include "raytrace.h"

/*
 *  It is expected that entries on this mater list will be sorted
 *  in strictly ascending order, with no overlaps (ie, monotonicly
 * increasing).
 */
struct mater *MaterHead = MATER_NULL;

void color_addrec();
static void insert_color();

static void
pr_mater( mp )
register struct mater *mp;
{
	(void)fprintf(stderr, "%5d..%d\t", mp->mt_low, mp->mt_high );
	(void)fprintf(stderr, "%d,%d,%d\t", mp->mt_r, mp->mt_g, mp->mt_b);
	(void)fprintf(stderr, "%s\n", mp->mt_handle );
}

/*
 *  			C O L O R _ A D D R E C
 *  
 *  Called from dir_build() when initially scanning database.
 */
void
color_addrec( recp, addr )
union record *recp;
long addr;
{
	register struct mater *mp;

	GETSTRUCT( mp, mater );
	mp->mt_low = recp->md.md_low;
	mp->mt_high = recp->md.md_hi;
	mp->mt_r = recp->md.md_r;
	mp->mt_g = recp->md.md_g;
	mp->mt_b = recp->md.md_b;
/*	mp->mt_handle = strdup( recp->md.md_material ); */
	mp->mt_daddr = addr;
	insert_color( mp );
}

/*
 *  			I N S E R T _ C O L O R
 *
 *  While any additional database records are created and written here,
 *  it is the responsibility of the caller to color_putrec(newp) if needed.
 */
static void
insert_color( newp )
register struct mater *newp;
{
	register struct mater *mp;
	register struct mater *zot;

	if( MaterHead == MATER_NULL || newp->mt_high < MaterHead->mt_low )  {
		/* Insert at head of list */
		newp->mt_forw = MaterHead;
		MaterHead = newp;
		return;
	}
	if( newp->mt_low < MaterHead->mt_low )  {
		/* Insert at head of list, check for redefinition */
		newp->mt_forw = MaterHead;
		MaterHead = newp;
		goto check_overlap;
	}
	for( mp = MaterHead; mp != MATER_NULL; mp = mp->mt_forw )  {
		if( mp->mt_low == newp->mt_low  &&
		    mp->mt_high <= newp->mt_high )  {
			(void)fprintf(stderr,"dropping overwritten entry:\n");
			newp->mt_forw = mp->mt_forw;
			pr_mater( mp );
			*mp = *newp;		/* struct copy */
			free( newp );
			newp = mp;
			goto check_overlap;
		}
		if( mp->mt_low  < newp->mt_low  &&
		    mp->mt_high > newp->mt_high )  {
			/* New range entirely contained in old range; split */
			(void)fprintf(stderr,"Splitting into 3 ranges\n");
			GETSTRUCT( zot, mater );
			*zot = *mp;		/* struct copy */
			zot->mt_daddr = MATER_NO_ADDR;
			/* zot->mt_high = mp->mt_high; */
			zot->mt_low = newp->mt_high+1;
			mp->mt_high = newp->mt_low-1;
			/* mp, newp, zot */
			/* zot->mt_forw = mp->mt_forw; */
			newp->mt_forw = zot;
			mp->mt_forw = newp;
			pr_mater( mp );
			pr_mater( newp );
			pr_mater( zot );
			return;
		}
		if( mp->mt_high > newp->mt_low )  {
			/* Overlap to the left: Shorten preceeding entry */
			(void)fprintf(stderr,"Shortening lhs range, from:\n");
			pr_mater( mp );
			(void)fprintf(stderr,"to:\n");
			mp->mt_high = newp->mt_low-1;
			pr_mater( mp );
			/* Now append */
			newp->mt_forw = mp->mt_forw;
			mp->mt_forw = newp;
			goto check_overlap;
		}
		if( mp->mt_forw == MATER_NULL ||
		    newp->mt_low < mp->mt_forw->mt_low )  {
			/* Append */
			newp->mt_forw = mp->mt_forw;
			mp->mt_forw = newp;
			goto check_overlap;
		}
	}
	(void)fprintf(stderr,"fell out of insert_color loop, append to end\n");
	/* Append at end */
	newp->mt_forw = MATER_NULL;
	mp->mt_forw = newp;
	return;

	/* Check for overlap, ie, redefinition of following colors */
check_overlap:
	while( newp->mt_forw != MATER_NULL &&
	       newp->mt_high >= newp->mt_forw->mt_low )  {
		if( newp->mt_high >= newp->mt_forw->mt_high )  {
			/* Drop this mater struct */
			zot = newp->mt_forw;
			newp->mt_forw = zot->mt_forw;
			(void)fprintf(stderr,"dropping overlaping entry:\n");
			pr_mater( zot );
			free( zot );
			continue;
		}
		if( newp->mt_high >= newp->mt_forw->mt_low )  {
			/* Shorten this mater struct, then done */
			(void)fprintf(stderr,"Shortening rhs range, from:\n");
			pr_mater( newp->mt_forw );
			(void)fprintf(stderr,"to:\n");
			newp->mt_forw->mt_low = newp->mt_high+1;
			pr_mater( newp->mt_forw );
			continue;	/* more conservative than returning */
		}
	}
}

static struct mater default_mater = {
	0, 32767,
	99,
	255, 0, 0,		/* red */
	"{default mater}",
	MATER_NO_ADDR, 0
};

/*
 *  			C O L O R _ M A P
 *
 *  Map one region description into a material description
 *  mater structure.
 */
void
color_map( regp )
register struct region *regp;
{
	register struct mater *mp;

	if( regp == REGION_NULL )  {
		fprintf(stderr,"color_map(NULL)\n");
		return;
	}
	for( mp = MaterHead; mp != MATER_NULL; mp = mp->mt_forw )  {
		if( regp->reg_regionid <= mp->mt_high &&
		    regp->reg_regionid >= mp->mt_low ) {
			regp->reg_materp = (char *)mp;
			return;
		}
	}
	regp->reg_materp = (char *)&default_mater;
}
