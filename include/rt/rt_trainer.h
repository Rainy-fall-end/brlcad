/*                    R T _ T R A I N E R . H
 * BRL-CAD
 *
 * Copyright (c) 1993-2024 United States Government as represented by
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
#ifndef RT_RT_TRAINER_H
#define RT_RT_TRAINER_H

#include<vmath.h>
#include<vector>
#include<array>
#include<dm.h>
#include "nlohmann/json.hpp"
extern "C"
{
	extern int curframe;		/* from main.c */
	extern double airdensity;	/* from opt.c */
	extern double haze[3];		/* from opt.c */
	extern int do_kut_plane;        /* from opt.c */
	extern plane_t kut_plane;       /* from opt.c */
	extern struct fb* fbp;
	extern fastf_t	rt_dist_tol;		/* Value for rti_tol.dist */
	extern fastf_t	rt_perp_tol;		/* Value for rti_tol.perp */
	/***** variables from neural.c *****/
	extern int view_init(struct application* ap, char* file, char* obj, int minus_o, int minus_F);
	extern int ao_raymiss(register struct application* ap);
	extern int ao_rayhit(register struct application* ap,
		struct partition* PartHeadp,
		struct seg* UNUSED(segp));
	extern void
		ambientOcclusion(struct application* ap, struct partition* pp);
	extern int
		colorview(struct application* ap, struct partition* PartHeadp, struct seg* finished_segs);
	extern void
		application_init(void);
	/***** variables from do.c *****/
	extern void
		do_prep(struct rt_i* rtip);
	extern int
		do_frame(int framenumber);
	/***** variables from neural.c *****/
	extern int fb_setup(void);
	extern void
		initialize_option_defaults(void);
	extern void
		initialize_resources(size_t cnt, struct resource* resp, struct rt_i* rtip);
	extern void
		view_2init(struct application* ap, char* UNUSED(framename));
	extern void set_size(int size);
}
const std::string global_model_path;

using RayParam = std::vector<std::pair< std::vector<fastf_t>, std::vector<fastf_t>>>;
using RGBdata = std::array<int, 3>;
using Rayres = std::vector<RGBdata>;
using json = nlohmann::json;

namespace rt_tool
{
	extern "C"
	{
		extern void do_ray(point_t start, vect_t dir, RGBpixel rgb);
	}
	void init_rt(const char* database_name, const char* object_name, struct rt_i* rtip);
	Rayres ShootSamples(const RayParam& ray_list);
}

namespace util
{
	// write a mged script
	void create_plot(const char* db_name, const RayParam& rays, const char* plot_name);
	// convert data from RGBpixel to RGBData
	RGBdata pix_to_rgb(const RGBpixel data);
	void write_json(const RayParam& para, const Rayres& res, const char* path);
}

namespace rt_sample
{
	// generate a random number between begin and end
	double RandomNum(double begin, double end);
	// generate random datas randomly
	RayParam SampleRandom(size_t num);
	// generate random datas randomly within a sphere
	RayParam SampleSphere(size_t num);
	// generate uniform datas within on a sphere
	RayParam UniformSphere(size_t num);
	RayParam RangeFixVec(size_t num,fastf_t max, fastf_t min, std::vector<fastf_t> vec);
}
namespace rt_neu
{
	// begin neural rendering
	void render();
}
#endif // !RT_RT_TRAINER_H