/*                      D M G L T . C P P
 * BRL-CAD
 *
 * Copyright (c) 2021 United States Government as represented by
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
/** @file dmglt.cpp
 *
 * Brief description
 *
 */

#include <QOpenGLWidget>
#include <QKeyEvent>
#include <QGuiApplication> // for qGuiApp

extern "C" {
#include "bu/parallel.h"
#include "dm/bview_util.h"
#include "ged.h"
}
#include "dmglt.h"

DMRendererT::DMRendererT(dmGLT *w)
    : m_w(w)
{
}

DMRendererT::~DMRendererT()
{
}

void DMRendererT::resize()
{
}

void DMRendererT::render()
{
    if (m_exiting)
	return;

    int w = m_w->width();
    int h = m_w->height();
    // Zero size == nothing to do
    if (!w || !h)
	return;

    if (m_init && (!m_w->gedp || !dm_get_dirty((struct dm *)m_w->gedp->ged_dmp))) {
	// Avoid a hot spin
	QThread::usleep(10000);
	return;
    }

    // Since we're in a separate rendering thread, there is
    // some preliminary work we need to do before proceeding
    // with OpenGL calls
    QOpenGLContext *ctx = m_w->context();
    if (!ctx) // QOpenGLWidget not yet initialized
	return;
    // Grab the context.
    m_grabMutex.lock();
    emit contextWanted();
    m_grabCond.wait(&m_grabMutex);
    QMutexLocker lock(&m_renderMutex);
    m_grabMutex.unlock();
    if (m_exiting)
	return;
    Q_ASSERT(ctx->thread() == QThread::currentThread());


    // Have context, initialize if necessary
    m_w->makeCurrent();
    if (!m_init) {
	initializeOpenGLFunctions();
	m_init = true;
	if (!dmp) {
	    const char *acmd = "attach";
	    dmp = dm_open((void *)m_w, NULL, "qtgl", 1, &acmd);
	    if (m_w->gedp) {
		m_w->gedp->ged_dmp = (void *)dmp;
		dm_set_vp(dmp, &m_w->gedp->ged_gvp->gv_scale);
	    }
	    dm_configure_win(dmp, 0);
	    dm_set_dirty(dmp, 1);
	}
    }

    if (dm_get_dirty(dmp)) {
	// TODO - dm should be doing this...
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// We're going to start drawing - clear the flag.
	// If anything changes while we're drawing, we need
	// to draw again, so this clearing should come before
	// the actual draw operation.
    dm_set_dirty(dmp, 0);


	// TODO - we are faced with a conundrum.  gd_headDisplay contains the
	// actual geometry to be drawn by libdm, but it is also manipulated by
	// a number of libged commands.  For the drawing thread to work
	// successfully while doing its work, we can't have vlist contents
	// being manipulated by commands at the same time.  Our options are:
	//
	// 1.  Block GED command execution until the current render is done -
	// either hardcode select commands (current libtclcad approach), build
	// a similar set by scanning the source code, or all commands (poor
	// usability).
	//
	// 2.  Change gd_headDisplay and struct solid.  Create a pool of solids
	// which are pointed to by headDisplay entries, and alter any libged
	// commands manipulating vlists to use an existing solid from the pool
	// or create a new one if none are queued, rather than operating on an
	// existing solid in headDisplay.  Then adjust the dl drawing routines
	// to take not gd_headDisplay but a separate bu_ptbl of struct solid
	// pointers which will be created from gd_headDisplay at draw time.
	// After drawing is complete, compare gd_headDisplay against the
	// pointers in the bu_ptbl and put any struct solids not actively
	// pointed to by gd_headDisplay into the pool after releasing their
	// vlists.  In this fashion libdm will not have any struct solid data
	// changed out from under it, but libged commands will still be able to
	// manipulate the libged headDisplay list.  The hashing mechanism will (at
	// least in theory) be able to detect changes in the display list and
	// set the dm_dirty flag.
	//
	// This would be a fairly heavy left, as libged, libtclcad and mged all
	// access gd_headDisplay...
	if (m_w->gedp) {
	    matp_t mat = m_w->gedp->ged_gvp->gv_model2view;
	dm_loadmatrix(dmp, mat, 0);
	unsigned char geometry_default_color[] = { 255, 0, 0 };
	    dm_draw_display_list(dmp, m_w->gedp->ged_gdp->gd_headDisplay,
		    1.0, m_w->gedp->ged_gvp->gv_isize, 255, 0, 0, 1,
		0, 0, geometry_default_color, 1, 0);
    }

}

    // Make no context current on this thread and move the QOpenGLWidget's
    // context back to the gui thread.
    m_w->doneCurrent();
    ctx->moveToThread(qGuiApp->thread());

    // Schedule composition. Note that this will use QueuedConnection, meaning
    // that update() will be invoked on the gui thread.
    QMetaObject::invokeMethod(m_w, "update");

    // Avoid a hot spin
    QThread::usleep(10000);
}

