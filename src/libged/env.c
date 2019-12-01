/*                           E N V . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2019 United States Government as represented by
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
/** @file libged/env.c
 *
 * The env command.
 *
 * Interface for discovering and managing environment variables relevant
 * to the BRL-CAD libraries and programs.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <string.h>

#include "bu/env.h"
#include "bu/path.h"
#include "./ged_private.h"

#include "./env_cmd.c"

/**
 * Reports on and manipulates environment variables relevant to BRL-CAD.
 */
int
ged_env(struct ged *gedp, int argc, const char *argv[])
{
    int ret = GED_OK;

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    ret = env_cmd(gedp->ged_result_str, argc, argv);

    if (ret == 2) {
	return GED_HELP;
    }

    ret = (!ret) ? GED_OK : GED_ERROR;

    return ret;
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
