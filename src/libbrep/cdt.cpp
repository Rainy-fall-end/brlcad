/*                        C D T . C P P
 * BRL-CAD
 *
 * Copyright (c) 2007-2019 United States Government as represented by
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
/** @addtogroup libbrep */
/** @{ */
/** @file cdt.cpp
 *
 * Constrained Delaunay Triangulation of NURBS B-Rep objects.
 *
 */

#include "common.h"

#include <vector>
#include <list>
#include <map>
#include <stack>
#include <iostream>
#include <algorithm>
#include <set>
#include <utility>

#include "poly2tri/poly2tri.h"

#include "assert.h"

#include "vmath.h"

#include "bu/cv.h"
#include "bu/opt.h"
#include "bu/time.h"
#include "brep/defines.h"
#include "brep/cdt.h"
#include "brep/pullback.h"
#include "brep/util.h"

#define BREP_CDT_FAILED -3
#define BREP_CDT_NON_SOLID -2
#define BREP_CDT_UNTESSELLATED -1
#define BREP_CDT_SOLID 0

struct ON_Brep_CDT_State {

    int status;
    ON_Brep *brep;

    /* Tolerances */
    fastf_t abs;
    fastf_t rel;
    fastf_t norm;
    fastf_t dist;

    /* 3D data */
    std::vector<ON_3dPoint *> *w3dpnts;
    std::map<int, ON_3dPoint *> *vert_pnts;
    std::vector<ON_3dPoint *> *w3dnorms;
    std::map<int, ON_3dPoint *> *vert_norms;

    /* Poly2Tri data */
    p2t::CDT **p2t_faces;
    std::map<p2t::Point *, ON_3dPoint *> **p2t_maps;
    std::map<p2t::Point *, ON_3dPoint *> **p2t_nmaps;
};

struct brep_cdt_tol {
    fastf_t min_dist;
    fastf_t max_dist;
    fastf_t within_dist;
    fastf_t cos_within_ang;
};

#define BREP_CDT_TOL_ZERO {0.0, 0.0, 0.0, 0.0}

// Digest tessellation tolerances...
static void
CDT_Tol_Set(struct brep_cdt_tol *cdt, double dist, fastf_t md, double t_abs, double t_rel, double t_norm, double t_dist)
{
    fastf_t min_dist, max_dist, within_dist, cos_within_ang;

    max_dist = md;

    if (t_abs < t_dist + ON_ZERO_TOLERANCE) {
	min_dist = t_dist;
    } else {
	min_dist = t_abs;
    }

    double rel = 0.0;
    if (t_rel > 0.0 + ON_ZERO_TOLERANCE) {
	rel = t_rel * dist;
	if (max_dist < rel * 10.0) {
	    max_dist = rel * 10.0;
	}
	within_dist = rel < min_dist ? min_dist : rel;
    } else if (t_abs > 0.0 + ON_ZERO_TOLERANCE) {
	within_dist = min_dist;
    } else {
	within_dist = 0.01 * dist; // default to 1% of dist
    }

    if (t_norm > 0.0 + ON_ZERO_TOLERANCE) {
	cos_within_ang = cos(t_norm);
    } else {
	cos_within_ang = cos(ON_PI / 2.0);
    }

    cdt->min_dist = min_dist;
    cdt->max_dist = max_dist;
    cdt->within_dist = within_dist;
    cdt->cos_within_ang = cos_within_ang;
}


static void
getEdgePoints(
	ON_BrepEdge *edge,
	ON_NurbsCurve *nc,
	const ON_BrepTrim &trim,
	BrepTrimPoint *sbtp1,
	BrepTrimPoint *ebtp1,
	BrepTrimPoint *sbtp2,
	BrepTrimPoint *ebtp2,
	const struct brep_cdt_tol *cdt_tol,
	std::map<double, BrepTrimPoint *> *trim1_param_points,
	std::map<double, BrepTrimPoint *> *trim2_param_points,
	std::vector<ON_3dPoint *> *w3dpnts,
	std::vector<ON_3dPoint *> *w3dnorms
	)
{
    ON_3dPoint tmp1, tmp2;
    ON_BrepTrim *trim2 = (edge->Trim(0)->m_trim_index == trim.m_trim_index) ? edge->Trim(1) : edge->Trim(0);
    const ON_Surface *s1 = trim.SurfaceOf();
    const ON_Surface *s2 = trim2->SurfaceOf();
    fastf_t t1 = (sbtp1->t + ebtp1->t) / 2.0;
    fastf_t t2 = (sbtp2->t + ebtp2->t) / 2.0;
    ON_3dPoint trim1_mid_2d = trim.PointAt(t1);
    ON_3dPoint trim2_mid_2d = trim2->PointAt(t2);
    fastf_t emid = (sbtp1->e + ebtp1->e) / 2.0;

    ON_3dVector trim1_mid_norm = ON_3dVector::UnsetVector;
    ON_3dVector trim2_mid_norm = ON_3dVector::UnsetVector;
    ON_3dPoint edge_mid_3d = ON_3dPoint::UnsetPoint;
    ON_3dVector edge_mid_tang = ON_3dVector::UnsetVector;


    if (!(nc->EvTangent(emid, edge_mid_3d, edge_mid_tang))) {
	// EvTangent call failed, get 3d point
	edge_mid_3d = nc->PointAt(emid);
	// If the edge curve failed, try to average tangents from trims
	ON_3dVector trim1_mid_tang(0.0, 0.0, 0.0);
	ON_3dVector trim2_mid_tang(0.0, 0.0, 0.0);
	int evals = 0;
	evals += (trim.EvTangent(t1, tmp1, trim1_mid_tang)) ? 1 : 0;
	evals += (trim2->EvTangent(t2, tmp2, trim2_mid_tang)) ? 1 : 0;
	if (evals == 2) {
	    edge_mid_tang = (trim1_mid_tang + trim2_mid_tang) / 2;
	} else {
	    edge_mid_tang = ON_3dVector::UnsetVector;
	}
    }

    // bu_log("Edge %d, Trim %d(%d): %f %f %f\n", edge->m_edge_index, trim.m_trim_index, trim.m_bRev3d, edge_mid_3d.x, edge_mid_3d.y, edge_mid_3d.z);

    int dosplit = 0;

    ON_Line line3d(*(sbtp1->p3d), *(ebtp1->p3d));
    double dist3d = edge_mid_3d.DistanceTo(line3d.ClosestPointTo(edge_mid_3d));
    dosplit += (line3d.Length() > cdt_tol->max_dist) ? 1 : 0;
    dosplit += (dist3d > (cdt_tol->within_dist + ON_ZERO_TOLERANCE)) ? 1 : 0;

    if ((dist3d > cdt_tol->min_dist + ON_ZERO_TOLERANCE)) {
	if (!dosplit) {
	    dosplit += ((sbtp1->tangent * ebtp1->tangent) < cdt_tol->cos_within_ang - ON_ZERO_TOLERANCE) ? 1 : 0;
	}

	if (!dosplit && sbtp1->normal != ON_3dVector::UnsetVector && ebtp1->normal != ON_3dVector::UnsetVector) {
	    dosplit += ((sbtp1->normal * ebtp1->normal) < cdt_tol->cos_within_ang - ON_ZERO_TOLERANCE) ? 1 : 0;
	}

	if (!dosplit && sbtp2->normal != ON_3dVector::UnsetVector && ebtp2->normal != ON_3dVector::UnsetVector) {
	    dosplit += ((sbtp2->normal * ebtp2->normal) < cdt_tol->cos_within_ang - ON_ZERO_TOLERANCE) ? 1 : 0;
	}
    }

    if (dosplit) {

	if (!surface_EvNormal(s1, trim1_mid_2d.x, trim1_mid_2d.y, tmp1, trim1_mid_norm)) {
	    trim1_mid_norm = ON_3dVector::UnsetVector;
	} else {
	    if (trim.Face()->m_bRev) {
		trim1_mid_norm = trim1_mid_norm  * -1.0;
	    }
	}

	if (!surface_EvNormal(s2, trim2_mid_2d.x, trim2_mid_2d.y, tmp2, trim2_mid_norm)) {
	    trim2_mid_norm = ON_3dVector::UnsetVector;
	} else {
	    if (trim2->Face()->m_bRev) {
		trim2_mid_norm = trim2_mid_norm  * -1.0;
	    }
	}

	ON_3dPoint *npt = new ON_3dPoint(edge_mid_3d);
	w3dpnts->push_back(npt);

	ON_3dPoint *nrm = new ON_3dPoint((trim1_mid_norm + trim1_mid_norm)/2);
	w3dnorms->push_back(nrm);

	BrepTrimPoint *nbtp1 = new BrepTrimPoint;
	nbtp1->p3d = npt;
	nbtp1->n3d = nrm;
	nbtp1->p2d = trim1_mid_2d;
	nbtp1->normal = trim1_mid_norm;
	nbtp1->tangent = edge_mid_tang;
	nbtp1->t = t1;
	nbtp1->e = emid;
	(*trim1_param_points)[nbtp1->t] = nbtp1;

	BrepTrimPoint *nbtp2 = new BrepTrimPoint;
	nbtp2->p3d = npt;
	nbtp2->n3d = nrm;
	nbtp2->p2d = trim2_mid_2d;
	nbtp2->normal = trim2_mid_norm;
	nbtp2->tangent = edge_mid_tang;
	nbtp2->t = t2;
	nbtp2->e = emid;
	(*trim2_param_points)[nbtp2->t] = nbtp2;

	getEdgePoints(edge, nc, trim, sbtp1, nbtp1, sbtp2, nbtp2, cdt_tol, trim1_param_points, trim2_param_points, w3dpnts, w3dnorms);
	getEdgePoints(edge, nc, trim, nbtp1, ebtp1, nbtp2, ebtp2, cdt_tol, trim1_param_points, trim2_param_points, w3dpnts, w3dnorms);
	return;
    }

    int udir = 0;
    int vdir = 0;
    double trim1_seam_t = 0.0;
    double trim2_seam_t = 0.0;
    ON_2dPoint trim1_start = sbtp1->p2d;
    ON_2dPoint trim1_end = ebtp1->p2d;
    ON_2dPoint trim2_start = sbtp2->p2d;
    ON_2dPoint trim2_end = ebtp2->p2d;
    ON_2dPoint trim1_seam_2d, trim2_seam_2d;
    ON_3dPoint trim1_seam_3d, trim2_seam_3d;
    int t1_dosplit = 0;
    int t2_dosplit = 0;

    if (ConsecutivePointsCrossClosedSeam(s1, trim1_start, trim1_end, udir, vdir, BREP_SAME_POINT_TOLERANCE)) {
	ON_2dPoint from = ON_2dPoint::UnsetPoint;
	ON_2dPoint to = ON_2dPoint::UnsetPoint;
	if (FindTrimSeamCrossing(trim, sbtp1->t, ebtp1->t, trim1_seam_t, from, to, BREP_SAME_POINT_TOLERANCE)) {
	    trim1_seam_2d = trim.PointAt(trim1_seam_t);
	    trim1_seam_3d = s1->PointAt(trim1_seam_2d.x, trim1_seam_2d.y);
	    if (trim1_param_points->find(trim1_seam_t) == trim1_param_points->end()) {
		t1_dosplit = 1;
	    }
	}
    }

    if (ConsecutivePointsCrossClosedSeam(s2, trim2_start, trim2_end, udir, vdir, BREP_SAME_POINT_TOLERANCE)) {
	ON_2dPoint from = ON_2dPoint::UnsetPoint;
	ON_2dPoint to = ON_2dPoint::UnsetPoint;
	if (FindTrimSeamCrossing(trim, sbtp2->t, ebtp2->t, trim2_seam_t, from, to, BREP_SAME_POINT_TOLERANCE)) {
	    trim2_seam_2d = trim2->PointAt(trim2_seam_t);
	    trim2_seam_3d = s2->PointAt(trim2_seam_2d.x, trim2_seam_2d.y);
	    if (trim2_param_points->find(trim2_seam_t) == trim2_param_points->end()) {
		t2_dosplit = 1;
	    }
	}
    }

    if (t1_dosplit || t2_dosplit) {
	ON_3dPoint *nsptp;
	if (t1_dosplit && t2_dosplit) {
	    ON_3dPoint nspt = (trim1_seam_3d + trim2_seam_3d)/2;
	    nsptp = new ON_3dPoint(nspt);
	} else {
	    if (!t1_dosplit) {
		trim1_seam_t = (sbtp1->t + ebtp1->t)/2;
		trim1_seam_2d = trim.PointAt(trim1_seam_t);
		nsptp = new ON_3dPoint(trim2_seam_3d);
	    }
	    if (!t2_dosplit) {
		trim2_seam_t = (sbtp2->t + ebtp2->t)/2;
		trim2_seam_2d = trim2->PointAt(trim2_seam_t);
		nsptp = new ON_3dPoint(trim1_seam_3d);
	    }
	}
	w3dpnts->push_back(nsptp);

	// Note - by this point we shouldn't need tangents and normals...
	BrepTrimPoint *nbtp1 = new BrepTrimPoint;
	nbtp1->p3d = nsptp;
	nbtp1->p2d = trim1_seam_2d;
	nbtp1->t = trim1_seam_t;
	nbtp1->e = ON_UNSET_VALUE;
	(*trim1_param_points)[nbtp1->t] = nbtp1;

	BrepTrimPoint *nbtp2 = new BrepTrimPoint;
	nbtp2->p3d = nsptp;
	nbtp2->p2d = trim2_seam_2d;
	nbtp2->t = trim2_seam_t;
	nbtp2->e = ON_UNSET_VALUE;
	(*trim2_param_points)[nbtp2->t] = nbtp2;
    }

}

