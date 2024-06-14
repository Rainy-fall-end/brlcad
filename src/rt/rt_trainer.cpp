#include "common.h"

#include"rt/rt_trainer.h"
#include"rt/application.h"
#include "raytrace.h"

#include "vmath.h"
#include "icv.h"
#include "bu/cv.h"
#include "dm.h"
#include "bv/plot3.h"
#include "photonmap.h"
#include "scanline.h"

#include "./rtuif.h"
#include "./ext.h"

extern "C"
{
	extern int curframe;		/* from main.c */
	extern double airdensity;	/* from opt.c */
	extern double haze[3];		/* from opt.c */
	extern int do_kut_plane;        /* from opt.c */
	extern plane_t kut_plane;       /* from opt.c */
}
struct soltab* kut_soltab = NULL;
int ambSlow = 0;
int ambSamples = 0;
double ambRadius = 0.0;
double ambOffset = 0.0;
vect_t ambient_color = { 1, 1, 1 };	/* Ambient white light */
int ibackground[3] = { 0 };		/* integer 0..255 version */
int inonbackground[3] = { 0 };		/* integer non-background */
fastf_t gamma_corr = 0.0;		/* gamma correction if !0 */
TrainData::TrainData(struct rt_i* rtip) : m_rt_i(rtip) {}

std::vector<RGBpixel> TrainData::ShootSamples(const RayParam& ray_list) {
    struct application ap;

    RT_APPLICATION_INIT(&ap);
    ap.a_rt_i = this->m_rt_i;
    // ap.a_hit = hit;
    // ap.a_miss = miss;
    ap.a_onehit = 1;
    ap.a_logoverlap = rt_silent_logoverlap;
	std::vector<RGBpixel> res;
	return res;
}

void TrainData::ClearRes()
{
    m_res.clear();
}

TrainData::~TrainData() {}

static int
hit_nothing(struct application* ap)
{
	if (OPTICAL_DEBUG & OPTICAL_DEBUG_MISSPLOT) {
		vect_t out;

		/* XXX length should be 1 model diameter */
		VJOIN1(out, ap->a_ray.r_pt,
			10000, ap->a_ray.r_dir);	/* to imply direction */
		bu_semaphore_acquire(BU_SEM_SYSCALL);
		pl_color(stdout, 190, 0, 0);
		pdv_3line(stdout, ap->a_ray.r_pt, out);
		bu_semaphore_release(BU_SEM_SYSCALL);
	}

	if (env_region.reg_mfuncs) {
		struct gunk {
			struct partition part;
			struct hit hit;
			struct shadework sw;
		} u;

		memset((char*)&u, 0, sizeof(u));
		/* Make "miss" hit the environment map */
		/* Build up the fakery */
		u.part.pt_magic = PT_MAGIC;
		u.part.pt_inhit = u.part.pt_outhit = &u.hit;
		u.part.pt_regionp = &env_region;
		u.hit.hit_magic = RT_HIT_MAGIC;
		u.hit.hit_dist = ap->a_rt_i->rti_radius * 2;	/* model diam */
		u.hit.hit_rayp = &ap->a_ray;

		u.sw.sw_transmit = u.sw.sw_reflect = 0.0;
		u.sw.sw_refrac_index = 1.0;
		u.sw.sw_extinction = 0;
		u.sw.sw_xmitonly = 1;		/* don't shade env map! */

		/* "Surface" Normal points inward, UV is azim/elev of ray */
		u.sw.sw_inputs = MFI_NORMAL | MFI_UV;
		VREVERSE(u.sw.sw_hit.hit_normal, ap->a_ray.r_dir);
		/* U is azimuth, atan() range: -pi to +pi */
		u.sw.sw_uv.uv_u = bn_atan2(ap->a_ray.r_dir[Y],
			ap->a_ray.r_dir[X]) * M_1_2PI;
		if (u.sw.sw_uv.uv_u < 0)
			u.sw.sw_uv.uv_u += 1.0;
		/*
		 * V is elevation, atan() range: -pi/2 to +pi/2, because
		 * sqrt() ensures that X parameter is always >0
		 */
		u.sw.sw_uv.uv_v = bn_atan2(ap->a_ray.r_dir[Z],
			sqrt(ap->a_ray.r_dir[X] * ap->a_ray.r_dir[X] +
				ap->a_ray.r_dir[Y] * ap->a_ray.r_dir[Y])) *
			M_1_PI + 0.5;
		u.sw.sw_uv.uv_du = u.sw.sw_uv.uv_dv = 0;

		VSETALL(u.sw.sw_color, 1);
		VSETALL(u.sw.sw_basecolor, 1);

		if (OPTICAL_DEBUG & OPTICAL_DEBUG_SHADE)
			bu_log("hit_nothing calling viewshade\n");

		(void)viewshade(ap, &u.part, &u.sw);

		VMOVE(ap->a_color, u.sw.sw_color);
		ap->a_user = 1;		/* Signal view_pixel:  HIT */
		ap->a_uptr = (void*)&env_region;
		return 1;
	}

	ap->a_user = 0;		/* Signal view_pixel:  MISS */
	VMOVE(ap->a_color, background);	/* In case someone looks */
	return 0;
}