dmGLT::dmGLT(QWidget *parent)
    : QOpenGLWidget(parent)
{
    BU_GET(v, struct bview);
    bview_init(v);

    connect(this, &QOpenGLWidget::aboutToCompose, this, &dmGLT::onAboutToCompose);
    connect(this, &QOpenGLWidget::frameSwapped, this, &dmGLT::onFrameSwapped);
    connect(this, &QOpenGLWidget::aboutToResize, this, &dmGLT::onAboutToResize);
    connect(this, &QOpenGLWidget::resized, this, &dmGLT::onResized);

    m_thread = new QThread;

    // Create the renderer
    m_renderer = new DMRendererT(this);
    m_renderer->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_renderer, &QObject::deleteLater);

    // Let dmGLT know to trigger the renderer
    connect(this, &dmGLT::renderRequested, m_renderer, &DMRendererT::render);
    connect(m_renderer, &DMRendererT::contextWanted, this, &dmGLT::grabContext);

    m_thread->start();

    // This is an important Qt setting for interactivity - it allowing key
    // bindings to propagate to this widget and trigger actions such as
    // resolution scaling, rotation, etc.
    setFocusPolicy(Qt::WheelFocus);
}

dmGLT::~dmGLT()
{
    m_renderer->prepareExit();
    m_thread->quit();
    m_thread->wait();
    delete m_thread;
    BU_PUT(v, struct bview);
}

void dmGLT::onAboutToCompose()
{
    // We are on the gui thread here. Composition is about to
    // begin. Wait until the render thread finishes.
    m_renderer->lockRenderer();
}

void dmGLT::onFrameSwapped()
{
    m_renderer->unlockRenderer();
    // Assuming a blocking swap, our animation is driven purely by the
    // vsync in this example.
    emit renderRequested();
}

void dmGLT::onAboutToResize()
{
    m_renderer->lockRenderer();
}

void dmGLT::onResized()
{
    if (gedp && gedp->ged_dmp) {
	dm_configure_win((struct dm *)gedp->ged_dmp, 0);
	dm_set_dirty((struct dm *)gedp->ged_dmp, 1);
    }
    m_renderer->unlockRenderer();
}

void dmGLT::grabContext()
{
    if (m_renderer->m_exiting)
	return;
    m_renderer->lockRenderer();
    QMutexLocker lock(m_renderer->grabMutex());
    context()->moveToThread(m_thread);
    m_renderer->grabCond()->wakeAll();
    m_renderer->unlockRenderer();
}

void dmGLT::keyPressEvent(QKeyEvent *k) {
    //QString kstr = QKeySequence(k->key()).toString();
    //bu_log("%s\n", kstr.toStdString().c_str());
#if 0
    switch (k->key()) {
	case '=':
	    m_renderer->res_incr();
	    emit renderRequested();
	    update();
	    return;
	    break;
	case '-':
	    m_renderer->res_decr();
	    emit renderRequested();
	    update();
	    return;
	    break;
    }
#endif
    QOpenGLWidget::keyPressEvent(k);
}

void dmGLT::mousePressEvent(QMouseEvent *e) {

    // If we're intending the mouse motion to do the work,
    // then the press has to be a no-op.  If we're going
    // to do configurable key bindings, this will take some
    // thought - if we want unmodded left button to be a
    // rotation, and Ctrl+Left to do something else, these
    // checks are all going to have to be exact-flag-combo-only
    // actions.
    if (e->modifiers()) {
	QOpenGLWidget::mousePressEvent(e);
	return;
    }

    int dx = 0;
    int dy = 0;
    unsigned long long view_flags = BVIEW_IDLE;

    // Let bview know what the current view width and height are, in
    // case the dx/dy mouse translations need that information
    v->gv_width = width();
    v->gv_height = height();

    if (e->buttons().testFlag(Qt::LeftButton)) {
	bu_log("Press Left\n");
	view_flags = BVIEW_SCALE;
	dy = -100;
    }
    if (e->buttons().testFlag(Qt::RightButton)) {
	bu_log("Press Right\n");
	view_flags = BVIEW_SCALE;
	dy = 100;
    }
    if (e->buttons().testFlag(Qt::MiddleButton)) {
	bu_log("Press Middle\n");
    }

    point_t center;
    MAT_DELTAS_GET_NEG(center, v->gv_center);
    VSCALE(center, center, gedp->ged_wdbp->dbip->dbi_base2local);
    if (bview_adjust(v, dx, dy, center, BVIEW_VIEW, view_flags)) {
	dm_set_dirty((struct dm *)gedp->ged_dmp, 1);
	update();
    }


    QOpenGLWidget::mousePressEvent(e);
}

