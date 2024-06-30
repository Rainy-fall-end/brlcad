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
#include "rt/torch_runner.h"
int main(int argc, char* argv[])
{
	const char* db = "C:\\works\\soc\\rainy\\brlcad\\build\\share\\db\\moss.g";
	const char* ob = "all.g";
	struct rt_i* rtip = NULL;
	set_size(128);
	rt_tool::init_rt(db, ob, rtip);
#if 1
	set_model_path("C:\\works\\soc\\rainy\\Rendernn\\model2.pt");
	rt_neu::render();
#endif

# if 0
	auto ray_list = rt_sample::RangeFixVec(1000000, 400, -100, { -0.742403865,-0.519836783,-0.422618270 });
	auto ray_res = rt_tool::ShootSamples(ray_list);
	// test for write json
	util::write_json(ray_list, ray_res, "C:\\works\\soc\\rainy\\neural\\c.json");
#endif
	return 0;
}