int
colorview(struct application* ap, struct partition* PartHeadp, struct seg* finished_segs)
{
	struct partition* pp;
	struct hit* hitp;
	struct shadework sw;

	pp = PartHeadp->pt_forw;
	if (ap->a_flag == 1) {
		/* This ray is an escaping internal ray after refraction
		 * through glass.  Sometimes, after refraction and starting a
		 * new ray at the glass exit, the new ray hits a sliver of the
		 * same glass, and gets confused. This bit of code attempts to
		 * spot this behavior and skip over the glass sliver.  Any
		 * sliver less than 0.05mm thick will be skipped (0.05 is a
		 * SWAG).
		 */
		if ((void*)pp->pt_regionp == ap->a_uptr &&
			pp->pt_forw != PartHeadp &&
			pp->pt_outhit->hit_dist - pp->pt_inhit->hit_dist < 0.05)
			pp = pp->pt_forw;
	}

	for (; pp != PartHeadp; pp = pp->pt_forw)
		if (pp->pt_outhit->hit_dist >= 0.0) break;

	if (pp == PartHeadp) {
		bu_log("colorview:  no hit out front?\n");
		return 0;
	}

	if (do_kut_plane) {
		pp = do_kut(ap, pp, PartHeadp);

		if (!pp || pp == PartHeadp) {
			/* we ignored everything, this is now a miss */
			ap->a_miss(ap);
			return 0;
		}
	}


	RT_CK_PT(pp);
	hitp = pp->pt_inhit;
	RT_CK_HIT(hitp);
	RT_CK_RAY(hitp->hit_rayp);
	ap->a_uptr = (void*)pp->pt_regionp;	/* note which region was shaded */

	if (OPTICAL_DEBUG & OPTICAL_DEBUG_HITS) {
		bu_log("colorview: lvl=%d coloring %s\n",
			ap->a_level,
			pp->pt_regionp->reg_name);
		rt_pr_partition(ap->a_rt_i, pp);
	}
	if (hitp->hit_dist >= INFINITY) {
		bu_log("colorview:  entry beyond infinity\n");
		VSET(ap->a_color, .5, 0, 0);
		ap->a_user = 1;		/* Signal view_pixel:  HIT */
		ap->a_dist = hitp->hit_dist;
		goto out;
	}

	/* Check to see if eye is "inside" the solid It might only be
	 * worthwhile doing all this in perspective mode XXX Note that
	 * hit_dist can be faintly negative, e.g. -1e-13
	 *
	 * XXX we should certainly only do this if the eye starts out
	 * inside an opaque solid.  If it starts out inside glass or air
	 * we don't really want to do this
	 */

	if (hitp->hit_dist < 0.0 && pp->pt_regionp->reg_aircode == 0) {
		struct application sub_ap;
		fastf_t f;

		if (pp->pt_outhit->hit_dist >= INFINITY ||
			ap->a_level > max_bounces) {
			if (OPTICAL_DEBUG & OPTICAL_DEBUG_SHOWERR) {
				VSET(ap->a_color, 9, 0, 0);	/* RED */
				bu_log("colorview:  eye inside %s (x=%d, y=%d, lvl=%d)\n",
					pp->pt_regionp->reg_name,
					ap->a_x, ap->a_y, ap->a_level);
			}
			else {
				VSETALL(ap->a_color, 0.18);	/* 18% Grey */
			}
			ap->a_user = 1;		/* Signal view_pixel:  HIT */
			ap->a_dist = hitp->hit_dist;
			goto out;
		}
		/* Push on to exit point, and trace on from there */
		sub_ap = *ap;	/* struct copy */
		sub_ap.a_level = ap->a_level + 1;
		f = pp->pt_outhit->hit_dist + hitp->hit_dist + 0.0001;
		VJOIN1(sub_ap.a_ray.r_pt, ap->a_ray.r_pt, f, ap->a_ray.r_dir);
		sub_ap.a_purpose = "pushed eye position";
		(void)rt_shootray(&sub_ap);

		/* The eye is inside a solid and we are "Looking out" so we
		 * are going to darken what we see beyond to give a visual cue
		 * that something is wrong.
		 */
		VSCALE(ap->a_color, sub_ap.a_color, 0.80);

		ap->a_user = 1;		/* Signal view_pixel: HIT */
		ap->a_dist = f + sub_ap.a_dist;
		ap->a_uptr = sub_ap.a_uptr;	/* which region */
		goto out;
	}

	/* Record the approach path */
	if (OPTICAL_DEBUG & OPTICAL_DEBUG_RAYWRITE && (hitp->hit_dist > 0.0001)) {
		VJOIN1(hitp->hit_point, ap->a_ray.r_pt,
			hitp->hit_dist, ap->a_ray.r_dir);
		wraypts(ap->a_ray.r_pt,
			ap->a_ray.r_dir,
			hitp->hit_point,
			-1, ap, stdout);	/* -1 = air */
	}

	if ((OPTICAL_DEBUG & (OPTICAL_DEBUG_RAYPLOT | OPTICAL_DEBUG_RAYWRITE | OPTICAL_DEBUG_REFRACT)) && (hitp->hit_dist > 0.0001)) {
		/* There are two parts to plot here.  Ray start to inhit
		 * (purple), and inhit to outhit (grey).
		 */
		int i, lvl;
		fastf_t out;
		vect_t inhit, outhit;

		lvl = ap->a_level % 100;
		if (lvl < 0) lvl = 0;
		else if (lvl > 3) lvl = 3;
		i = 255 - lvl * (128 / 4);

		VJOIN1(inhit, ap->a_ray.r_pt,
			hitp->hit_dist, ap->a_ray.r_dir);
		if (OPTICAL_DEBUG & OPTICAL_DEBUG_RAYPLOT) {
			bu_semaphore_acquire(BU_SEM_SYSCALL);
			pl_color(stdout, i, 0, i);
			pdv_3line(stdout, ap->a_ray.r_pt, inhit);
			bu_semaphore_release(BU_SEM_SYSCALL);
		}
		bu_log("From ray start to inhit (purple):\n \
vdraw open oray;vdraw params c %2.2x%2.2x%2.2x;vdraw write n 0 %g %g %g;vdraw write n 1 %g %g %g;vdraw send\n",
i, 0, i,
V3ARGS(ap->a_ray.r_pt),
V3ARGS(inhit));

		if ((out = pp->pt_outhit->hit_dist) >= INFINITY)
			out = 10000;	/* to imply the direction */
		VJOIN1(outhit,
			ap->a_ray.r_pt, out,
			ap->a_ray.r_dir);
		if (OPTICAL_DEBUG & OPTICAL_DEBUG_RAYPLOT) {
			bu_semaphore_acquire(BU_SEM_SYSCALL);
			pl_color(stdout, i, i, i);
			pdv_3line(stdout, inhit, outhit);
			bu_semaphore_release(BU_SEM_SYSCALL);
		}
		bu_log("From inhit to outhit (grey):\n \
vdraw open iray;vdraw params c %2.2x%2.2x%2.2x;vdraw write n 0 %g %g %g;vdraw write n 1 %g %g %g;vdraw send\n",
i, i, i,
V3ARGS(inhit), V3ARGS(outhit));

	}

	memset((char*)&sw, 0, sizeof(sw));
	sw.sw_transmit = sw.sw_reflect = 0.0;
	sw.sw_refrac_index = 1.0;
	sw.sw_extinction = 0;
	sw.sw_xmitonly = 0;		/* want full data */
	sw.sw_inputs = 0;		/* no fields filled yet */
	sw.sw_frame = curframe;
	sw.sw_segs = finished_segs;
	VSETALL(sw.sw_color, 1);
	VSETALL(sw.sw_basecolor, 1);

	if (OPTICAL_DEBUG & OPTICAL_DEBUG_SHADE)
		bu_log("colorview calling viewshade\n");

	/* individual shaders must handle reflection & refraction */
	(void)viewshade(ap, pp, &sw);

	VMOVE(ap->a_color, sw.sw_color);
	ap->a_user = 1;		/* Signal view_pixel:  HIT */
	/* XXX This is always negative when eye is inside air solid */
	ap->a_dist = hitp->hit_dist;

out:
	/*
	 * e ^(-density * distance)
	 */
	if (!ZERO(airdensity)) {
		double g;
		double f = exp(-hitp->hit_dist * airdensity);
		g = (1.0 - f);

		VSCALE(ap->a_color, ap->a_color, f);
		VJOIN1(ap->a_color, ap->a_color, g, haze);
	}


	if (ambSamples > 0)
		ambientOcclusion(ap, pp);

	RT_CK_REGION(ap->a_uptr);
	if (OPTICAL_DEBUG & OPTICAL_DEBUG_HITS) {
		bu_log("colorview: lvl=%d ret a_user=%d %s\n",
			ap->a_level,
			ap->a_user,
			pp->pt_regionp->reg_name);
		VPRINT("color   ", ap->a_color);
	}
	return 1;
}

