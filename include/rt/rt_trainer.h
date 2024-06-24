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
#include<dm.h>

static int
hit_nothing(struct application* ap);

int
colorview(struct application* ap, struct partition* PartHeadp, struct seg* finished_segs);

static struct partition*
do_kut(const struct application* ap, struct partition* pp, struct partition* PartHeadp);

void
ambientOcclusion(struct application* ap, struct partition* pp);

int
ao_rayhit(register struct application* ap,
	struct partition* PartHeadp,
	struct seg* UNUSED(segp));

int
ao_raymiss(register struct application* ap);

typedef std::vector<std::pair< pointp_t, pointp_t>> RayParam;
class TrainData
{
public:
	TrainData(struct rt_i* rtip);
	TrainData(const char* database_name, const char* object_name);
	std::vector<RGBpixel> ShootSamples(const RayParam& ray_list);
	void ClearRes();
	~TrainData();
private:
	struct rt_i* m_rt_i;
	std::vector<bool> m_res;
};

void create_plot(const char* db_name, const RayParam& rays);


#endif // !RT_RT_TRAINER_H