static std::map<double, BrepTrimPoint *> *
getEdgePoints(
	struct ON_Brep_CDT_State *s,
	ON_BrepEdge *edge,
	ON_BrepTrim &trim,
	fastf_t max_dist,
	std::map<int, ON_3dPoint *> *vert_pnts,
	std::vector<ON_3dPoint *> *w3dpnts,
	std::map<int, ON_3dPoint *> *vert_norms,
	std::vector<ON_3dPoint *> *w3dnorms
	)
{
    struct brep_cdt_tol cdt_tol = BREP_CDT_TOL_ZERO;
    std::map<double, BrepTrimPoint *> *trim1_param_points = NULL;
    std::map<double, BrepTrimPoint *> *trim2_param_points = NULL;

    // Get the other trim
    // TODO - this won't work if we don't have a 1->2 edge to trims relationship - in that
    // case we'll have to split up the edge and find the matching sub-trims (possibly splitting
    // those as well if they don't line up at shared 3D points.)
    ON_BrepTrim *trim2 = (edge->Trim(0)->m_trim_index == trim.m_trim_index) ? edge->Trim(1) : edge->Trim(0);

    double dist = 1000.0;

    const ON_Surface *s1 = trim.SurfaceOf();
    const ON_Surface *s2 = trim2->SurfaceOf();

    bool bGrowBox = false;
    ON_3dPoint min, max;

    /* If we're out of sync, bail - something is very very wrong */
    if (trim.m_trim_user.p != NULL && trim2->m_trim_user.p == NULL) {
	return NULL;
    }
    if (trim.m_trim_user.p == NULL && trim2->m_trim_user.p != NULL) {
	return NULL;
    }


    /* If we've already got the points, just return them */
    if (trim.m_trim_user.p != NULL) {
	trim1_param_points = (std::map<double, BrepTrimPoint *> *) trim.m_trim_user.p;
	return trim1_param_points;
    }

    /* Establish tolerances (TODO - get from edge curve...) */
    if (trim.GetBoundingBox(min, max, bGrowBox)) {
	dist = DIST_PT_PT(min, max);
    }
    CDT_Tol_Set(&cdt_tol, dist, max_dist, s->abs, s->rel, s->norm, s->dist);

    /* Begin point collection */
    ON_3dPoint tmp1, tmp2;
    int evals = 0;
    ON_3dPoint *edge_start_3d, *edge_end_3d = NULL;
    ON_3dPoint *edge_start_3dnorm, *edge_end_3dnorm = NULL;
    ON_3dVector edge_start_tang, edge_end_tang = ON_3dVector::UnsetVector;
    ON_3dPoint trim1_start_2d, trim1_end_2d = ON_3dPoint::UnsetPoint;
    ON_3dVector trim1_start_tang, trim1_end_tang, trim1_start_normal, trim1_end_normal = ON_3dVector::UnsetVector;
    ON_3dPoint trim2_start_2d, trim2_end_2d = ON_3dPoint::UnsetPoint;
    ON_3dVector trim2_start_tang, trim2_end_tang, trim2_start_normal, trim2_end_normal = ON_3dVector::UnsetVector;

    trim1_param_points = new std::map<double, BrepTrimPoint *>();
    trim.m_trim_user.p = (void *) trim1_param_points;
    trim2_param_points = new std::map<double, BrepTrimPoint *>();
    trim2->m_trim_user.p = (void *) trim2_param_points;

    ON_Interval range1 = trim.Domain();
    ON_Interval range2 = trim2->Domain();

    // TODO - what is this for?
    if (s1->IsClosed(0) || s1->IsClosed(1)) {
	ON_BoundingBox trim_bbox = ON_BoundingBox::EmptyBoundingBox;
	trim.GetBoundingBox(trim_bbox, false);
    }
    if (s2->IsClosed(0) || s2->IsClosed(1)) {
	ON_BoundingBox trim_bbox2 = ON_BoundingBox::EmptyBoundingBox;
	trim2->GetBoundingBox(trim_bbox2, false);
    }

    /* Normalize the domain of the curve to the ControlPolygonLength() of the
     * NURBS form of the curve to attempt to minimize distortion in 3D to
     * mirror what we do for the surfaces.  (GetSurfaceSize uses this under the
     * hood for its estimates.)  */
    const ON_Curve* crv = edge->EdgeCurveOf();
    ON_NurbsCurve *nc = crv->NurbsCurve();
    double cplen = nc->ControlPolygonLength();
    nc->SetDomain(0.0, cplen);
    ON_Interval erange = nc->Domain();

    /* For beginning and end of curve, we use vert points */
    edge_start_3d = (*vert_pnts)[edge->Vertex(0)->m_vertex_index];
    edge_end_3d = (*vert_pnts)[edge->Vertex(1)->m_vertex_index];
    edge_start_3dnorm = (*vert_norms)[edge->Vertex(0)->m_vertex_index];
    edge_end_3dnorm = (*vert_norms)[edge->Vertex(1)->m_vertex_index];

    /* Populate the 2D points */
    double st1 = (trim.m_bRev3d) ? range1.m_t[1] : range1.m_t[0];
    double et1 = (trim.m_bRev3d) ? range1.m_t[0] : range1.m_t[1];
    double st2 = (trim2->m_bRev3d) ? range2.m_t[1] : range2.m_t[0];
    double et2 = (trim2->m_bRev3d) ? range2.m_t[0] : range2.m_t[1];
    trim1_start_2d = trim.PointAt(st1);
    trim1_end_2d = trim.PointAt(et1);
    trim2_start_2d = trim2->PointAt(st2);
    trim2_end_2d = trim2->PointAt(et2);

    /* Get starting tangent from edge*/
    if (!(nc->EvTangent(erange.m_t[0], tmp1, edge_start_tang))) {
	// If the edge curve failed, average tangents from trims
	evals = 0;
	evals += (trim.EvTangent(st1, tmp1, trim1_start_tang)) ? 1 : 0;
	evals += (trim2->EvTangent(st2, tmp2, trim2_start_tang)) ? 1 : 0;
	if (evals == 2) {
	    edge_start_tang = (trim1_start_tang + trim2_start_tang) / 2;
	} else {
	    edge_start_tang = ON_3dVector::UnsetVector;
	}
    }
    /* Get ending tangent from edge*/
    if (!(nc->EvTangent(erange.m_t[1], tmp2, edge_end_tang))) {
	// If the edge curve failed, average tangents from trims
	evals = 0;
	evals += (trim.EvTangent(et1, tmp1, trim1_end_tang)) ? 1 : 0;
	evals += (trim2->EvTangent(et2, tmp2, trim2_end_tang)) ? 1 : 0;
	if (evals == 2) {
	    edge_end_tang = (trim1_end_tang + trim2_end_tang) / 2;
	} else {
	    edge_end_tang = ON_3dVector::UnsetVector;
	}
    }

    // Get the normals
    evals = 0;
    if (surface_EvNormal(s1, trim1_start_2d.x, trim1_start_2d.y, tmp1, trim1_start_normal)) {
	if (trim.Face()->m_bRev) {
	    trim1_start_normal = trim1_start_normal  * -1.0;
	}
	evals++;
    }
    if (surface_EvNormal(s1, trim1_end_2d.x, trim1_end_2d.y, tmp2, trim1_end_normal)) {
	if (trim.Face()->m_bRev) {
	    trim1_end_normal = trim1_end_normal  * -1.0;
	}
	evals++;
    }
    if (surface_EvNormal(s2, trim2_start_2d.x, trim2_start_2d.y, tmp1, trim2_start_normal)) {
	if (trim2->Face()->m_bRev) {
	    trim2_start_normal = trim2_start_normal  * -1.0;
	}
	evals++;
    }
    if (surface_EvNormal(s2, trim2_end_2d.x, trim2_end_2d.y, tmp2, trim2_end_normal)) {
	if (trim2->Face()->m_bRev) {
	    trim2_end_normal = trim2_end_normal  * -1.0;
	}
	evals++;
    }

    if (evals != 4) {
	bu_log("problem with normal evals\n");
    }

    /* Start and end points for both trims can now be defined */
    BrepTrimPoint *sbtp1 = new BrepTrimPoint;
    sbtp1->p3d = edge_start_3d;
    sbtp1->n3d = edge_start_3dnorm;
    sbtp1->tangent = edge_start_tang;
    sbtp1->e = erange.m_t[0];
    sbtp1->p2d = trim1_start_2d;
    sbtp1->normal = trim1_start_normal;
    sbtp1->t = st1;
    (*trim1_param_points)[sbtp1->t] = sbtp1;

    BrepTrimPoint *ebtp1 = new BrepTrimPoint;
    ebtp1->p3d = edge_end_3d;
    ebtp1->n3d = edge_end_3dnorm;
    ebtp1->tangent = edge_end_tang;
    ebtp1->e = erange.m_t[1];
    ebtp1->p2d = trim1_end_2d;
    ebtp1->normal = trim1_end_normal;
    ebtp1->t = et1;
    (*trim1_param_points)[ebtp1->t] = ebtp1;

    BrepTrimPoint *sbtp2 = new BrepTrimPoint;
    sbtp2->p3d = edge_start_3d;
    sbtp2->n3d = edge_start_3dnorm;
    sbtp2->tangent = edge_start_tang;
    sbtp2->e = erange.m_t[0];
    sbtp2->p2d = trim2_start_2d;
    sbtp2->normal = trim2_start_normal;
    sbtp2->t = st2;
    (*trim2_param_points)[sbtp2->t] = sbtp2;

    BrepTrimPoint *ebtp2 = new BrepTrimPoint;
    ebtp2->p3d = edge_end_3d;
    ebtp2->n3d = edge_end_3dnorm;
    ebtp2->tangent = edge_end_tang;
    ebtp2->e = erange.m_t[1];
    ebtp2->p2d = trim2_end_2d;
    ebtp2->normal = trim2_end_normal;
    ebtp2->t = et2;
    (*trim2_param_points)[ebtp2->t] = ebtp2;


    if (trim.IsClosed() || trim2->IsClosed()) {

	double trim1_mid_range = (st1 + et1) / 2.0;
	double trim2_mid_range = (st2 + et2) / 2.0;
	double edge_mid_range = (erange.m_t[0] + erange.m_t[1]) / 2.0;
	ON_3dVector edge_mid_tang, trim1_mid_norm, trim2_mid_norm = ON_3dVector::UnsetVector;
	ON_3dPoint edge_mid_3d = ON_3dPoint::UnsetPoint;

	if (!(nc->EvTangent(edge_mid_range, edge_mid_3d, edge_mid_tang))) {
	    // EvTangent call failed, get 3d point
	    edge_mid_3d = nc->PointAt(edge_mid_range);
	    // If the edge curve failed, try to average tangents from trims
	    ON_3dVector trim1_mid_tang(0.0, 0.0, 0.0);
	    ON_3dVector trim2_mid_tang(0.0, 0.0, 0.0);
	    evals = 0;
	    evals += (trim.EvTangent(trim1_mid_range, tmp1, trim1_mid_tang)) ? 1 : 0;
	    evals += (trim2->EvTangent(trim2_mid_range, tmp2, trim2_mid_tang)) ? 1 : 0;
	    if (evals == 2) {
		edge_mid_tang = (trim1_start_tang + trim2_start_tang) / 2;
	    } else {
		edge_mid_tang = ON_3dVector::UnsetVector;
	    }
	}

	evals = 0;
	ON_3dPoint trim1_mid_2d = trim.PointAt(trim1_mid_range);
	ON_3dPoint trim2_mid_2d = trim2->PointAt(trim2_mid_range);
	evals += (surface_EvNormal(s1, trim1_mid_2d.x, trim1_mid_2d.y, tmp1, trim1_mid_norm)) ? 1 : 0;
	evals += (surface_EvNormal(s2, trim2_mid_2d.x, trim2_mid_2d.y, tmp2, trim2_mid_norm)) ? 1 : 0;
	if (evals != 2) {
	    bu_log("problem with mid normal evals\n");
	}

	ON_3dPoint *nmp = new ON_3dPoint(edge_mid_3d);
	w3dpnts->push_back(nmp);
	ON_3dPoint *nmn = new ON_3dPoint((trim1_mid_norm + trim2_mid_norm)/2);
	w3dnorms->push_back(nmn);

	BrepTrimPoint *mbtp1 = new BrepTrimPoint;
	mbtp1->p3d = nmp;
	mbtp1->n3d = nmn;
	mbtp1->p2d = trim1_mid_2d;
	mbtp1->tangent = edge_mid_tang;
	mbtp1->normal = trim1_mid_norm;
	mbtp1->t = trim1_mid_range;
	mbtp1->e = edge_mid_range;
	(*trim1_param_points)[mbtp1->t] = mbtp1;

	BrepTrimPoint *mbtp2 = new BrepTrimPoint;
	mbtp2->p3d = nmp;
	mbtp2->n3d = nmn;
	mbtp2->p2d = trim2_mid_2d;
	mbtp2->tangent = edge_mid_tang;
	mbtp2->normal = trim2_mid_norm;
	mbtp2->t = trim2_mid_range;
	mbtp1->e = edge_mid_range;
	(*trim2_param_points)[mbtp2->t] = mbtp2;

	getEdgePoints(edge, nc, trim, sbtp1, mbtp1, sbtp2, mbtp2, &cdt_tol, trim1_param_points, trim2_param_points, w3dpnts, w3dnorms);
	getEdgePoints(edge, nc, trim, mbtp1, ebtp1, mbtp2, ebtp2, &cdt_tol, trim1_param_points, trim2_param_points, w3dpnts, w3dnorms);

    } else {

	getEdgePoints(edge, nc, trim, sbtp1, ebtp1, sbtp2, ebtp2, &cdt_tol, trim1_param_points, trim2_param_points, w3dpnts, w3dnorms);

    }

    return trim1_param_points;
}

