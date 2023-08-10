/*                       E I G E N . C P P
 * BRL-CAD
 *
 * Copyright (c) 2023 United States Government as represented by
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
/** @file eigen.cpp
 *
 * The primary purpose of this test is to use Eigen's Map capability
 * to work with libbn/vmath data.
 */

#include "common.h"
#include <limits>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <Eigen/Geometry>
#include "bu/app.h"
#include "bu/exit.h"
#include "bn/mat.h"

int main(int UNUSED(ac), const char **av)
{
    bu_setprogname(av[0]);

    // Set up a vmath zero matrix without reference to Eigen
    mat_t bn_mat;
    MAT_ZERO(bn_mat);
    bn_mat_print("Initial Zero Matrix", bn_mat);

    // Establish an Eigen map to the vmath mat_t storage
    Eigen::Map<Eigen::Matrix<fastf_t, 4, 4>> emat(bn_mat);

    // Set the matrix to identity in Eigen
    emat = Eigen::Matrix<fastf_t, 4, 4>::Identity();

    // Verify (independent of Eigen) that the libbn data received the update
    if (!bn_mat_is_identity(bn_mat))
	bu_exit(BRLCAD_ERROR, "Identity matrix assignment failed\n");
    bn_mat_print("Identity Matrix", bn_mat);

    return 0;
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
