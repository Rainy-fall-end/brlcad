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

	TrainData td(db, ob);
	vect_t dir = { 1,0,0 };
	
	point_t p1 = { 1,1,1 };

	point_t p2 = { 1,2,1 };

	point_t p3 = { 1,1,2 };

	RayParam test;
	/*test.push_back(std::make_pair(p1, dir));

	test.push_back(std::make_pair(p2, dir));

	test.push_back(std::make_pair(p3, dir));*/
	// test = rt_sample::SampleRandom(10);

	test = rt_sample::UniformSphere(100);
	create_plot("moss", test,"all_points1.g");
	return 0;
}