static void
getSurfacePoints(const ON_Surface *s,
		 fastf_t u1,
		 fastf_t u2,
		 fastf_t v1,
		 fastf_t v2,
		 fastf_t min_dist,
		 fastf_t within_dist,
		 fastf_t cos_within_ang,
		 ON_2dPointArray &on_surf_points,
		 bool left,
		 bool below)
{
    double ldfactor = 2.0;
    ON_2dPoint p2d(0.0, 0.0);
    ON_3dPoint p[4] = {ON_3dPoint(), ON_3dPoint(), ON_3dPoint(), ON_3dPoint()};
    ON_3dVector norm[4] = {ON_3dVector(), ON_3dVector(), ON_3dVector(), ON_3dVector()};
    ON_3dPoint mid(0.0, 0.0, 0.0);
    ON_3dVector norm_mid(0.0, 0.0, 0.0);
    fastf_t u = (u1 + u2) / 2.0;
    fastf_t v = (v1 + v2) / 2.0;
    fastf_t udist = u2 - u1;
    fastf_t vdist = v2 - v1;

    if ((udist < min_dist + ON_ZERO_TOLERANCE)
	|| (vdist < min_dist + ON_ZERO_TOLERANCE)) {
	return;
    }

    if (udist > ldfactor * vdist) {
	int isteps = (int)(udist / vdist);
	isteps = (int)(udist / vdist / ldfactor * 2.0);
	fastf_t step = udist / (fastf_t) isteps;

	fastf_t step_u;
	for (int i = 1; i <= isteps; i++) {
	    step_u = u1 + i * step;
	    if ((below) && (i < isteps)) {
		p2d.Set(step_u, v1);
		on_surf_points.Append(p2d);
	    }
	    if (i == 1) {
		getSurfacePoints(s, u1, u1 + step, v1, v2, min_dist,
				 within_dist, cos_within_ang, on_surf_points, left,
				 below);
	    } else if (i == isteps) {
		getSurfacePoints(s, u2 - step, u2, v1, v2, min_dist,
				 within_dist, cos_within_ang, on_surf_points, left,
				 below);
	    } else {
		getSurfacePoints(s, step_u - step, step_u, v1, v2, min_dist, within_dist,
				 cos_within_ang, on_surf_points, left, below);
	    }
	    left = false;

	    if (i < isteps) {
		//top
		p2d.Set(step_u, v2);
		on_surf_points.Append(p2d);
	    }
	}
    } else if (vdist > ldfactor * udist) {
	int isteps = (int)(vdist / udist);
	isteps = (int)(vdist / udist / ldfactor * 2.0);
	fastf_t step = vdist / (fastf_t) isteps;
	fastf_t step_v;
	for (int i = 1; i <= isteps; i++) {
	    step_v = v1 + i * step;
	    if ((left) && (i < isteps)) {
		p2d.Set(u1, step_v);
		on_surf_points.Append(p2d);
	    }

	    if (i == 1) {
		getSurfacePoints(s, u1, u2, v1, v1 + step, min_dist,
				 within_dist, cos_within_ang, on_surf_points, left,
				 below);
	    } else if (i == isteps) {
		getSurfacePoints(s, u1, u2, v2 - step, v2, min_dist,
				 within_dist, cos_within_ang, on_surf_points, left,
				 below);
	    } else {
		getSurfacePoints(s, u1, u2, step_v - step, step_v, min_dist, within_dist,
				 cos_within_ang, on_surf_points, left, below);
	    }

	    below = false;

	    if (i < isteps) {
		//right
		p2d.Set(u2, step_v);
		on_surf_points.Append(p2d);
	    }
	}
    } else if ((surface_EvNormal(s, u1, v1, p[0], norm[0]))
	       && (surface_EvNormal(s, u2, v1, p[1], norm[1])) // for u
	       && (surface_EvNormal(s, u2, v2, p[2], norm[2]))
	       && (surface_EvNormal(s, u1, v2, p[3], norm[3]))
	       && (surface_EvNormal(s, u, v, mid, norm_mid))) {
	double udot;
	double vdot;
	ON_Line line1(p[0], p[2]);
	ON_Line line2(p[1], p[3]);
	double dist = mid.DistanceTo(line1.ClosestPointTo(mid));
	V_MAX(dist, mid.DistanceTo(line2.ClosestPointTo(mid)));

	if (dist < min_dist + ON_ZERO_TOLERANCE) {
	    return;
	}

	if (VNEAR_EQUAL(norm[0], norm[1], ON_ZERO_TOLERANCE)) {
	    udot = 1.0;
	} else {
	    udot = norm[0] * norm[1];
	}
	if (VNEAR_EQUAL(norm[0], norm[3], ON_ZERO_TOLERANCE)) {
	    vdot = 1.0;
	} else {
	    vdot = norm[0] * norm[3];
	}
	if ((udot < cos_within_ang - ON_ZERO_TOLERANCE)
	    && (vdot < cos_within_ang - ON_ZERO_TOLERANCE)) {
	    if (left) {
		p2d.Set(u1, v);
		on_surf_points.Append(p2d);
	    }
	    if (below) {
		p2d.Set(u, v1);
		on_surf_points.Append(p2d);
	    }
	    //center
	    p2d.Set(u, v);
	    on_surf_points.Append(p2d);
	    //right
	    p2d.Set(u2, v);
	    on_surf_points.Append(p2d);
	    //top
	    p2d.Set(u, v2);
	    on_surf_points.Append(p2d);

	    getSurfacePoints(s, u1, u, v1, v, min_dist, within_dist,
			     cos_within_ang, on_surf_points, left, below);
	    getSurfacePoints(s, u1, u, v, v2, min_dist, within_dist,
			     cos_within_ang, on_surf_points, left, false);
	    getSurfacePoints(s, u, u2, v1, v, min_dist, within_dist,
			     cos_within_ang, on_surf_points, false, below);
	    getSurfacePoints(s, u, u2, v, v2, min_dist, within_dist,
			     cos_within_ang, on_surf_points, false, false);
	} else if (udot < cos_within_ang - ON_ZERO_TOLERANCE) {
	    if (below) {
		p2d.Set(u, v1);
		on_surf_points.Append(p2d);
	    }
	    //top
	    p2d.Set(u, v2);
	    on_surf_points.Append(p2d);
	    getSurfacePoints(s, u1, u, v1, v2, min_dist, within_dist,
			     cos_within_ang, on_surf_points, left, below);
	    getSurfacePoints(s, u, u2, v1, v2, min_dist, within_dist,
			     cos_within_ang, on_surf_points, false, below);
	} else if (vdot < cos_within_ang - ON_ZERO_TOLERANCE) {
	    if (left) {
		p2d.Set(u1, v);
		on_surf_points.Append(p2d);
	    }
	    //right
	    p2d.Set(u2, v);
	    on_surf_points.Append(p2d);

	    getSurfacePoints(s, u1, u2, v1, v, min_dist, within_dist,
			     cos_within_ang, on_surf_points, left, below);
	    getSurfacePoints(s, u1, u2, v, v2, min_dist, within_dist,
			     cos_within_ang, on_surf_points, left, false);
	} else {
	    if (left) {
		p2d.Set(u1, v);
		on_surf_points.Append(p2d);
	    }
	    if (below) {
		p2d.Set(u, v1);
		on_surf_points.Append(p2d);
	    }
	    //center
	    p2d.Set(u, v);
	    on_surf_points.Append(p2d);
	    //right
	    p2d.Set(u2, v);
	    on_surf_points.Append(p2d);
	    //top
	    p2d.Set(u, v2);
	    on_surf_points.Append(p2d);

	    if (dist > within_dist + ON_ZERO_TOLERANCE) {

		getSurfacePoints(s, u1, u, v1, v, min_dist, within_dist,
				 cos_within_ang, on_surf_points, left, below);
		getSurfacePoints(s, u1, u, v, v2, min_dist, within_dist,
				 cos_within_ang, on_surf_points, left, false);
		getSurfacePoints(s, u, u2, v1, v, min_dist, within_dist,
				 cos_within_ang, on_surf_points, false, below);
		getSurfacePoints(s, u, u2, v, v2, min_dist, within_dist,
				 cos_within_ang, on_surf_points, false, false);
	    }
	}
    }
}


static void
getSurfacePoints(struct ON_Brep_CDT_State *s_cdt,
	         const ON_BrepFace &face,
		 ON_2dPointArray &on_surf_points)
{
    double surface_width, surface_height;
    const ON_Surface *s = face.SurfaceOf();
    const ON_Brep *brep = face.Brep();

    if (s->GetSurfaceSize(&surface_width, &surface_height)) {
	double dist = 0.0;
	double min_dist = 0.0;
	double within_dist = 0.0;
	double  cos_within_ang = 0.0;

	if ((surface_width < s_cdt->dist) || (surface_height < s_cdt->dist)) {
	    return;
	}

	// may be a smaller trimmed subset of surface so worth getting
	// face boundary
	bool bGrowBox = false;
	ON_3dPoint min, max;
	for (int li = 0; li < face.LoopCount(); li++) {
	    for (int ti = 0; ti < face.Loop(li)->TrimCount(); ti++) {
		const ON_BrepTrim *trim = face.Loop(li)->Trim(ti);
		trim->GetBoundingBox(min, max, bGrowBox);
		bGrowBox = true;
	    }
	}

	ON_BoundingBox tight_bbox;
	if (brep->GetTightBoundingBox(tight_bbox)) {
	    dist = DIST_PT_PT(tight_bbox.m_min, tight_bbox.m_max);
	}

	if (s_cdt->abs < s_cdt->dist + ON_ZERO_TOLERANCE) {
	    min_dist = s_cdt->dist;
	} else {
	    min_dist = s_cdt->abs;
	}

	double rel = 0.0;
	if (s_cdt->rel > 0.0 + ON_ZERO_TOLERANCE) {
	    rel = s_cdt->rel * dist;
	    within_dist = rel < min_dist ? min_dist : rel;
	    //if (s_cdt->abs < s_cdt->dist + ON_ZERO_TOLERANCE) {
	    //    min_dist = within_dist;
	    //}
	} else if ((s_cdt->abs > 0.0 + ON_ZERO_TOLERANCE)
		   && (s_cdt->norm < 0.0 + ON_ZERO_TOLERANCE)) {
	    within_dist = min_dist;
	} else if ((s_cdt->abs > 0.0 + ON_ZERO_TOLERANCE)
		   || (s_cdt->norm > 0.0 + ON_ZERO_TOLERANCE)) {
	    within_dist = dist;
	} else {
	    within_dist = 0.01 * dist; // default to 1% minimum surface distance
	}

	if (s_cdt->norm > 0.0 + ON_ZERO_TOLERANCE) {
	    cos_within_ang = cos(s_cdt->norm);
	} else {
	    cos_within_ang = cos(ON_PI / 2.0);
	}
	ON_BOOL32 uclosed = s->IsClosed(0);
	ON_BOOL32 vclosed = s->IsClosed(1);
	if (uclosed && vclosed) {
	    ON_2dPoint p(0.0, 0.0);
	    double midx = (min.x + max.x) / 2.0;
	    double midy = (min.y + max.y) / 2.0;

	    //bottom left
	    p.Set(min.x, min.y);
	    on_surf_points.Append(p);

	    //midy left
	    p.Set(min.x, midy);
	    on_surf_points.Append(p);

	    getSurfacePoints(s, min.x, midx, min.y, midy, min_dist, within_dist,
			     cos_within_ang, on_surf_points, true, true);

	    //bottom midx
	    p.Set(midx, min.y);
	    on_surf_points.Append(p);

	    //midx midy
	    p.Set(midx, midy);
	    on_surf_points.Append(p);

	    getSurfacePoints(s, midx, max.x, min.y, midy, min_dist, within_dist,
			     cos_within_ang, on_surf_points, false, true);

	    //bottom right
	    p.Set(max.x, min.y);
	    on_surf_points.Append(p);

	    //right  midy
	    p.Set(max.x, midy);
	    on_surf_points.Append(p);

	    //top left
	    p.Set(min.x, max.y);
	    on_surf_points.Append(p);

	    getSurfacePoints(s, min.x, midx, midy, max.y, min_dist, within_dist,
			     cos_within_ang, on_surf_points, true, false);

	    //top midx
	    p.Set(midx, max.y);
	    on_surf_points.Append(p);

	    getSurfacePoints(s, midx, max.x, midy, max.y, min_dist, within_dist,
			     cos_within_ang, on_surf_points, false, false);

	    //top left
	    p.Set(max.x, max.y);
	    on_surf_points.Append(p);
	} else if (uclosed) {
	    ON_2dPoint p(0.0, 0.0);
	    double midx = (min.x + max.x) / 2.0;

	    //bottom left
	    p.Set(min.x, min.y);
	    on_surf_points.Append(p);

	    //top left
	    p.Set(min.x, max.y);
	    on_surf_points.Append(p);

	    getSurfacePoints(s, min.x, midx, min.y, max.y, min_dist,
			     within_dist, cos_within_ang, on_surf_points, true, true);

	    //bottom midx
	    p.Set(midx, min.y);
	    on_surf_points.Append(p);

	    //top midx
	    p.Set(midx, max.y);
	    on_surf_points.Append(p);

	    getSurfacePoints(s, midx, max.x, min.y, max.y, min_dist,
			     within_dist, cos_within_ang, on_surf_points, false, true);

	    //bottom right
	    p.Set(max.x, min.y);
	    on_surf_points.Append(p);

	    //top right
	    p.Set(max.x, max.y);
	    on_surf_points.Append(p);
	} else if (vclosed) {
	    ON_2dPoint p(0.0, 0.0);
	    double midy = (min.y + max.y) / 2.0;

	    //bottom left
	    p.Set(min.x, min.y);
	    on_surf_points.Append(p);

	    //left midy
	    p.Set(min.x, midy);
	    on_surf_points.Append(p);

	    getSurfacePoints(s, min.x, max.x, min.y, midy, min_dist,
			     within_dist, cos_within_ang, on_surf_points, true, true);

	    //bottom right
	    p.Set(max.x, min.y);
	    on_surf_points.Append(p);

	    //right midy
	    p.Set(max.x, midy);
	    on_surf_points.Append(p);

	    getSurfacePoints(s, min.x, max.x, midy, max.y, min_dist,
			     within_dist, cos_within_ang, on_surf_points, true, false);

	    // top left
	    p.Set(min.x, max.y);
	    on_surf_points.Append(p);

	    //top right
	    p.Set(max.x, max.y);
	    on_surf_points.Append(p);
	} else {
	    ON_2dPoint p(0.0, 0.0);

	    //bottom left
	    p.Set(min.x, min.y);
	    on_surf_points.Append(p);

	    //top left
	    p.Set(min.x, max.y);
	    on_surf_points.Append(p);

	    getSurfacePoints(s, min.x, max.x, min.y, max.y, min_dist,
			     within_dist, cos_within_ang, on_surf_points, true, true);

	    //bottom right
	    p.Set(max.x, min.y);
	    on_surf_points.Append(p);

	    //top right
	    p.Set(max.x, max.y);
	    on_surf_points.Append(p);
	}
    }
}


static void
getUVCurveSamples(
	          const ON_Surface *s,
		  const ON_Curve *curve,
		  fastf_t t1,
		  const ON_3dPoint &start_2d,
		  const ON_3dVector &start_tang,
		  const ON_3dPoint &start_3d,
		  const ON_3dVector &start_norm,
		  fastf_t t2,
		  const ON_3dPoint &end_2d,
		  const ON_3dVector &end_tang,
		  const ON_3dPoint &end_3d,
		  const ON_3dVector &end_norm,
		  fastf_t min_dist,
		  fastf_t max_dist,
		  fastf_t within_dist,
		  fastf_t cos_within_ang,
		  std::map<double, ON_3dPoint *> &param_points)
{
    ON_Interval range = curve->Domain();
    ON_3dPoint mid_2d(0.0, 0.0, 0.0);
    ON_3dPoint mid_3d(0.0, 0.0, 0.0);
    ON_3dVector mid_norm(0.0, 0.0, 0.0);
    ON_3dVector mid_tang(0.0, 0.0, 0.0);
    fastf_t t = (t1 + t2) / 2.0;

    if (curve->EvTangent(t, mid_2d, mid_tang)
	&& surface_EvNormal(s, mid_2d.x, mid_2d.y, mid_3d, mid_norm)) {
	ON_Line line3d(start_3d, end_3d);
	double dist3d;

	if ((line3d.Length() > max_dist)
	    || ((dist3d = mid_3d.DistanceTo(line3d.ClosestPointTo(mid_3d)))
		> within_dist + ON_ZERO_TOLERANCE)
	    || ((((start_tang * end_tang)
		  < cos_within_ang - ON_ZERO_TOLERANCE)
		 || ((start_norm * end_norm)
		     < cos_within_ang - ON_ZERO_TOLERANCE))
		&& (dist3d > min_dist + ON_ZERO_TOLERANCE))) {
	    getUVCurveSamples(s, curve, t1, start_2d, start_tang, start_3d, start_norm,
			      t, mid_2d, mid_tang, mid_3d, mid_norm, min_dist, max_dist,
			      within_dist, cos_within_ang, param_points);
	    param_points[(t - range.m_t[0]) / (range.m_t[1] - range.m_t[0])] =
		new ON_3dPoint(mid_3d);
	    getUVCurveSamples(s, curve, t, mid_2d, mid_tang, mid_3d, mid_norm, t2,
			      end_2d, end_tang, end_3d, end_norm, min_dist, max_dist,
			      within_dist, cos_within_ang, param_points);
	}
    }
}


