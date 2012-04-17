/*                 MassSiUnit.h
 * BRL-CAD
 *
 * Copyright (c) 1994-2012 United States Government as represented by
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
/** @file step/MassSiUnit.h
 *
 * Class definition used to convert STEP "MassSiUnit" to BRL-CAD BREP
 * structures.
 *
 */

#ifndef MASSSIUNIT_H_
#define MASSSIUNIT_H_

#include "MassUnit.h"
#include "SiUnit.h"

class MassSiUnit : public MassUnit, public SiUnit {
private:
	static string entityname;

protected:

public:
	MassSiUnit();
	virtual ~MassSiUnit();
	MassSiUnit(STEPWrapper *sw,int step_id);
	bool Load(STEPWrapper *sw,SDAI_Application_instance *sse);
	virtual void Print(int level);

	//static methods
	static STEPEntity *Create(STEPWrapper *sw,SDAI_Application_instance *sse);
};

#endif /* MASSSIUNIT_H_ */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