void dmGLT::mouseMoveEvent(QMouseEvent *e)
{
    unsigned long long view_flags = BVIEW_IDLE;

    if (x_prev == -INT_MAX) {
	x_prev = e->x();
	y_prev = e->y();
	QOpenGLWidget::mouseMoveEvent(e);
	return;
    }

    if (!gedp || !gedp->ged_dmp) {
	QOpenGLWidget::mouseMoveEvent(e);
	return;
    }

    if (e->buttons().testFlag(Qt::LeftButton)) {
	bu_log("Left\n");

	if (e->modifiers().testFlag(Qt::ControlModifier)) {
	    bu_log("Ctrl+Left\n");
	    view_flags = BVIEW_ROT;
	}

	if (e->modifiers().testFlag(Qt::ShiftModifier)) {
	    bu_log("Shift+Left\n");
	    view_flags = BVIEW_TRANS;
	}

	if (e->modifiers().testFlag(Qt::ShiftModifier) && e->modifiers().testFlag(Qt::ControlModifier)) {
	    bu_log("Ctrl+Shift+Left\n");
	    view_flags = BVIEW_SCALE;
	}
    }

    if (e->buttons().testFlag(Qt::MiddleButton)) {
	bu_log("Middle\n");
    }

    if (e->buttons().testFlag(Qt::RightButton)) {
	bu_log("Right\n");
    }

    // Let bview know what the current view width and height are, in
    // case the dx/dy mouse translations need that information
    v->gv_width = width();
    v->gv_height = height();

    // Start following MGED's mouse motions to see how it handles view
    // updates.  The trail starts at doevent.c's motion_event_handler,
    // which in turn generates a command fed to f_knob.
    int dx = e->x() - x_prev;
    int dy = e->y() - y_prev;

    // TODO - the key point and the mode/flags are all hardcoded
    // right now, but eventually for shift grips they will need to
    // respond to the various mod keys.  The intent is to set flags
    // based on which mod keys are set to allow bview_adjust to
    // do the correct math.
    point_t center;
    MAT_DELTAS_GET_NEG(center, v->gv_center);
    VSCALE(center, center, gedp->ged_wdbp->dbip->dbi_base2local);
    if (bview_adjust(v, dx, dy, center, BVIEW_VIEW, view_flags)) {
	 dm_set_dirty((struct dm *)gedp->ged_dmp, 1);
	 emit renderRequested();
    }

    // Current positions are the new previous positions
    x_prev = e->x();
    y_prev = e->y();

    QOpenGLWidget::mouseMoveEvent(e);
}

void dmGLT::wheelEvent(QWheelEvent *e) {
    QPoint delta = e->angleDelta();
    int incr = delta.y() / 8;

    // Let bview know what the current view width and height are, in
    // case the dx/dy mouse translations need that information
    v->gv_width = width();
    v->gv_height = height();

    point_t origin = VINIT_ZERO;
    if (bview_adjust(v, 0, incr, origin, BVIEW_VIEW, BVIEW_SCALE)) {
	 dm_set_dirty((struct dm *)gedp->ged_dmp, 1);
	 update();
    }

    QOpenGLWidget::wheelEvent(e);
}

void dmGLT::mouseReleaseEvent(QMouseEvent *e) {
    // To avoid an abrupt jump in scene motion the next time movement is
    // started with the mouse, after we release we return to the default state.
    x_prev = -INT_MAX;
    y_prev = -INT_MAX;

    QOpenGLWidget::mouseReleaseEvent(e);
}

void dmGLT::save_image() {
    QImage image = this->grabFramebuffer();
    image.save("file.png");
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

