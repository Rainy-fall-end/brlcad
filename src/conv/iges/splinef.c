/*                       S P L I N E F . C
 * BRL-CAD
 *
 * Copyright (c) 1990-2008 United States Government as represented by
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
/** @file splinef.c
 *  Authors -
 *	John R. Anderson
 *	Susanne L. Muuss
 *	Earl P. Weaver
 *
 */

/* spline function */

#include "common.h"

#include <stdio.h>

#include "machine.h"

fastf_t
splinef( c, s )
fastf_t c[4], s;
{
	int i;
	float retval;
	double stopow=1.0;

	retval = c[0];
	for( i=1 ; i<4 ; i++ )
	{
		stopow *= s;
		if( c[i] != 0.0 )
			retval += c[i]*stopow;
	}

	return( retval );
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
