/*                 M A I N _ W I N D O W . C P P
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
/** @file main_window.cpp
 *
 */

#include <QSplitter>
#include "bview/util.h"
#include "main_window.h"
#include "dmapp.h"
#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#include "xxhash.h"

DM_MainWindow::DM_MainWindow()
{
    // This solves the disappearing menubar problem on Ubuntu + fluxbox -
    // suspect Unity's "global toolbar" settings are being used even when
    // the Qt app isn't being run under unity - this is probably a quirk
    // of this particular setup, but it sure is an annoying one...
    menuBar()->setNativeMenuBar(false);

    // Create Menus
    file_menu = menuBar()->addMenu("File");
    g_open = new QAction("Open", this);
    connect(g_open, &QAction::triggered, this, &DM_MainWindow::open_file);
    file_menu->addAction(g_open);

    g_save_image = new QAction("Save Image", this);
    connect(g_save_image, &QAction::triggered, this, &DM_MainWindow::save_image);
    file_menu->addAction(g_save_image);

    g_save_settings = new QAction("Save Settings", this);
    connect(g_save_settings, &QAction::triggered, this, &DM_MainWindow::write_settings);
    file_menu->addAction(g_save_settings);

    g_exit = new QAction("Exit", this);
    connect(g_exit, &QAction::triggered , this, &DM_MainWindow::close);
    file_menu->addAction(g_exit);

    // Set up Display canvas
    canvas = new dmGL();
    canvas->setMinimumSize(512,512);

    console = new pqConsoleWidget(this);
    console->prompt("$ ");
    QSplitter *wgrp = new QSplitter(Qt::Vertical, this);
    // With the drawing window we do better not to trigger continual redrawing
    // - it's a heavy drawing load and seems to cause occasional issues with
    // the wrong thread trying to grab the OpenGL context
    wgrp->setOpaqueResize(false);
    setCentralWidget(wgrp);
    if (canvas)
	wgrp->addWidget(canvas);
    wgrp->addWidget(console);

    QObject::connect(this->console, &pqConsoleWidget::executeCommand, this, &DM_MainWindow::run_cmd);
}

void
DM_MainWindow::open_file()
{
    const char *file_filters = "BRL-CAD (*.g *.asc);;All Files (*)";
    QString fileName = QFileDialog::getOpenFileName((QWidget *)this,
	    "Open Geometry File",
	    qApp->applicationDirPath(),
	    file_filters,
	    NULL,
	    QFileDialog::DontUseNativeDialog);
    if (!fileName.isEmpty()) {
	int ret = ((DMApp *)qApp)->load_g(fileName.toLocal8Bit(), 0, NULL);
	if (ret) {
	    statusBar()->showMessage("open failed");
	} else {
	    statusBar()->showMessage(fileName);
	}
    }
}

static unsigned long long
dl_name_hash(struct ged *gedp)
{
    if (!BU_LIST_NON_EMPTY(gedp->ged_gdp->gd_headDisplay))
	return 0;

    XXH64_hash_t hash_val;
    XXH64_state_t *state;
    state = XXH64_createState();
    if (!state)
	return 0;
    XXH64_reset(state, 0);

    struct display_list *gdlp;
    struct display_list *next_gdlp;
    gdlp = BU_LIST_NEXT(display_list, gedp->ged_gdp->gd_headDisplay);
    while (BU_LIST_NOT_HEAD(gdlp, gedp->ged_gdp->gd_headDisplay)) {
	struct bview_scene_obj *sp;
	struct bview_scene_obj *nsp;
	next_gdlp = BU_LIST_PNEXT(display_list, gdlp);
	sp = BU_LIST_NEXT(bview_scene_obj, &gdlp->dl_head_scene_obj);
	while (BU_LIST_NOT_HEAD(sp, &gdlp->dl_head_scene_obj)) {
	    nsp = BU_LIST_PNEXT(bview_scene_obj, sp);
	    if (sp->s_u_data) {
		struct ged_bview_data *bdata = (struct ged_bview_data *)sp->s_u_data;
		for (size_t i = 0; i < bdata->s_fullpath.fp_len; i++) {
		    struct directory *dp = bdata->s_fullpath.fp_names[i];
		    XXH64_update(state, dp->d_namep, strlen(dp->d_namep));
		}
	    }
	    sp = nsp;
	}
	gdlp = next_gdlp;
    }

    hash_val = XXH64_digest(state);
    XXH64_freeState(state);

    return (unsigned long long)hash_val;
}

static unsigned long long
dl_update(struct ged *gedp)
{
    unsigned long long hincr = 0;
    if (!BU_LIST_NON_EMPTY(gedp->ged_gdp->gd_headDisplay))
	return 0;

    struct display_list *gdlp;
    struct display_list *next_gdlp;
    gdlp = BU_LIST_NEXT(display_list, gedp->ged_gdp->gd_headDisplay);
    while (BU_LIST_NOT_HEAD(gdlp, gedp->ged_gdp->gd_headDisplay)) {
	struct bview_scene_obj *sp;
	struct bview_scene_obj *nsp;
	next_gdlp = BU_LIST_PNEXT(display_list, gdlp);
	sp = BU_LIST_NEXT(bview_scene_obj, &gdlp->dl_head_scene_obj);
	while (BU_LIST_NOT_HEAD(sp, &gdlp->dl_head_scene_obj)) {
	    nsp = BU_LIST_PNEXT(bview_scene_obj, sp);
	    // TODO - this core logic should probably become the default
	    // solid object s_update_callback function's standard viewing mode
	    // routine (when interactively editing primitives a.l.a. MGED's
	    // sedit mode, something else is needed since primitive parameters
	    // are supposed to change in response to mouse motions...  for
	    // normal checking we only need to redraw if the dbio layer asserts
	    // a command changed something).
	    if (sp->s_u_data) {
		int changed = 0;
		struct ged_bview_data *bdata = (struct ged_bview_data *)sp->s_u_data;
		for (size_t i = 0; i < bdata->s_fullpath.fp_len; i++) {
		    struct directory *dp = bdata->s_fullpath.fp_names[i];
		    if (dp->edit_flag) {
			// db object along path was edited - we
			// need to update the object's info
			changed = 1;
			hincr++;
			break;
		    }
		}
		if (changed) {
		    char *spath = db_path_to_string(&bdata->s_fullpath);
		    bu_log("redrawing %s\n", spath);
		    const char *av[3];
		    av[0] = "draw";
		    av[1] = spath;
		    av[2] = NULL;
		    ged_draw(gedp, 2, av);
		    bu_free(spath, "free spath");
		}
	    }
	    sp = nsp;
	}
	gdlp = next_gdlp;
    }
    // Updating done - clear all edit flags
    gdlp = BU_LIST_NEXT(display_list, gedp->ged_gdp->gd_headDisplay);
    while (BU_LIST_NOT_HEAD(gdlp, gedp->ged_gdp->gd_headDisplay)) {
	struct bview_scene_obj *sp;
	struct bview_scene_obj *nsp;
	next_gdlp = BU_LIST_PNEXT(display_list, gdlp);
	sp = BU_LIST_NEXT(bview_scene_obj, &gdlp->dl_head_scene_obj);
	while (BU_LIST_NOT_HEAD(sp, &gdlp->dl_head_scene_obj)) {
	    nsp = BU_LIST_PNEXT(bview_scene_obj, sp);
	    if (sp->s_u_data) {
		struct ged_bview_data *bdata = (struct ged_bview_data *)sp->s_u_data;
		for (size_t i = 0; i < bdata->s_fullpath.fp_len; i++) {
		    struct directory *dp = bdata->s_fullpath.fp_names[i];
		    dp->edit_flag = 0;
		}
	    }
	    sp = nsp;
	}
	gdlp = next_gdlp;
    }


    return hincr;
}