static struct partition*
do_kut(const struct application* ap, struct partition* pp, struct partition* PartHeadp)
{
	fastf_t slant_factor;
	fastf_t dist;
	fastf_t norm_dist;

	if (UNLIKELY(!ap || !PartHeadp))
		return NULL;

	norm_dist = DIST_PNT_PLANE(ap->a_ray.r_pt, kut_plane);

	slant_factor = -VDOT(kut_plane, ap->a_ray.r_dir);
	if (slant_factor < -1.0e-10) {
		/* exit point, ignore everything before "dist" */
		dist = norm_dist / slant_factor;
		for (; pp != PartHeadp; pp = pp->pt_forw) {
			if (pp->pt_outhit->hit_dist >= dist) {
				if (pp->pt_inhit->hit_dist < dist) {
					pp->pt_inhit->hit_dist = dist;
					pp->pt_inflip = 0;
					pp->pt_inseg->seg_stp = kut_soltab;
				}
				break;
			}
		}
		if (pp == PartHeadp) {
			/* we ignored everything, this is now a miss */
			return pp;
		}
	}
	else if (slant_factor > 1.0e-10) {
		/* entry point, ignore everything after "dist" */
		dist = norm_dist / slant_factor;
		if (pp->pt_inhit->hit_dist > dist) {
			/* everything is after kut plane, this is now a miss */
			return NULL;
		}
	}
	else {
		/* ray is parallel to plane when dir.N == 0.
		 * If it is inside the solid, this is a miss */
		if (norm_dist < 0.0) {
			return NULL;
		}
	}

	return pp;
}