static std::map<double, ON_3dPoint *> *
getUVCurveSamples(struct ON_Brep_CDT_State *s_cdt,
	          const ON_Surface *surf,
		  const ON_Curve *curve,
		  fastf_t max_dist)
{
    fastf_t min_dist, within_dist, cos_within_ang;

    double dist = 1000.0;

    bool bGrowBox = false;
    ON_3dPoint min, max;
    if (curve->GetBoundingBox(min, max, bGrowBox)) {
	dist = DIST_PT_PT(min, max);
    }

    if (s_cdt->abs < s_cdt->dist + ON_ZERO_TOLERANCE) {
	min_dist = s_cdt->dist;
    } else {
	min_dist = s_cdt->abs;
    }

    double rel = 0.0;
    if (s_cdt->rel > 0.0 + ON_ZERO_TOLERANCE) {
	rel = s_cdt->rel * dist;
	if (max_dist < rel * 10.0) {
	    max_dist = rel * 10.0;
	}
	within_dist = rel < min_dist ? min_dist : rel;
    } else if (s_cdt->abs > 0.0 + ON_ZERO_TOLERANCE) {
	within_dist = min_dist;
    } else {
	within_dist = 0.01 * dist; // default to 1% minimum surface distance
    }

    if (s_cdt->norm > 0.0 + ON_ZERO_TOLERANCE) {
	cos_within_ang = cos(s_cdt->norm);
    } else {
	cos_within_ang = cos(ON_PI / 2.0);
    }

    std::map<double, ON_3dPoint *> *param_points = new std::map<double, ON_3dPoint *>();
    ON_Interval range = curve->Domain();

    if (curve->IsClosed()) {
	double mid_range = (range.m_t[0] + range.m_t[1]) / 2.0;
	ON_3dPoint start_2d(0.0, 0.0, 0.0);
	ON_3dPoint start_3d(0.0, 0.0, 0.0);
	ON_3dVector start_tang(0.0, 0.0, 0.0);
	ON_3dVector start_norm(0.0, 0.0, 0.0);
	ON_3dPoint mid_2d(0.0, 0.0, 0.0);
	ON_3dPoint mid_3d(0.0, 0.0, 0.0);
	ON_3dVector mid_tang(0.0, 0.0, 0.0);
	ON_3dVector mid_norm(0.0, 0.0, 0.0);
	ON_3dPoint end_2d(0.0, 0.0, 0.0);
	ON_3dPoint end_3d(0.0, 0.0, 0.0);
	ON_3dVector end_tang(0.0, 0.0, 0.0);
	ON_3dVector end_norm(0.0, 0.0, 0.0);

	if (curve->EvTangent(range.m_t[0], start_2d, start_tang)
	    && curve->EvTangent(mid_range, mid_2d, mid_tang)
	    && curve->EvTangent(range.m_t[1], end_2d, end_tang)
	    && surface_EvNormal(surf, mid_2d.x, mid_2d.y, mid_3d, mid_norm)
	    && surface_EvNormal(surf, start_2d.x, start_2d.y, start_3d, start_norm)
	    && surface_EvNormal(surf, end_2d.x, end_2d.y, end_3d, end_norm))
	{
	    (*param_points)[0.0] = new ON_3dPoint(
		surf->PointAt(curve->PointAt(range.m_t[0]).x,
			      curve->PointAt(range.m_t[0]).y));
	    getUVCurveSamples(surf, curve, range.m_t[0], start_2d, start_tang,
			      start_3d, start_norm, mid_range, mid_2d, mid_tang,
			      mid_3d, mid_norm, min_dist, max_dist, within_dist,
			      cos_within_ang, *param_points);
	    (*param_points)[0.5] = new ON_3dPoint(
		surf->PointAt(curve->PointAt(mid_range).x,
			      curve->PointAt(mid_range).y));
	    getUVCurveSamples(surf, curve, mid_range, mid_2d, mid_tang, mid_3d,
			      mid_norm, range.m_t[1], end_2d, end_tang, end_3d,
			      end_norm, min_dist, max_dist, within_dist,
			      cos_within_ang, *param_points);
	    (*param_points)[1.0] = new ON_3dPoint(
		surf->PointAt(curve->PointAt(range.m_t[1]).x,
			      curve->PointAt(range.m_t[1]).y));
	}
    } else {
	ON_3dPoint start_2d(0.0, 0.0, 0.0);
	ON_3dPoint start_3d(0.0, 0.0, 0.0);
	ON_3dVector start_tang(0.0, 0.0, 0.0);
	ON_3dVector start_norm(0.0, 0.0, 0.0);
	ON_3dPoint end_2d(0.0, 0.0, 0.0);
	ON_3dPoint end_3d(0.0, 0.0, 0.0);
	ON_3dVector end_tang(0.0, 0.0, 0.0);
	ON_3dVector end_norm(0.0, 0.0, 0.0);

	if (curve->EvTangent(range.m_t[0], start_2d, start_tang)
	    && curve->EvTangent(range.m_t[1], end_2d, end_tang)
	    && surface_EvNormal(surf, start_2d.x, start_2d.y, start_3d, start_norm)
	    && surface_EvNormal(surf, end_2d.x, end_2d.y, end_3d, end_norm))
	{
	    (*param_points)[0.0] = new ON_3dPoint(start_3d);
	    getUVCurveSamples(surf, curve, range.m_t[0], start_2d, start_tang,
			      start_3d, start_norm, range.m_t[1], end_2d, end_tang,
			      end_3d, end_norm, min_dist, max_dist, within_dist,
			      cos_within_ang, *param_points);
	    (*param_points)[1.0] = new ON_3dPoint(end_3d);
	}
    }


    return param_points;
}


/*
 * number_of_seam_crossings
 */
static int
number_of_seam_crossings(const ON_Surface *surf,  ON_SimpleArray<BrepTrimPoint> &brep_trim_points)
{
    int rc = 0;
    const ON_2dPoint *prev_non_seam_pt = NULL;
    for (int i = 0; i < brep_trim_points.Count(); i++) {
	const ON_2dPoint *pt = &brep_trim_points[i].p2d;
	if (!IsAtSeam(surf, *pt, BREP_SAME_POINT_TOLERANCE)) {
	    int udir = 0;
	    int vdir = 0;
	    if (prev_non_seam_pt != NULL) {
		if (ConsecutivePointsCrossClosedSeam(surf, *prev_non_seam_pt, *pt, udir, vdir, BREP_SAME_POINT_TOLERANCE)) {
		    rc++;
		}
	    }
	    prev_non_seam_pt = pt;
	}
    }

    return rc;
}


static bool
LoopStraddlesDomain(const ON_Surface *surf,  ON_SimpleArray<BrepTrimPoint> &brep_loop_points)
{
    if (surf->IsClosed(0) || surf->IsClosed(1)) {
	int num_crossings = number_of_seam_crossings(surf, brep_loop_points);
	if (num_crossings == 1) {
	    return true;
	}
    }
    return false;
}


/*
 * entering - 1
 * exiting - 2
 * contained - 0
 */
static int
is_entering(const ON_Surface *surf,  const ON_SimpleArray<BrepTrimPoint> &brep_loop_points)
{
    int numpoints = brep_loop_points.Count();
    for (int i = 1; i < numpoints - 1; i++) {
	int seam = 0;
	ON_2dPoint p = brep_loop_points[i].p2d;
	if ((seam = IsAtSeam(surf, p, BREP_SAME_POINT_TOLERANCE)) > 0) {
	    ON_2dPoint unwrapped = UnwrapUVPoint(surf, p, BREP_SAME_POINT_TOLERANCE);
	    if (seam == 1) {
		bool right_seam = unwrapped.x > surf->Domain(0).Mid();
		bool decreasing = (brep_loop_points[numpoints - 1].p2d.x - brep_loop_points[0].p2d.x) < 0;
		if (right_seam != decreasing) { // basically XOR'ing here
		    return 2;
		} else {
		    return 1;
		}
	    } else {
		bool top_seam = unwrapped.y > surf->Domain(1).Mid();
		bool decreasing = (brep_loop_points[numpoints - 1].p2d.y - brep_loop_points[0].p2d.y) < 0;
		if (top_seam != decreasing) { // basically XOR'ing here
		    return 2;
		} else {
		    return 1;
		}
	    }
	}
    }
    return 0;
}

/*
 * shift_closed_curve_split_over_seam
 */
static bool
shift_loop_straddled_over_seam(const ON_Surface *surf,  ON_SimpleArray<BrepTrimPoint> &brep_loop_points, double same_point_tolerance)
{
    if (surf->IsClosed(0) || surf->IsClosed(1)) {
	ON_Interval dom[2];
	int entering = is_entering(surf, brep_loop_points);

	dom[0] = surf->Domain(0);
	dom[1] = surf->Domain(1);

	int seam = 0;
	int i;
	BrepTrimPoint btp;
	BrepTrimPoint end_btp;
	ON_SimpleArray<BrepTrimPoint> part1;
	ON_SimpleArray<BrepTrimPoint> part2;

	end_btp.p2d = ON_2dPoint::UnsetPoint;
	int numpoints = brep_loop_points.Count();
	bool first_seam_pt = true;
	for (i = 0; i < numpoints; i++) {
	    btp = brep_loop_points[i];
	    if ((seam = IsAtSeam(surf, btp.p2d, same_point_tolerance)) > 0) {
		if (first_seam_pt) {
		    part1.Append(btp);
		    first_seam_pt = false;
		}
		end_btp = btp;
		SwapUVSeamPoint(surf, end_btp.p2d);
		part2.Append(end_btp);
	    } else {
		if (dom[0].Includes(btp.p2d.x, false) && dom[1].Includes(btp.p2d.y, false)) {
		    part1.Append(brep_loop_points[i]);
		} else {
		    btp = brep_loop_points[i];
		    btp.p2d = UnwrapUVPoint(surf, brep_loop_points[i].p2d, same_point_tolerance);
		    part2.Append(btp);
		}
	    }
	}

	brep_loop_points.Empty();
	if (entering == 1) {
	    brep_loop_points.Append(part1.Count() - 1, part1.Array());
	    brep_loop_points.Append(part2.Count(), part2.Array());
	} else {
	    brep_loop_points.Append(part2.Count() - 1, part2.Array());
	    brep_loop_points.Append(part1.Count(), part1.Array());
	}
    }
    return true;
}


/*
 * extend_over_seam_crossings
 */
static bool
extend_over_seam_crossings(const ON_Surface *surf,  ON_SimpleArray<BrepTrimPoint> &brep_loop_points)
{
    int num_points = brep_loop_points.Count();
    double ulength = surf->Domain(0).Length();
    double vlength = surf->Domain(1).Length();
    for (int i = 1; i < num_points; i++) {
	if (surf->IsClosed(0)) {
	    double delta = brep_loop_points[i].p2d.x - brep_loop_points[i - 1].p2d.x;
	    while (fabs(delta) > ulength / 2.0) {
		if (delta < 0.0) {
		    brep_loop_points[i].p2d.x += ulength; // east bound
		} else {
		    brep_loop_points[i].p2d.x -= ulength;; // west bound
		}
		delta = brep_loop_points[i].p2d.x - brep_loop_points[i - 1].p2d.x;
	    }
	}
	if (surf->IsClosed(1)) {
	    double delta = brep_loop_points[i].p2d.y - brep_loop_points[i - 1].p2d.y;
	    while (fabs(delta) > vlength / 2.0) {
		if (delta < 0.0) {
		    brep_loop_points[i].p2d.y += vlength; // north bound
		} else {
		    brep_loop_points[i].p2d.y -= vlength;; // south bound
		}
		delta = brep_loop_points[i].p2d.y - brep_loop_points[i - 1].p2d.y;
	    }
	}
    }

    return true;
}

static void
get_loop_sample_points(
	struct ON_Brep_CDT_State *s,
	ON_SimpleArray<BrepTrimPoint> *points,
	const ON_BrepFace &face,
	const ON_BrepLoop *loop,
	fastf_t max_dist,
	std::map<int, ON_3dPoint *> *vert_pnts,
	std::vector<ON_3dPoint *> *w3dpnts,
	std::map<int, ON_3dPoint *> *vert_norms,
	std::vector<ON_3dPoint *> *w3dnorms
	)
{
    int trim_count = loop->TrimCount();

    for (int lti = 0; lti < trim_count; lti++) {
	ON_BrepTrim *trim = loop->Trim(lti);
	ON_BrepEdge *edge = trim->Edge();

	if (trim->m_type == ON_BrepTrim::singular) {
	    BrepTrimPoint btp;
	    const ON_BrepVertex& v1 = face.Brep()->m_V[trim->m_vi[0]];
	    ON_3dPoint *p3d = new ON_3dPoint(v1.Point());
	    //ON_2dPoint p2d_begin = trim->PointAt(trim->Domain().m_t[0]);
	    //ON_2dPoint p2d_end = trim->PointAt(trim->Domain().m_t[1]);
	    double delta =  trim->Domain().Length() / 10.0;
	    ON_Interval trim_dom = trim->Domain();

	    for (int i = 1; i <= 10; i++) {
		btp.p3d = p3d;
		btp.p2d = v1.Point();
		btp.t = trim->Domain().m_t[0] + (i - 1) * delta;
		btp.p2d = trim->PointAt(btp.t);
		btp.e = ON_UNSET_VALUE;
		points->Append(btp);
	    }
	    // skip last point of trim if not last trim
	    if (lti < trim_count - 1)
		continue;

	    const ON_BrepVertex& v2 = face.Brep()->m_V[trim->m_vi[1]];
	    btp.p3d = p3d;
	    btp.p2d = v2.Point();
	    btp.t = trim->Domain().m_t[1];
	    btp.p2d = trim->PointAt(btp.t);
	    btp.e = ON_UNSET_VALUE;
	    points->Append(btp);

	    continue;
	}

	if (!trim->m_trim_user.p) {
	    (void)getEdgePoints(s, edge, *trim, max_dist, vert_pnts, w3dpnts, vert_norms, w3dnorms);
	    //bu_log("Initialized trim->m_trim_user.p: Trim %d (associated with Edge %d) point count: %zd\n", trim->m_trim_index, trim->Edge()->m_edge_index, m->size());
	}
	if (trim->m_trim_user.p) {
	    std::map<double, BrepTrimPoint *> *param_points3d = (std::map<double, BrepTrimPoint *> *) trim->m_trim_user.p;
	    //bu_log("Trim %d (associated with Edge %d) point count: %zd\n", trim->m_trim_index, trim->Edge()->m_edge_index, param_points3d->size());

	    ON_3dPoint boxmin;
	    ON_3dPoint boxmax;

	    if (trim->GetBoundingBox(boxmin, boxmax, false)) {
		double t0, t1;

		std::map<double, BrepTrimPoint*>::const_iterator i;
		std::map<double, BrepTrimPoint*>::const_iterator ni;

		trim->GetDomain(&t0, &t1);
		for (i = param_points3d->begin(); i != param_points3d->end();) {
		    BrepTrimPoint *btp = (*i).second;
		    ni = ++i;
		    // skip last point of trim if not last trim
		    if (ni == param_points3d->end()) {
			if (lti < trim_count - 1) {
			    continue;
			}
		    }
		    points->Append(*btp);
		}
	    }
	}
    }
}


