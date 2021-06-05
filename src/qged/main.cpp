/*                        M A I N . C X X
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
/** @file main.cxx
 *
 * Command line parsing and main application launching for qbrlcad
 *
 */

#include <iostream>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QApplication>

#include "main_window.h"
#include "cadapp.h"
#include "cadcommands.h"

#include "bu/app.h"
#include "bu/log.h"
#include "brlcad_version.h"

int main(int argc, char *argv[])
{
    bu_setprogname(argv[0]);

    CADApp app(argc, argv);

    app.initialize();

    app.w = new BRLCAD_MainWindow();

    QCoreApplication::setApplicationName("BRL-CAD");
    QCoreApplication::setApplicationVersion(brlcad_version());

    QCommandLineParser parser;
    parser.setApplicationDescription("BRL-CAD - an Open Source Solid Modeling Computer Aided Design System");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption consoleOption(QStringList() << "c" << "console", "Run in command-line mode");
    parser.addOption(consoleOption);
    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (args.size() > 1) {
	bu_exit(1, "Error: Only one .g file at a time may be opened.");
    }

    if (args.size() == 1) {
	if (app.opendb(args.at(0))) {
	    fprintf(stderr, "%s%s%s\n", "Error: opening ", (const char *)args.at(0).toLocal8Bit(), " failed.");
	    exit(1);
	}
    }

    // TODO - this needs to be a setting that is saved and restored
    app.w->resize(1100, 800);

    // Set up the command prompt's commands
    cad_register_commands(&app);

    if (parser.isSet(consoleOption)) {
	bu_exit(1, "Console mode unimplemented\n");
    } else {
	app.w->show();
	return app.exec();
    }
}

/*
 * Local Variables:
 * mode: C++
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