void
ambientOcclusion(struct application* ap, struct partition* pp)
{
	struct application amb_ap = *ap;
	struct soltab* stp;
	struct hit* hitp;
	vect_t inormal;
	vect_t vAxis;
	vect_t uAxis;
	int ao_samp;
	vect_t origin = VINIT_ZERO;
	double occlusionFactor;
	int hitCount = 0;

	stp = pp->pt_inseg->seg_stp;

	hitp = pp->pt_inhit;
	VJOIN1(amb_ap.a_ray.r_pt, ap->a_ray.r_pt, hitp->hit_dist, ap->a_ray.r_dir);
	amb_ap.a_hit = ao_rayhit;
	amb_ap.a_miss = ao_raymiss;
	amb_ap.a_onehit = 4;  /* make sure we get at least two complete partitions.  The first may be "behind" the ray start */

	RT_HIT_NORMAL(inormal, hitp, stp, &(ap->a_ray), pp->pt_inflip);

	/* construct the ray origin.  Move it normalward off the surface
	 * to reduce the chances that the AO rays will hit the surface the ray
	 * is departing from.
	 */
	if (ZERO(ambOffset)) {
		VJOIN1(amb_ap.a_ray.r_pt, amb_ap.a_ray.r_pt, ap->a_rt_i->rti_tol.dist, inormal);
	}
	else {
		VJOIN1(amb_ap.a_ray.r_pt, amb_ap.a_ray.r_pt, ambOffset, inormal);
	}

	/* form a coordinate system at the hit point */
	VCROSS(vAxis, inormal, ap->a_ray.r_dir);
	if (MAGSQ(vAxis) < ap->a_rt_i->rti_tol.dist_sq) {
		/* It appears the ray and the normal are perfectly aligned.
		 * Time to construct a random vector to use for the cross-product
		 * to construct the coordinate system
		 */
		vect_t arbitraryDir;

		VSET(arbitraryDir, inormal[Y], inormal[Z], inormal[X]);
		VCROSS(vAxis, inormal, arbitraryDir);
	}

	VUNITIZE(vAxis);
	VCROSS(uAxis, vAxis, inormal);

	for (ao_samp = 0; ao_samp < ambSamples; ao_samp++) {
		vect_t randScale;

		/* pick a random direction in the unit sphere */
		do {
			/* less noisy but much slower */
			randScale[X] = (bn_randmt() - 0.5) * 2.0;
			randScale[Y] = (bn_randmt() - 0.5) * 2.0;
			randScale[Z] = bn_randmt();
		} while (MAGSQ(randScale) > 1.0);

		VJOIN3(amb_ap.a_ray.r_dir, origin,
			randScale[X], uAxis,
			randScale[Y], vAxis,
			randScale[Z], inormal);

		VUNITIZE(amb_ap.a_ray.r_dir);

		amb_ap.a_user = 0;
		amb_ap.a_flag = 0;

		/* shoot in the direction and see what we hit */
		rt_shootray(&amb_ap);
		hitCount += amb_ap.a_flag;
	}

	occlusionFactor = 1.0 - (hitCount / (float)ambSamples);

	/* try not go go completely black */
	CLAMP(occlusionFactor, 0.0125, 1.0);

	VSCALE(ap->a_color, ap->a_color, occlusionFactor);
}