/* force near seam points to seam */
static void
ForceNearSeamPointsToSeam(
	const ON_Surface *s,
	const ON_BrepFace &face,
	ON_SimpleArray<BrepTrimPoint> *brep_loop_points,
	double same_point_tolerance)
{
    int loop_cnt = face.LoopCount();
    for (int li = 0; li < loop_cnt; li++) {
	int num_loop_points = brep_loop_points[li].Count();
	if (num_loop_points > 1) {
	    for (int i = 0; i < num_loop_points; i++) {
		ON_2dPoint &p = brep_loop_points[li][i].p2d;
		if (IsAtSeam(s, p, same_point_tolerance)) {
		    ForceToClosestSeam(s, p, same_point_tolerance);
		}
	    }
	}
    }
}


static void
ExtendPointsOverClosedSeam(
	const ON_Surface *s,
	const ON_BrepFace &face,
	ON_SimpleArray<BrepTrimPoint> *brep_loop_points)
{
    int loop_cnt = face.LoopCount();
    // extend loop points over seam if needed.
    for (int li = 0; li < loop_cnt; li++) {
	int num_loop_points = brep_loop_points[li].Count();
	if (num_loop_points > 1) {
	    if (!extend_over_seam_crossings(s, brep_loop_points[li])) {
		std::cerr << "Error: Face(" << face.m_face_index << ") cannot extend loops over closed seams." << std::endl;
	    }
	}
    }
}


// process through loops checking for straddle condition.
static void
ShiftLoopsThatStraddleSeam(
	const ON_Surface *s,
	const ON_BrepFace &face,
	ON_SimpleArray<BrepTrimPoint> *brep_loop_points,
	double same_point_tolerance)
{
    int loop_cnt = face.LoopCount();
    for (int li = 0; li < loop_cnt; li++) {
	int num_loop_points = brep_loop_points[li].Count();
	if (num_loop_points > 1) {
	    ON_2dPoint brep_loop_begin = brep_loop_points[li][0].p2d;
	    ON_2dPoint brep_loop_end = brep_loop_points[li][num_loop_points - 1].p2d;

	    if (!V2NEAR_EQUAL(brep_loop_begin, brep_loop_end, same_point_tolerance)) {
		if (LoopStraddlesDomain(s, brep_loop_points[li])) {
		    // reorder loop points
		    shift_loop_straddled_over_seam(s, brep_loop_points[li], same_point_tolerance);
		}
	    }
	}
    }
}


// process through closing open loops that begin and end on closed seam
static void
CloseOpenLoops(
	struct ON_Brep_CDT_State *s_cdt,
	const ON_Surface *s,
	const ON_BrepFace &face,
	ON_SimpleArray<BrepTrimPoint> *brep_loop_points,
	double same_point_tolerance)
{
    std::list<std::map<double, ON_3dPoint *> *> bridgePoints;
    int loop_cnt = face.LoopCount();
    for (int li = 0; li < loop_cnt; li++) {
	int num_loop_points = brep_loop_points[li].Count();
	if (num_loop_points > 1) {
	    ON_2dPoint brep_loop_begin = brep_loop_points[li][0].p2d;
	    ON_2dPoint brep_loop_end = brep_loop_points[li][num_loop_points - 1].p2d;
	    ON_3dPoint brep_loop_begin3d = s->PointAt(brep_loop_begin.x, brep_loop_begin.y);
	    ON_3dPoint brep_loop_end3d = s->PointAt(brep_loop_end.x, brep_loop_end.y);

	    if (!V2NEAR_EQUAL(brep_loop_begin, brep_loop_end, same_point_tolerance) &&
		VNEAR_EQUAL(brep_loop_begin3d, brep_loop_end3d, same_point_tolerance)) {
		int seam_begin = 0;
		int seam_end = 0;
		if ((seam_begin = IsAtSeam(s, brep_loop_begin, same_point_tolerance)) &&
		    (seam_end = IsAtSeam(s, brep_loop_end, same_point_tolerance))) {
		    bool loop_not_closed = true;
		    if ((li + 1) < loop_cnt) {
			// close using remaining loops
			for (int rli = li + 1; rli < loop_cnt; rli++) {
			    int rnum_loop_points = brep_loop_points[rli].Count();
			    ON_2dPoint rbrep_loop_begin = brep_loop_points[rli][0].p2d;
			    ON_2dPoint rbrep_loop_end = brep_loop_points[rli][rnum_loop_points - 1].p2d;
			    if (!V2NEAR_EQUAL(rbrep_loop_begin, rbrep_loop_end, same_point_tolerance)) {
				if (IsAtSeam(s, rbrep_loop_begin, same_point_tolerance) && IsAtSeam(s, rbrep_loop_end, same_point_tolerance)) {
				    double t0, t1;
				    ON_LineCurve line1(brep_loop_end, rbrep_loop_begin);
				    std::map<double, ON_3dPoint *> *linepoints3d = getUVCurveSamples(s_cdt, s, &line1, 1000.0);
				    bridgePoints.push_back(linepoints3d);
				    line1.GetDomain(&t0, &t1);
				    std::map<double, ON_3dPoint*>::const_iterator i;
				    for (i = linepoints3d->begin();
					 i != linepoints3d->end(); i++) {
					BrepTrimPoint btp;

					// skips first point
					if (i == linepoints3d->begin())
					    continue;

					btp.t = (*i).first;
					btp.p3d = (*i).second;
					btp.p2d = line1.PointAt(t0 + (t1 - t0) * btp.t);
					btp.e = ON_UNSET_VALUE;
					brep_loop_points[li].Append(btp);
				    }
				    //brep_loop_points[li].Append(brep_loop_points[rli].Count(),brep_loop_points[rli].Array());
				    for (int j = 1; j < rnum_loop_points; j++) {
					brep_loop_points[li].Append(brep_loop_points[rli][j]);
				    }
				    ON_LineCurve line2(rbrep_loop_end, brep_loop_begin);
				    linepoints3d = getUVCurveSamples(s_cdt, s, &line2, 1000.0);
				    bridgePoints.push_back(linepoints3d);
				    line2.GetDomain(&t0, &t1);

				    for (i = linepoints3d->begin();
					 i != linepoints3d->end(); i++) {
					BrepTrimPoint btp;
					// skips first point
					if (i == linepoints3d->begin())
					    continue;

					btp.t = (*i).first;
					btp.p3d = (*i).second;
					btp.p2d = line2.PointAt(t0 + (t1 - t0) * btp.t);
					btp.e = ON_UNSET_VALUE;
					brep_loop_points[li].Append(btp);
				    }
				    brep_loop_points[rli].Empty();
				    loop_not_closed = false;
				}
			    }
			}
		    }
		    if (loop_not_closed) {
			// no matching loops found that would close so use domain boundary
			ON_Interval u = s->Domain(0);
			ON_Interval v = s->Domain(1);
			if (seam_end == 1) {
			    if (NEAR_EQUAL(brep_loop_end.x, u.m_t[0], same_point_tolerance)) {
				// low end so decreasing

				// now where do we have to close to
				if (seam_begin == 1) {
				    // has to be on opposite seam
				    double t0, t1;
				    ON_2dPoint p = brep_loop_end;
				    p.y = v.m_t[0];
				    ON_LineCurve line1(brep_loop_end, p);
				    std::map<double, ON_3dPoint *> *linepoints3d = getUVCurveSamples(s_cdt, s, &line1, 1000.0);
				    bridgePoints.push_back(linepoints3d);
				    line1.GetDomain(&t0, &t1);
				    std::map<double, ON_3dPoint*>::const_iterator i;
				    for (i = linepoints3d->begin();
					 i != linepoints3d->end(); i++) {
					BrepTrimPoint btp;

					// skips first point
					if (i == linepoints3d->begin())
					    continue;

					btp.t = (*i).first;
					btp.p3d = (*i).second;
					btp.p2d = line1.PointAt(t0 + (t1 - t0) * btp.t);
					btp.e = ON_UNSET_VALUE;
					brep_loop_points[li].Append(btp);
				    }
				    line1.SetStartPoint(p);
				    p.x = u.m_t[1];
				    line1.SetEndPoint(p);
				    linepoints3d = getUVCurveSamples(s_cdt, s, &line1, 1000.0);
				    bridgePoints.push_back(linepoints3d);
				    line1.GetDomain(&t0, &t1);
				    for (i = linepoints3d->begin();
					 i != linepoints3d->end(); i++) {
					BrepTrimPoint btp;

					// skips first point
					if (i == linepoints3d->begin())
					    continue;

					btp.t = (*i).first;
					btp.p3d = (*i).second;
					btp.p2d = line1.PointAt(t0 + (t1 - t0) * btp.t);
					btp.e = ON_UNSET_VALUE;
					brep_loop_points[li].Append(btp);
				    }
				    line1.SetStartPoint(p);
				    line1.SetEndPoint(brep_loop_begin);
				    linepoints3d = getUVCurveSamples(s_cdt, s, &line1, 1000.0);
				    bridgePoints.push_back(linepoints3d);
				    line1.GetDomain(&t0, &t1);
				    for (i = linepoints3d->begin();
					 i != linepoints3d->end(); i++) {
					BrepTrimPoint btp;

					// skips first point
					if (i == linepoints3d->begin())
					    continue;

					btp.t = (*i).first;
					btp.p3d = (*i).second;
					btp.p2d = line1.PointAt(t0 + (t1 - t0) * btp.t);
					btp.e = ON_UNSET_VALUE;
					brep_loop_points[li].Append(btp);
				    }

				} else if (seam_begin == 2) {

				} else {
				    //both needed
				}

			    } else { //assume on other end
				// high end so increasing
				// now where do we have to close to
				if (seam_begin == 1) {
				    // has to be on opposite seam
				    double t0, t1;
				    ON_2dPoint p = brep_loop_end;
				    p.y = v.m_t[1];
				    ON_LineCurve line1(brep_loop_end, p);
				    std::map<double, ON_3dPoint *> *linepoints3d = getUVCurveSamples(s_cdt, s, &line1, 1000.0);
				    bridgePoints.push_back(linepoints3d);
				    line1.GetDomain(&t0, &t1);
				    std::map<double, ON_3dPoint*>::const_iterator i;
				    for (i = linepoints3d->begin();
					 i != linepoints3d->end(); i++) {
					BrepTrimPoint btp;

					// skips first point
					if (i == linepoints3d->begin())
					    continue;

					btp.t = (*i).first;
					btp.p3d = (*i).second;
					btp.p2d = line1.PointAt(t0 + (t1 - t0) * btp.t);
					btp.e = ON_UNSET_VALUE;
					brep_loop_points[li].Append(btp);
				    }
				    line1.SetStartPoint(p);
				    p.x = u.m_t[0];
				    line1.SetEndPoint(p);
				    linepoints3d = getUVCurveSamples(s_cdt, s, &line1, 1000.0);
				    bridgePoints.push_back(linepoints3d);
				    line1.GetDomain(&t0, &t1);
				    for (i = linepoints3d->begin();
					 i != linepoints3d->end(); i++) {
					BrepTrimPoint btp;

					// skips first point
					if (i == linepoints3d->begin())
					    continue;

					btp.t = (*i).first;
					btp.p3d = (*i).second;
					btp.p2d = line1.PointAt(t0 + (t1 - t0) * btp.t);
					btp.e = ON_UNSET_VALUE;
					brep_loop_points[li].Append(btp);
				    }
				    line1.SetStartPoint(p);
				    line1.SetEndPoint(brep_loop_begin);
				    linepoints3d = getUVCurveSamples(s_cdt, s, &line1, 1000.0);
				    bridgePoints.push_back(linepoints3d);
				    line1.GetDomain(&t0, &t1);
				    for (i = linepoints3d->begin();
					 i != linepoints3d->end(); i++) {
					BrepTrimPoint btp;

					// skips first point
					if (i == linepoints3d->begin())
					    continue;

					btp.t = (*i).first;
					btp.p3d = (*i).second;
					btp.p2d = line1.PointAt(t0 + (t1 - t0) * btp.t);
					btp.e = ON_UNSET_VALUE;
					brep_loop_points[li].Append(btp);
				    }
				} else if (seam_begin == 2) {

				} else {
				    //both
				}
			    }
			} else if (seam_end == 2) {
			    if (NEAR_EQUAL(brep_loop_end.y, v.m_t[0], same_point_tolerance)) {

			    } else { //assume on other end

			    }
			} else {
			    //both
			}
		    }
		}
	    }
	}
    }
}


