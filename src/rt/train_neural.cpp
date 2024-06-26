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
#include"rt/rt_trainer.h"
int main(int argc, char* argv[])
{
	const char* db = "C:\\works\\soc\\rainy\\brlcad\\build\\share\\db\\moss.g";
	const char* ob = "all.g";
	struct rt_i* rtip = NULL;
	rt_tool::init_rt(db, ob, rtip);
	// test for do_ray
	point_t rt_p1 = { 232.98318264408485, 17.129209479118760, -13.872954520341105 };
	vect_t rt_r1 = { -0.74240387650610373, -0.51983679072568467, -0.42261826174069961 };
	point_t rt_p2 = { 151.95860058527862, 111.99263490737854, 11.775405151166122 };
	vect_t rt_r2 = { -0.74240387650610373, -0.51983679072568467, -0.42261826174069961 };
	RGBpixel res;
	// res should be 0 0 1
	rt_tool::do_ray(rt_p1, rt_r1, res);
	// res should be 23 44 68
	rt_tool::do_ray(rt_p2, rt_r2, res);
	// test for shoot_sample
	auto ray_list = rt_sample::SampleSphere(50);
	auto ray_res = rt_tool::ShootSamples(ray_list);
	return 0;
}