int
ao_rayhit(register struct application* ap,
	struct partition* PartHeadp,
	struct seg* UNUSED(segp))
{
	struct partition* pp = PartHeadp->pt_forw;
	fastf_t dist = ap->a_rt_i->rti_tol.dist;

	if (do_kut_plane) {
		pp = do_kut(ap, pp, PartHeadp);
		if (!pp || pp == PartHeadp) {
			/* we ignored everything, this is now a miss */
			ap->a_miss(ap);
			return 0;
		}
	}

	/* if we don't have a radius, then any hit is ambient occlusion */
	if (NEAR_ZERO(ambRadius, dist)) {
		ap->a_user = 1;
		ap->a_flag = 1;
		return 1;
	}


	/* find the first hit that is in front */
	for (pp = PartHeadp->pt_forw; pp != PartHeadp; pp = pp->pt_forw) {

		dist = pp->pt_inhit->hit_dist;
		if (dist > 0.0) {
			if (dist < ambRadius) {
				/* first hit is inside radius, so this is occlusion */
				ap->a_user = 1;
				ap->a_flag = 1;
				return 1;
			}
			else {
				/* first hit outside radius is same as no occlusion */
				ap->a_user = 0;
				ap->a_flag = 0;
				return 0;
			}
		}
	}

	/* no hits in front of the ray, so we miss */
	ap->a_user = 0;
	ap->a_flag = 0;
	return 0;
}

int
ao_raymiss(register struct application* ap)
{
	ap->a_user = 0;
	ap->a_flag = 0;
	return 0;
}