/*
 * lifted from Clipper private function and rework for brep_loop_points
 */
static bool
PointInPolygon(const ON_2dPoint &pt, ON_SimpleArray<BrepTrimPoint> &brep_loop_points)
{
    bool result = false;

    for( int i = 0; i < brep_loop_points.Count(); i++){
	ON_2dPoint curr_pt = brep_loop_points[i].p2d;
	ON_2dPoint prev_pt = ON_2dPoint::UnsetPoint;
	if (i == 0) {
	    prev_pt = brep_loop_points[brep_loop_points.Count()-1].p2d;
	} else {
	    prev_pt = brep_loop_points[i-1].p2d;
	}
	if ((((curr_pt.y <= pt.y) && (pt.y < prev_pt.y)) ||
		((prev_pt.y <= pt.y) && (pt.y < curr_pt.y))) &&
		(pt.x < (prev_pt.x - curr_pt.x) * (pt.y - curr_pt.y) /
			(prev_pt.y - curr_pt.y) + curr_pt.x ))
	    result = !result;
    }
    return result;
}


static void
ShiftPoints(ON_SimpleArray<BrepTrimPoint> &brep_loop_points, double ushift, double vshift)
{
    for( int i = 0; i < brep_loop_points.Count(); i++){
	brep_loop_points[i].p2d.x += ushift;
	brep_loop_points[i].p2d.y += vshift;
    }
}


// process through to make sure inner hole loops are actually inside of outer polygon
// need to make sure that any hole polygons are properly shifted over correct closed seams
// going to try and do an inside test on hole vertex
static void
ShiftInnerLoops(
	const ON_Surface *s,
	const ON_BrepFace &face,
	ON_SimpleArray<BrepTrimPoint> *brep_loop_points)
{
    int loop_cnt = face.LoopCount();
    if (loop_cnt > 1) { // has inner loops or holes
	for( int li = 1; li < loop_cnt; li++) {
	    ON_2dPoint p2d((brep_loop_points[li])[0].p2d.x, (brep_loop_points[li])[0].p2d.y);
	    if (!PointInPolygon(p2d, brep_loop_points[0])) {
		double ulength = s->Domain(0).Length();
		double vlength = s->Domain(1).Length();
		ON_2dPoint sftd_pt = p2d;

		//do shift until inside
		if (s->IsClosed(0) && s->IsClosed(1)) {
		    // First just U
		    for(int iu = 0; iu < 2; iu++) {
			double ushift = 0.0;
			if (iu == 0) {
			    ushift = -ulength;
			} else {
			    ushift =  ulength;
			}
			sftd_pt.x = p2d.x + ushift;
			if (PointInPolygon(sftd_pt, brep_loop_points[0])) {
			    // shift all U accordingly
			    ShiftPoints(brep_loop_points[li], ushift, 0.0);
			    break;
			}
		    }
		    // Second just V
		    for(int iv = 0; iv < 2; iv++) {
			double vshift = 0.0;
			if (iv == 0) {
			    vshift = -vlength;
			} else {
			    vshift = vlength;
			}
			sftd_pt.y = p2d.y + vshift;
			if (PointInPolygon(sftd_pt, brep_loop_points[0])) {
			    // shift all V accordingly
			    ShiftPoints(brep_loop_points[li], 0.0, vshift);
			    break;
			}
		    }
		    // Third both U & V
		    for(int iu = 0; iu < 2; iu++) {
			double ushift = 0.0;
			if (iu == 0) {
			    ushift = -ulength;
			} else {
			    ushift =  ulength;
			}
			sftd_pt.x = p2d.x + ushift;
			for(int iv = 0; iv < 2; iv++) {
			    double vshift = 0.0;
			    if (iv == 0) {
				vshift = -vlength;
			    } else {
				vshift = vlength;
			    }
			    sftd_pt.y = p2d.y + vshift;
			    if (PointInPolygon(sftd_pt, brep_loop_points[0])) {
				// shift all U & V accordingly
				ShiftPoints(brep_loop_points[li], ushift, vshift);
				break;
			    }
			}
		    }
		} else if (s->IsClosed(0)) {
		    // just U
		    for(int iu = 0; iu < 2; iu++) {
			double ushift = 0.0;
			if (iu == 0) {
			    ushift = -ulength;
			} else {
			    ushift =  ulength;
			}
			sftd_pt.x = p2d.x + ushift;
			if (PointInPolygon(sftd_pt, brep_loop_points[0])) {
			    // shift all U accordingly
			    ShiftPoints(brep_loop_points[li], ushift, 0.0);
			    break;
			}
		    }
		} else if (s->IsClosed(1)) {
		    // just V
		    for(int iv = 0; iv < 2; iv++) {
			double vshift = 0.0;
			if (iv == 0) {
			    vshift = -vlength;
			} else {
			    vshift = vlength;
			}
			sftd_pt.y = p2d.y + vshift;
			if (PointInPolygon(sftd_pt, brep_loop_points[0])) {
			    // shift all V accordingly
			    ShiftPoints(brep_loop_points[li], 0.0, vshift);
			    break;
			}
		    }
		}
	    }
	}
    }
}


static void
PerformClosedSurfaceChecks(
	struct ON_Brep_CDT_State *s_cdt,
	const ON_Surface *s,
	const ON_BrepFace &face,
	ON_SimpleArray<BrepTrimPoint> *brep_loop_points,
	double same_point_tolerance)
{
    // force near seam points to seam.
    ForceNearSeamPointsToSeam(s, face, brep_loop_points, same_point_tolerance);

    // extend loop points over closed seam if needed.
    ExtendPointsOverClosedSeam(s, face, brep_loop_points);

    // shift open loops that straddle a closed seam with the intent of closure at the surface boundary.
    ShiftLoopsThatStraddleSeam(s, face, brep_loop_points, same_point_tolerance);

    // process through closing open loops that begin and end on closed seam
    CloseOpenLoops(s_cdt, s, face, brep_loop_points, same_point_tolerance);

    // process through to make sure inner hole loops are actually inside of outer polygon
    // need to make sure that any hole polygons are properly shifted over correct closed seams
    ShiftInnerLoops(s, face, brep_loop_points);
}



struct ON_Brep_CDT_State *
ON_Brep_CDT_Create(ON_Brep *brep)
{
    struct ON_Brep_CDT_State *cdt = new struct ON_Brep_CDT_State;
    cdt->brep = brep;

    cdt->w3dpnts = new std::vector<ON_3dPoint *>;
    cdt->vert_pnts = new std::map<int, ON_3dPoint *>;
    cdt->w3dnorms = new std::vector<ON_3dPoint *>;
    cdt->vert_norms = new std::map<int, ON_3dPoint *>;

    cdt->p2t_faces = (p2t::CDT **)bu_calloc(brep->m_F.Count(), sizeof(p2t::CDT *), "poly2tri triangulations");
    cdt->p2t_maps = (std::map<p2t::Point *, ON_3dPoint *> **)bu_calloc(brep->m_F.Count(), sizeof(std::map<p2t::Point *, ON_3dPoint *> *), "poly2tri point to ON_3dPoint maps");
    cdt->p2t_nmaps = (std::map<p2t::Point *, ON_3dPoint *> **)bu_calloc(brep->m_F.Count(), sizeof(std::map<p2t::Point *, ON_3dPoint *> *), "poly2tri point to ON_3dVector normal maps");

    /* Set status to "never evaluated" */
    cdt->status = BREP_CDT_UNTESSELLATED;

    /* TODO - Set default tolerances */

    return cdt;
}

void
ON_Brep_CDT_Destroy(struct ON_Brep_CDT_State *s)
{
    for (size_t i = 0; i < s->w3dpnts->size(); i++) {
	delete (*(s->w3dpnts))[i];
    }
    for (size_t i = 0; i < s->w3dnorms->size(); i++) {
	delete (*(s->w3dnorms))[i];
    }

    delete s->w3dpnts;
    delete s->vert_pnts;
    delete s->w3dnorms;
    delete s->vert_norms;

    // TODO - delete p2t data

    delete s;
}

