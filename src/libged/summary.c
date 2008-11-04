/*                         S U M M A R Y . C
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file summary.c
 *
 * The summary command.
 *
 */

#include "common.h"

#include <string.h>

#include "bio.h"
#include "cmd.h"
#include "ged_private.h"

static void
ged_dir_summary(struct ged	*gedp,
		int		flag);

int
ged_summary(struct ged *gedp, int argc, const char *argv[])
{
    register char *cp;
    int flags = 0;
    static const char *usage = "[p r g]";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    if (argc == 1) {
	ged_dir_summary(gedp, 0);
	return BRLCAD_OK;
    }

    if (2 < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    cp = (char *)argv[1];
    while (*cp)  switch (*cp++) {
	case 'p':
	    flags |= DIR_SOLID;
	    break;
	case 'r':
	    flags |= DIR_REGION;
	    break;
	case 'g':
	    flags |= DIR_COMB;
	    break;
	default:
	    bu_vls_printf(&gedp->ged_result_str, "%s:  p, r or g are the only valid parmaters\n", argv[0]);
	    return BRLCAD_ERROR;
    }

    ged_dir_summary(gedp, flags);
    return BRLCAD_OK;
}


/*
 *  			G E D _ D I R _ S U M M A R Y
 *
 * Summarize the contents of the directory by categories
 * (solid, comb, region).  If flag is != 0, it is interpreted
 * as a request to print all the names in that category (eg, DIR_SOLID).
 */
static void
ged_dir_summary(struct ged	*gedp,
		int		flag)
{
    register struct directory *dp;
    register int i;
    static int sol, comb, reg;
    struct directory **dirp;
    struct directory **dirp0 = (struct directory **)NULL;

    sol = comb = reg = 0;
    for (i = 0; i < RT_DBNHASH; i++)  {
	for (dp = gedp->ged_wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
	    if (dp->d_flags & DIR_SOLID)
		sol++;
	    if (dp->d_flags & DIR_COMB) {
		if (dp->d_flags & DIR_REGION)
		    reg++;
		else
		    comb++;
	    }
	}
    }

    bu_vls_printf(&gedp->ged_result_str, "Summary:\n");
    bu_vls_printf(&gedp->ged_result_str, "  %5d primitives\n", sol);
    bu_vls_printf(&gedp->ged_result_str, "  %5d region; %d non-region combinations\n", reg, comb);
    bu_vls_printf(&gedp->ged_result_str, "  %5d total objects\n\n", sol+reg+comb );

    if (flag == 0)
	return;

    /* Print all names matching the flags parameter */
    /* THIS MIGHT WANT TO BE SEPARATED OUT BY CATEGORY */

    dirp = ged_dir_getspace(gedp->ged_wdbp->dbip, 0);
    dirp0 = dirp;
    /*
     * Walk the directory list adding pointers (to the directory entries
     * of interest) to the array
     */
    for (i = 0; i < RT_DBNHASH; i++)
	for (dp = gedp->ged_wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw)
	    if (dp->d_flags & flag)
		*dirp++ = dp;

    ged_vls_col_pr4v(&gedp->ged_result_str, dirp0, (int)(dirp - dirp0), 0);
    bu_free((genptr_t)dirp0, "dir_getspace");
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
