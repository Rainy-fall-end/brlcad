/*                    B O T _ E X T E R I O R . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2021 United States Government as represented by
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
/** @file libged/bot_exterior.c
 *
 * The bot exterior subcommand.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "rt/geom.h"

#include "../ged_private.h"


/* determine whether a given face is exterior or not */
static int
exterior_face(struct rt_bot_internal *bot, int face) {
    if (!bot || face < 0)
	return 0;

    if (face < 10)
	return 1;
    return 0;
}


static size_t
bot_exterior(struct rt_bot_internal *bot)
{
    if (!bot)
	return 0;
    RT_BOT_CK_MAGIC(bot);

    size_t i;
    size_t num_exterior = 0;
    int *faces;
    faces = (int *)bu_calloc(bot->num_faces, sizeof(int), "rt_bot_exterior: faces");

    /* walk the list of faces, and mark each one if it is exterior */
    for (i = 0; i < bot->num_faces; i++) {
	if (exterior_face(bot, i)) {
	    faces[i] = 1;
	    num_exterior++;
	}
    }

    /* sanity */
    if (num_exterior == 0) {
	bu_log("No interior faces??  Aborting.\n");
	return 0;
    }

    /* rebuild list of faces, copying over ones marked exterior */
    int j = 0;
    int *newfaces = (int *)bu_calloc(num_exterior, 3*sizeof(int), "new bot faces");
    for (i = 0; i < bot->num_faces; i++) {
	if (faces[i]) {
	    VMOVE(&newfaces[j*3], &bot->faces[i*3]);
	    j++;
	}
    }

    bu_free((char *)faces, "rt_bot_exterior: faces");

    if (num_exterior == bot->num_faces)
	return 0;

    int removed = bot->num_faces - num_exterior;
    bot->num_faces = num_exterior;
    bu_free(bot->faces, "release bot faces");
    bot->faces = newfaces;

    return removed;
}


int
ged_bot_exterior(struct ged *gedp, int argc, const char *argv[])
{
    struct directory *old_dp, *new_dp;
    struct rt_db_internal intern;
    struct rt_bot_internal *bot;
    size_t fcount = 0;
    size_t vcount = 0;
    static const char *usage = "old_bot new_bot";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_READ_ONLY(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 3) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    GED_DB_LOOKUP(gedp, old_dp, argv[1], LOOKUP_NOISY, BRLCAD_ERROR & BRLCAD_QUIET);
    GED_DB_GET_INTERNAL(gedp, &intern,  old_dp, bn_mat_identity, &rt_uniresource, BRLCAD_ERROR);

    if (intern.idb_major_type != DB5_MAJORTYPE_BRLCAD || intern.idb_minor_type != DB5_MINORTYPE_BRLCAD_BOT) {
	bu_vls_printf(gedp->ged_result_str, "%s: %s is not a BOT solid!\n", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    bot = (struct rt_bot_internal *)intern.idb_ptr;
    RT_BOT_CK_MAGIC(bot);

    if (bot->mode == RT_BOT_PLATE || bot->mode == RT_BOT_PLATE_NOCOS) {
	bu_log("%s: %s is a PLATE MODE BoT\n"
	       "Calculating exterior faces currently unsupported for PLATE MODE\n", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    fcount = bot_exterior(bot);
    vcount = rt_bot_condense(bot);

    bu_vls_printf(gedp->ged_result_str, "%s: %zu interior vertices eliminated\n", argv[0], vcount);
    bu_vls_printf(gedp->ged_result_str, "%s: %zu interior faces eliminated\n", argv[0], fcount);

    /* FIXME: if the BoT is not SOLID, create as PLATE instead */

    GED_DB_DIRADD(gedp, new_dp, argv[2], RT_DIR_PHONY_ADDR, 0, RT_DIR_SOLID, (void *)&intern.idb_type, BRLCAD_ERROR);
    GED_DB_PUT_INTERNAL(gedp, new_dp, &intern, &rt_uniresource, BRLCAD_ERROR);

    return BRLCAD_OK;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
