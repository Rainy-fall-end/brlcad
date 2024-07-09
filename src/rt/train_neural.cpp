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

#include "rt/neu_util.h"
#include"rt/rt_trainer.h"
#include "rt/torch_runner.h"
#include "vmath.h"
render_type rt_render_type;
int main(int argc, char* argv[])
{
	// database name
	const char* db = "C:\\works\\soc\\rainy\\brlcad\\build\\share\\db\\moss.g";
	// object name
	const char* ob = "all.g";
	struct rt_i* rtip = NULL;
	// set size
	set_size(64);
	rt_tool::init_rt(db, ob, rtip);
	// do_ae(30, 25);
	// rt_perspective = 120;

//rendering 
#if 1
	// choose rendering type:normal , neu_coordinate(Cartesian coordinate) neu_sphere
	set_type(neu_sphere);
	// choose model's input number: 2 for pos, 4 for both pos and dir
	set_model_type(2);
	// choose your model type
	set_model_path("C:\\works\\soc\\rainy\\Rendernn\\models\\model_sph_grid6.pt");
	// set_model_path("C:\\works\\soc\\rainy\\Rendernn\\models\\dir_model_sph_grid2.pt");
	// set_model_path("C:\\works\\soc\\rainy\\Rendernn\\models\\test.pt");
	// begin rendering
	rt_neu::render();
#endif

//sampling
# if 0
	// there are many different sample methods:
	// auto ray_list = rt_sample::RangeFixVec(100000, 400, -100, { -0.742403865,-0.519836783,-0.422618270 });
	// auto ray_list = rt_sample::SampleRandom(1000);
	// auto ray_list = rt_sample::SampleFixVecHit(100000, { -0.74240387650610373, -0.51983679072568467, -0.42261826174069961 });
	// auto ray_list = rt_sample::RangeFixVecHit(1000, 100,-100,{ -0.74240387650610373, -0.51983679072568467, -0.42261826174069961 });
	// auto ray_list = rt_sample::SampleFixVecHit(1000000, { -0.74240387650610373, -0.51983679072568467, -0.42261826174069961 });
	auto ray_list = rt_sample::RangeHit(1000,M_PI,0.75*M_PI);
	point_t center{ 0 };
	get_center(center);
	// shoot these points
	auto ray_res = rt_tool::ShootSamples(ray_list);
	// convert Cartesian coordinate to Spherical coordinate
	auto ray_list_sph = convert::cert_to_sph(ray_list, center, get_r());
	// test for write json
	// util::write_json(ray_list, ray_res, "C:\\works\\soc\\rainy\\Rendernn\\datas\\e.json");
	ray_list.clear();
	// write to a local file
	util::write_sph_json(ray_list_sph, ray_res, "C:\\works\\soc\\rainy\\Rendernn\\datas\\all_dir_sph_range_3.json");
#endif
	return 0;
}