void
DM_MainWindow::run_cmd(const QString &command)
{
    if (BU_STR_EQUAL(command.toStdString().c_str(), "q"))
	bu_exit(0, "exit");

    if (!gedp) {
	console->printString("No database open");
	console->prompt("$ ");
	return;
    }

    // make an argv array
    struct bu_vls ged_prefixed = BU_VLS_INIT_ZERO;
    bu_vls_sprintf(&ged_prefixed, "%s", command.toStdString().c_str());
    char *input = bu_strdup(bu_vls_addr(&ged_prefixed));
    bu_vls_free(&ged_prefixed);
    char **av = (char **)bu_calloc(strlen(input) + 1, sizeof(char *), "argv array");
    int ac = bu_argv_from_string(av, strlen(input), input);

    if (ged_cmd_valid(av[0], NULL)) {
	const char *ccmd = NULL;
	int edist = ged_cmd_lookup(&ccmd, av[0]);
	if (edist) {
	    struct bu_vls msg = BU_VLS_INIT_ZERO;
	    bu_vls_sprintf(&msg, "Command %s not found, did you mean %s (edit distance %d)?\n", av[0], ccmd, edist);
	    console->printString(bu_vls_cstr(&msg));
	    bu_vls_free(&msg);
	}
    } else {
	// TODO - need to add hashing to check the dm variables as well (i.e. if lighting
	// was turned on/off by the dm command...)
	canvas->prev_dhash = dm_hash((struct dm *)gedp->ged_dmp);
	canvas->prev_vhash = bview_hash(canvas->v);
	canvas->prev_lhash = dl_name_hash(gedp);

	// Clear the edit flags (TODO - really should only do this for objects active in
	// the scene...)
	struct directory *dp;
	for (int i = 0; i < RT_DBNHASH; i++) {
	    for (dp = gedp->ged_wdbp->dbip->dbi_Head[i]; dp != RT_DIR_NULL; dp = dp->d_forw) {
		dp->edit_flag = 0;
	    }
	}

	ged_exec(gedp, ac, (const char **)av);
	console->printString(bu_vls_cstr(gedp->ged_result_str));
	bu_vls_trunc(gedp->ged_result_str, 0);
	if (canvas) {
	    if (canvas->v->gv_cleared) {
		const char *aav[2];
		aav[0] = "autoview";
		aav[1] = NULL;
		ged_autoview(gedp, 1, (const char **)aav);
		canvas->v->gv_cleared = 0;
		bview_update(canvas->v);
	    }

	    std::chrono::time_point<std::chrono::steady_clock> stime, etime, htimes, htimee;
	    int sectime;
	    stime = std::chrono::steady_clock::now();

	    htimes = std::chrono::steady_clock::now();
	    unsigned long long dhash = dm_hash((struct dm *)gedp->ged_dmp);
	    if (dhash != canvas->prev_dhash) {
		std::cout << "prev_dhash: " << canvas->prev_dhash << "\n";
		std::cout << "dhash: " << dhash << "\n";
		canvas->prev_dhash = dhash;
		dm_set_dirty((struct dm *)gedp->ged_dmp, 1);
	    }
	    htimee = std::chrono::steady_clock::now();
	    sectime = std::chrono::duration_cast<std::chrono::seconds>(htimee - htimes).count();
	    bu_log("DM state hash (sec): %d\n", sectime);

	    htimes = std::chrono::steady_clock::now();
	    unsigned long long vhash = bview_hash(canvas->v);
	    if (vhash != canvas->prev_vhash) {
		std::cout << "prev_vhash: " << canvas->prev_vhash << "\n";
		std::cout << "vhash: " << vhash << "\n";
		canvas->prev_vhash = vhash;
		dm_set_dirty((struct dm *)gedp->ged_dmp, 1);
	    }
	    htimee = std::chrono::steady_clock::now();
	    sectime = std::chrono::duration_cast<std::chrono::seconds>(htimee - htimes).count();
	    bu_log("View hash (sec): %d\n", sectime);

	    htimes = std::chrono::steady_clock::now();
	    unsigned long long lhash = dl_name_hash(gedp);
	    unsigned long long lhash_edit = lhash;
	    lhash_edit += dl_update(gedp);
	    if (lhash_edit != canvas->prev_lhash) {
		std::cout << "prev_lhash: " << canvas->prev_lhash << "\n";
		std::cout << "lhash: " << lhash_edit << "\n";
		canvas->prev_lhash = lhash;
		dm_set_dirty((struct dm *)gedp->ged_dmp, 1);
	    }
	    htimee = std::chrono::steady_clock::now();
	    sectime = std::chrono::duration_cast<std::chrono::seconds>(htimee - htimes).count();
	    bu_log("Display list hash (sec): %d\n", sectime);

	    htimes = std::chrono::steady_clock::now();
	    unsigned long long ghash = ged_dl_hash((struct display_list *)gedp->ged_gdp->gd_headDisplay);
	    if (ghash != canvas->prev_ghash) {
		std::cout << "prev_ghash: " << canvas->prev_ghash << "\n";
		std::cout << "ghash: " << ghash << "\n";
		canvas->prev_ghash = ghash;
		dm_set_dirty((struct dm *)gedp->ged_dmp, 1);
	    }
	    htimee = std::chrono::steady_clock::now();
	    sectime = std::chrono::duration_cast<std::chrono::seconds>(htimee - htimes).count();
	    bu_log("Display list hash (GED data)(sec): %d\n", sectime);

	    etime = std::chrono::steady_clock::now();
	    sectime = std::chrono::duration_cast<std::chrono::seconds>(etime - stime).count();
	    bu_log("All hashes (sec): %d\n", sectime);
	    if (dm_get_dirty((struct dm *)gedp->ged_dmp))
		canvas->update();
	}
    }

    bu_free(input, "input copy");
    bu_free(av, "input argv");

    console->prompt("$ ");
}

void DM_MainWindow::readSettings()
{
    QSettings settings("BRL-CAD", "DM");

    settings.beginGroup("DM_MainWindow");
    resize(settings.value("size", QSize(1100, 800)).toSize());
    settings.endGroup();
}

void DM_MainWindow::write_settings()
{
    QSettings settings("BRL-CAD", "DM");

    settings.beginGroup("DM_MainWindow");
    settings.setValue("size", size());
    settings.endGroup();
}

void DM_MainWindow::save_image()
{
    if (canvas)
	canvas->save_image();
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