int ON_Brep_CDT_Tessellate(struct ON_Brep_CDT_State *s_cdt, std::vector<int> *faces)
{

    ON_wString wstr;
    ON_TextLog tl(wstr);

    ON_Brep* brep = s_cdt->brep;

    // Check for any conditions that are show-stoppers
    for (int index = 0; index < brep->m_E.Count(); index++) {
        ON_BrepEdge& edge = brep->m_E[index];
        if (edge.TrimCount() != 2) {
            bu_log("Edge %d trim count: %d - can't (yet) do watertight meshing\n", edge.m_edge_index, edge.TrimCount());
            return -1;
        }
        ON_wString wonstr;
        ON_TextLog vout(wonstr);
        if (!brep->IsValid(&vout)) {
            bu_log("brep is NOT valid, cannot produce watertight mesh\n");
            //return -1;
        }
    }

    // Reparameterize the face's surface and transform the "u" and "v"
    // coordinates of all the face's parameter space trimming curves to
    // minimize distortion in the map from parameter space to 3d..
    for (int face_index = 0; face_index < brep->m_F.Count(); face_index++) {
        ON_BrepFace *face = brep->Face(face_index);
        const ON_Surface *s = face->SurfaceOf();
        double surface_width, surface_height;
        if (s->GetSurfaceSize(&surface_width, &surface_height)) {
            face->SetDomain(0, 0.0, surface_width);
            face->SetDomain(1, 0.0, surface_height);
        }
    }

    /* We want to use ON_3dPoint pointers and BrepVertex points, but
     * vert->Point() produces a temporary address.  If this is our first time
     * through, make stable copies of the Vertex points. */
    if (!s_cdt->w3dpnts->size()) {
	for (int index = 0; index < brep->m_V.Count(); index++) {
	    ON_BrepVertex& v = brep->m_V[index];
	    (*s_cdt->vert_pnts)[index] = new ON_3dPoint(v.Point());
	    s_cdt->w3dpnts->push_back((*s_cdt->vert_pnts)[index]);
	}
    }

    /* If this is the first time through, get vertex normals that are the
     * average of the surface points at the junction.
     *
     * TODO - See if we can reject a normal in the "wrong" direction if two or
     * more surfaces vote one way and a third votes the other... */
    if (!s_cdt->w3dnorms->size()) {
	for (int index = 0; index < brep->m_V.Count(); index++) {
	    ON_BrepVertex& v = brep->m_V[index];
	    ON_3dVector vnrml(0.0, 0.0, 0.0);

	    for (int eind = 0; eind != v.EdgeCount(); eind++) {
		ON_3dPoint tmp1, tmp2;
		ON_3dVector trim1_norm = ON_3dVector::UnsetVector;
		ON_3dVector trim2_norm = ON_3dVector::UnsetVector;
		ON_BrepEdge& edge = brep->m_E[v.m_ei[eind]];
		ON_BrepTrim *trim1 = edge.Trim(0);
		ON_BrepTrim *trim2 = edge.Trim(1);
		ON_Interval t1range = trim1->Domain();
		ON_Interval t2range = trim2->Domain();
		double t1 = ((eind && trim1->m_bRev3d) || (!eind && !trim1->m_bRev3d)) ? t1range[0] : t1range[1];
		double t2 = ((eind && trim2->m_bRev3d) || (!eind && !trim2->m_bRev3d)) ? t2range[0] : t2range[1];
		ON_3dPoint t1_2d = trim1->PointAt(t1);
		ON_3dPoint t2_2d = trim2->PointAt(t2);
		const ON_Surface *s1 = trim1->SurfaceOf();
		const ON_Surface *s2 = trim2->SurfaceOf();
		if (surface_EvNormal(s1, t1_2d.x, t1_2d.y, tmp1, trim1_norm)) {
		    if (trim1->Face()->m_bRev) {
			trim1_norm = trim1_norm * -1;
		    }
		    vnrml += trim1_norm;
		}
		if (surface_EvNormal(s2, t2_2d.x, t2_2d.y, tmp2, trim2_norm)) {
		    if (trim2->Face()->m_bRev) {
			trim2_norm = trim2_norm * -1;
		    }
		    vnrml += trim2_norm;
		}
	    }
	    vnrml.Unitize();

	    // We store this as a point to keep C++ happy...  If we try to
	    // propagate the ON_3dVector type through all the CDT logic it
	    // triggers issues with the compile.
	    (*s_cdt->vert_norms)[index] = new ON_3dPoint(vnrml);
	    s_cdt->w3dnorms->push_back((*s_cdt->vert_norms)[index]);
	}
    }



    /* To generate watertight meshes, the faces must share 3D edge points.  To ensure
     * a uniform set of edge points, we first sample all the edges and build their
     * point sets */
    std::map<const ON_Surface *, double> s_to_maxdist;

    if (!faces) {
	for (int face_index = 0; face_index < s_cdt->brep->m_F.Count(); face_index++) {
	    ON_BrepFace &face = s_cdt->brep->m_F[face_index];
	    const ON_Surface *s = face.SurfaceOf();
	    int loop_cnt = face.LoopCount();
	    ON_SimpleArray<BrepTrimPoint> *brep_loop_points = new ON_SimpleArray<BrepTrimPoint>[loop_cnt];

	    // first simply load loop point samples
	    for (int li = 0; li < loop_cnt; li++) {
		double max_dist = 0.0;
		const ON_BrepLoop *loop = face.Loop(li);
		if (s_to_maxdist.find(face.SurfaceOf()) != s_to_maxdist.end()) {
		    max_dist = s_to_maxdist[face.SurfaceOf()];
		}
		get_loop_sample_points(s_cdt, &brep_loop_points[li], face, loop, max_dist, s_cdt->vert_pnts, s_cdt->w3dpnts, s_cdt->vert_norms, s_cdt->w3dnorms);
	    }

	    if (s->IsClosed(0) || s->IsClosed(1)) {
		PerformClosedSurfaceChecks(s_cdt, s, face, brep_loop_points, BREP_SAME_POINT_TOLERANCE);

	    }

	    ON_2dPointArray on_surf_points;
	    getSurfacePoints(s_cdt, face, on_surf_points);

	}
    }
    return -3;
}

#if 0
int brep_cdt_plot(struct rt_wdb *wdbp, struct bu_vls *vls, const char *solid_name,
                      const struct rt_tess_tol *ttol, const struct bn_tol *tol,
                      struct brep_specific* bs, struct rt_brep_internal*UNUSED(bi),
                      struct bn_vlblock *UNUSED(vbp), int UNUSED(plottype), int UNUSED(num_points))
{

void
ON_Brep_CDT_To_Mesh() {
    // Generate a BoT with normals.
    // TODO - apply the bot validity testing routines.
    
    std::vector<ON_3dPoint *> vfpnts;
    std::vector<ON_3dPoint *> vfnormals;
    std::map<ON_3dPoint *, int> on_pnt_to_bot_pnt;
    std::map<ON_3dPoint *, int> on_pnt_to_bot_norm;
    size_t triangle_cnt = 0;

    for (int face_index = 0; face_index != brep->m_F.Count(); face_index++) {
	ON_BrepFace &face = brep->m_F[face_index];
	p2t::CDT *cdt = p2t_faces[face_index];
	std::map<p2t::Point *, ON_3dPoint *> *pointmap = p2t_maps[face_index];
	std::map<p2t::Point *, ON_3dPoint *> *normalmap = p2t_nmaps[face_index];
	std::vector<p2t::Triangle*> tris = cdt->GetTriangles();
	triangle_cnt += tris.size();
	for (size_t i = 0; i < tris.size(); i++) {
	    p2t::Triangle *t = tris[i];
	    for (size_t j = 0; j < 3; j++) {
		p2t::Point *p = t->GetPoint(j);
		if (p) {
		    ON_3dPoint *op = (*pointmap)[p];
		    ON_3dPoint *onorm = (*normalmap)[p];
		    if (!op || !onorm) {
			/* We've got some calculating to do... */
			const ON_Surface *s = face.SurfaceOf();
			ON_3dPoint pnt;
			ON_3dVector norm;
			if (surface_EvNormal(s, p->x, p->y, pnt, norm)) {
			    if (face.m_bRev) {
				norm = norm * -1.0;
			    }
			    if (!op) {
				op = new ON_3dPoint(pnt);
				w3dpnts.push_back(op);
				vfpnts.push_back(op);
				(*pointmap)[p] = op;
				on_pnt_to_bot_pnt[op] = vfpnts.size() - 1;
			    }
			    if (!onorm) {
				onorm = new ON_3dPoint(norm);
				w3dnorms.push_back(onorm);
				vfnormals.push_back(onorm);
				(*normalmap)[p] = onorm;
				on_pnt_to_bot_norm[op] = vfnormals.size() - 1;
			    }
			} else {
			    bu_log("Erm... eval failed, no normal info?\n");
			    if (!op) {
				pnt = s->PointAt(p->x, p->y);
				op = new ON_3dPoint(pnt);
				w3dpnts.push_back(op);
				vfpnts.push_back(op);
				(*pointmap)[p] = op;
				on_pnt_to_bot_pnt[op] = vfpnts.size() - 1;
			    } else {
				if (on_pnt_to_bot_pnt.find(op) == on_pnt_to_bot_pnt.end()) {
				    vfpnts.push_back(op);
				    (*pointmap)[p] = op;
				    on_pnt_to_bot_pnt[op] = vfpnts.size() - 1;
				}
			    }
			}
		    } else {
			/* We've got them already, just add them */ 
			if (on_pnt_to_bot_pnt.find(op) == on_pnt_to_bot_pnt.end()) {
			    vfpnts.push_back(op);
			    vfnormals.push_back(onorm);
			    on_pnt_to_bot_pnt[op] = vfpnts.size() - 1;
			    on_pnt_to_bot_norm[op] = vfnormals.size() - 1;
			}
		    }
		} else {
		    bu_log("Face %d: p2t face without proper point info...\n", face.m_face_index);
		}
	    }
	}
    }

    // Know how many faces and points now - initialize BoT container.
    int *faces = (int *)bu_calloc(triangle_cnt*3, sizeof(int), "new faces array");
    fastf_t *vertices = (fastf_t *)bu_calloc(vfpnts.size()*3, sizeof(fastf_t), "new vert array");
    fastf_t *normals = (fastf_t *)bu_calloc(vfnormals.size()*3, sizeof(fastf_t), "new normals array");
    int *face_normals = (int *)bu_calloc(triangle_cnt*3, sizeof(int), "new face_normals array");

    for (size_t i = 0; i < vfpnts.size(); i++) {
	vertices[i*3] = vfpnts[i]->x;
	vertices[i*3+1] = vfpnts[i]->y;
	vertices[i*3+2] = vfpnts[i]->z;
    }

    for (size_t i = 0; i < vfnormals.size(); i++) {
	normals[i*3] = vfnormals[i]->x;
	normals[i*3+1] = vfnormals[i]->y;
	normals[i*3+2] = vfnormals[i]->z;
    }


    // Iterate over faces, adding points and faces to BoT container.  All
    // 3d points must end up unique in this final container.
    int face_cnt = 0;
    triangle_cnt = 0;
    for (int face_index = 0; face_index != brep->m_F.Count(); face_index++) {
	p2t::CDT *cdt = p2t_faces[face_index];
	std::map<p2t::Point *, ON_3dPoint *> *pointmap = p2t_maps[face_index];
	std::vector<p2t::Triangle*> tris = cdt->GetTriangles();
	triangle_cnt += tris.size();
	for (size_t i = 0; i < tris.size(); i++) {
	    p2t::Triangle *t = tris[i];
	    for (size_t j = 0; j < 3; j++) {
		p2t::Point *p = t->GetPoint(j);
		ON_3dPoint *op = (*pointmap)[p];
		int ind = on_pnt_to_bot_pnt[op];
		int nind = on_pnt_to_bot_norm[op];
		faces[face_cnt*3 + j] = ind;
		face_normals[face_cnt*3 + j] = nind;
	    }
	    if (brep->m_F[face_index].m_bRev) {
		int ftmp = faces[face_cnt*3 + 1];
		faces[face_cnt*3 + 1] = faces[face_cnt*3 + 2];
		faces[face_cnt*3 + 2] = ftmp;
		int fntmp = face_normals[face_cnt*3 + 1];
		face_normals[face_cnt*3 + 1] = face_normals[face_cnt*3 + 2];
		face_normals[face_cnt*3 + 2] = fntmp;
	    }

	    face_cnt++;
	}
    }

    struct rt_bot_internal *bot;
    BU_ALLOC(bot, struct rt_bot_internal);
    bot->magic = RT_BOT_INTERNAL_MAGIC;
    bot->mode = RT_BOT_SOLID;
    bot->orientation = RT_BOT_CCW;
    bot->bot_flags = 0;
    bot->num_vertices = vfpnts.size();
    bot->num_faces = triangle_cnt;
    bot->vertices = vertices;
    bot->faces = faces;
    bot->thickness = NULL;
    bot->face_mode = (struct bu_bitv *)NULL;
    bot->num_normals = vfpnts.size();
    bot->num_face_normals = bot->num_faces;
    bot->normals = normals;
    bot->face_normals = face_normals;

    return wdb_export(wdbp, "bot.s", (void *)bot, ID_BOT, 1.0);
}
#endif


/** @} */

// Local Variables:
// mode: C++
// tab-width: 8
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

