/*                           D X F . H
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
/** @file dxf.h
 *
 * Structures and data common to the DXF importer and exporter.
 *
 */

#ifndef CONV_DXF_DXF_H
#define CONV_DXF_DXF_H

static unsigned char rgb[]={
    0, 0, 0,
    255, 0, 0,
    255, 255, 0,
    0, 255, 0,
    0, 255, 255,
    0, 0, 255,
    255, 0, 255,
    255, 255, 255,
    65, 65, 65,
    128, 128, 128,
    255, 0, 0,
    255, 128, 128,
    166, 0, 0,
    166, 83, 83,
    128, 0, 0,
    128, 64, 64,
    77, 0, 0,
    77, 38, 38,
    38, 0, 0,
    38, 19, 19,
    255, 64, 0,
    255, 159, 128,
    166, 41, 0,
    166, 104, 83,
    128, 32, 0,
    128, 80, 64,
    77, 19, 0,
    77, 48, 38,
    38, 10, 0,
    38, 24, 19,
    255, 128, 0,
    255, 191, 128,
    166, 83, 0,
    166, 124, 83,
    128, 64, 0,
    128, 96, 64,
    77, 38, 0,
    77, 57, 38,
    38, 19, 0,
    38, 29, 19,
    255, 191, 0,
    255, 223, 128,
    166, 124, 0,
    166, 145, 83,
    128, 96, 0,
    128, 112, 64,
    77, 57, 0,
    77, 67, 38,
    38, 29, 0,
    38, 33, 19,
    255, 255, 0,
    255, 255, 128,
    166, 166, 0,
    166, 166, 83,
    128, 128, 0,
    128, 128, 64,
    77, 77, 0,
    77, 77, 38,
    38, 38, 0,
    38, 38, 19,
    191, 255, 0,
    223, 255, 128,
    124, 166, 0,
    145, 166, 83,
    96, 128, 0,
    112, 128, 64,
    57, 77, 0,
    67, 77, 38,
    29, 38, 0,
    33, 38, 19,
    128, 255, 0,
    191, 255, 128,
    83, 166, 0,
    124, 166, 83,
    64, 128, 0,
    96, 128, 64,
    38, 77, 0,
    57, 77, 38,
    19, 38, 0,
    29, 38, 19,
    64, 255, 0,
    159, 255, 128,
    41, 166, 0,
    104, 166, 83,
    32, 128, 0,
    80, 128, 64,
    19, 77, 0,
    48, 77, 38,
    10, 38, 0,
    24, 38, 19,
    0, 255, 0,
    128, 255, 128,
    0, 166, 0,
    83, 166, 83,
    0, 128, 0,
    64, 128, 64,
    0, 77, 0,
    38, 77, 38,
    0, 38, 0,
    19, 38, 19,
    0, 255, 64,
    128, 255, 159,
    0, 166, 41,
    83, 166, 104,
    0, 128, 32,
    64, 128, 80,
    0, 77, 19,
    38, 77, 48,
    0, 38, 10,
    19, 38, 24,
    0, 255, 128,
    128, 255, 191,
    0, 166, 83,
    83, 166, 124,
    0, 128, 64,
    64, 128, 96,
    0, 77, 38,
    38, 77, 57,
    0, 38, 19,
    19, 38, 29,
    0, 255, 191,
    128, 255, 223,
    0, 166, 124,
    83, 166, 145,
    0, 128, 96,
    64, 128, 112,
    0, 77, 57,
    38, 77, 67,
    0, 38, 29,
    19, 38, 33,
    0, 255, 255,
    128, 255, 255,
    0, 166, 166,
    83, 166, 166,
    0, 128, 128,
    64, 128, 128,
    0, 77, 77,
    38, 77, 77,
    0, 38, 38,
    19, 38, 38,
    0, 191, 255,
    128, 223, 255,
    0, 124, 166,
    83, 145, 166,
    0, 96, 128,
    64, 112, 128,
    0, 57, 77,
    38, 67, 77,
    0, 29, 38,
    19, 33, 38,
    0, 128, 255,
    128, 191, 255,
    0, 83, 166,
    83, 124, 166,
    0, 64, 128,
    64, 96, 128,
    0, 38, 77,
    38, 57, 77,
    0, 19, 38,
    19, 29, 38,
    0, 64, 255,
    128, 159, 255,
    0, 41, 166,
    83, 104, 166,
    0, 32, 128,
    64, 80, 128,
    0, 19, 77,
    38, 48, 77,
    0, 10, 38,
    19, 24, 38,
    0, 0, 255,
    128, 128, 255,
    0, 0, 166,
    83, 83, 166,
    0, 0, 128,
    64, 64, 128,
    0, 0, 77,
    38, 38, 77,
    0, 0, 38,
    19, 19, 38,
    64, 0, 255,
    159, 128, 255,
    41, 0, 166,
    104, 83, 166,
    32, 0, 128,
    80, 64, 128,
    19, 0, 77,
    48, 38, 77,
    10, 0, 38,
    24, 19, 38,
    128, 0, 255,
    191, 128, 255,
    83, 0, 166,
    124, 83, 166,
    64, 0, 128,
    96, 64, 128,
    38, 0, 77,
    57, 38, 77,
    19, 0, 38,
    29, 19, 38,
    191, 0, 255,
    223, 128, 255,
    124, 0, 166,
    145, 83, 166,
    96, 0, 128,
    112, 64, 128,
    57, 0, 77,
    67, 38, 77,
    29, 0, 38,
    33, 19, 38,
    255, 0, 255,
    255, 128, 255,
    166, 0, 166,
    166, 83, 166,
    128, 0, 128,
    128, 64, 128,
    77, 0, 77,
    77, 38, 77,
    38, 0, 38,
    38, 19, 38,
    255, 0, 191,
    255, 128, 223,
    166, 0, 124,
    166, 83, 145,
    128, 0, 96,
    128, 64, 112,
    77, 0, 57,
    77, 38, 67,
    38, 0, 29,
    38, 19, 33,
    255, 0, 128,
    255, 128, 191,
    166, 0, 83,
    166, 83, 124,
    128, 0, 64,
    128, 64, 96,
    77, 0, 38,
    77, 38, 57,
    38, 0, 19,
    38, 19, 29,
    255, 0, 64,
    255, 128, 159,
    166, 0, 41,
    166, 83, 104,
    128, 0, 32,
    128, 64, 80,
    77, 0, 19,
    77, 38, 48,
    38, 0, 10,
    38, 19, 24,
    84, 84, 84,
    118, 118, 118,
    152, 152, 152,
    187, 187, 187,
    221, 221, 221,
    255, 255, 255
};

#endif /* CONV_DXF_DXF_H */


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */