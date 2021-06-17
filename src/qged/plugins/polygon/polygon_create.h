/*         P O L Y G O N _ C R E A T E . H
 * BRL-CAD
 *
 * Copyright (c) 2014-2021 United States Government as represented by
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
/** @file polygon_create.h
 *
 */

#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QPushButton>
#include <QRadioButton>
#include "bg/polygon_types.h"
#include "qtcad/QColorRGB.h"

class QPolyCreate : public QWidget
{
    Q_OBJECT

    public:
	QPolyCreate();
	~QPolyCreate();

	// Boolean Operation Mode
	QComboBox *csg_modes;

	// Adding polygons
	QLineEdit *view_name;
	QRadioButton *circle_mode;
	QRadioButton *ellipse_mode;
	QRadioButton *square_mode;
	QRadioButton *rectangle_mode;
	QRadioButton *general_mode;

	// Draw default settings
	// Default edge color
	QCheckBox *default_fill_poly;
	QColorRGB *default_fill_color;
	QColorRGB *default_edge_color;
	// TODO - maybe use sliders here?
	QLineEdit *default_fill_angle;
	QLineEdit *default_fill_density;

	// Modifying polygons
	QCheckBox *close_general_poly;

    signals:
	void view_updated(struct bview **);

    private slots:
	void toplevel_config(bool);
	void finalize(bool);

    protected:
	bool eventFilter(QObject *, QEvent *);

    private:
	bg_clip_t op = bg_Union;
	int poly_cnt = 0;
	struct bv_scene_obj *p = NULL;
	bool do_bool = false;
};


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
