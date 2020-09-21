/*                    E X E C _ M A P P I N G . C P P
 * BRL-CAD
 *
 * Copyright (c) 2020 United States Government as represented by
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
/** @file exec_wrapping.cpp
 *
 * Provide compile time wrappers that pass specific libged function calls
 * through to the plugin system.
 *
 * Some validation is also performed to ensure the argv[0] string value makes
 * sense - unlike raw function calls, correct argv[0] values are important with
 * ged_exec to ensure the expected functionality is invoked.
 */

#include "common.h"

#include "ged.h"

#define GED_CMD_HELPER1(x, y) x##y
#define GED_CMD(x) \
	int GED_CMD_HELPER1(ged_,x)(struct ged *gedp, int argc, const char *argv[]) \
	{ \
	    const char *fname = #x ; \
	    int vret = ged_cmd_valid(argv[0], fname); \
	    if (vret) { \
		argv[0] = fname; \
	    }\
	    int ret = ged_exec(gedp, argc, argv); \
	    if (vret) { \
		ret |= GED_UNKNOWN; \
	    } \
	    return ret; \
	} \

GED_CMD(3ptarb)
GED_CMD(E)
GED_CMD(adc)
GED_CMD(adjust)
GED_CMD(ae2dir)
GED_CMD(aet)
GED_CMD(analyze)
GED_CMD(annotate)
GED_CMD(arb)
GED_CMD(arced)
GED_CMD(arot)
GED_CMD(attr)
GED_CMD(autoview)
GED_CMD(bb)
GED_CMD(bev)
GED_CMD(blast)
GED_CMD(bo)
GED_CMD(bot)
GED_CMD(bot_condense)
GED_CMD(bot_decimate)
GED_CMD(bot_dump)
GED_CMD(bot_edge_split)
GED_CMD(bot_face_fuse)
GED_CMD(bot_face_sort)
GED_CMD(bot_face_split)
GED_CMD(bot_flip)
GED_CMD(bot_fuse)
GED_CMD(bot_merge)
GED_CMD(bot_move_pnt)
GED_CMD(bot_move_pnts)
GED_CMD(bot_smooth)
GED_CMD(bot_split)
GED_CMD(bot_sync)
GED_CMD(bot_vertex_fuse)
GED_CMD(brep)
GED_CMD(c)
GED_CMD(cat)
GED_CMD(cc)
GED_CMD(center)
GED_CMD(check)
GED_CMD(clone)
GED_CMD(coil)
GED_CMD(color)
GED_CMD(comb)
GED_CMD(comb_color)
GED_CMD(comb_std)
GED_CMD(combmem)
GED_CMD(concat)
GED_CMD(constraint)
GED_CMD(copy)
GED_CMD(copyeval)
GED_CMD(copymat)
GED_CMD(cpi)
GED_CMD(d)
GED_CMD(dbconcat)
GED_CMD(dbfind)
GED_CMD(dbip)
GED_CMD(dbot_dump)
GED_CMD(dbversion)
GED_CMD(debug)
GED_CMD(debugbu)
GED_CMD(debugdir)
GED_CMD(debuglib)
GED_CMD(debugnmg)
GED_CMD(decompose)
GED_CMD(delay)
GED_CMD(dir2ae)
GED_CMD(draw)
GED_CMD(dsp)
GED_CMD(dump)
GED_CMD(dup)
GED_CMD(eac)
GED_CMD(echo)
GED_CMD(edarb)
GED_CMD(edcodes)
GED_CMD(edcolor)
GED_CMD(edcomb)
GED_CMD(edit)
GED_CMD(editit)
GED_CMD(edmater)
GED_CMD(env)
GED_CMD(erase)
GED_CMD(ev)
GED_CMD(exists)
GED_CMD(expand)
GED_CMD(eye)
GED_CMD(eye_pos)
GED_CMD(eye_pt)
GED_CMD(facetize)
GED_CMD(fb2pix)
GED_CMD(fbclear)
GED_CMD(find)
GED_CMD(find_arb_edge)
GED_CMD(find_arb_edge_nearest_pnt)
GED_CMD(find_bot_edge)
GED_CMD(find_bot_edge_nearest_pnt)
GED_CMD(find_bot_pnt)
GED_CMD(find_bot_pnt_nearest_pnt)
GED_CMD(find_metaball_pnt_nearest_pnt)
GED_CMD(find_pipe_pnt)
GED_CMD(find_pipe_pnt_nearest_pnt)
GED_CMD(form)
GED_CMD(fracture)
GED_CMD(gdiff)
GED_CMD(get)
GED_CMD(get_autoview)
GED_CMD(get_bot_edges)
GED_CMD(get_comb)
GED_CMD(get_eyemodel)
GED_CMD(get_type)
GED_CMD(glob)
GED_CMD(gqa)
GED_CMD(graph)
GED_CMD(grid)
GED_CMD(grid2model_lu)
GED_CMD(grid2view_lu)
GED_CMD(group)
GED_CMD(heal)
GED_CMD(help)
GED_CMD(hide)
GED_CMD(how)
GED_CMD(human)
GED_CMD(i)
GED_CMD(idents)
GED_CMD(illum)
GED_CMD(importFg4Section)
GED_CMD(in)
GED_CMD(inside)
GED_CMD(instance)
GED_CMD(isize)
GED_CMD(item)
GED_CMD(joint)
GED_CMD(joint2)
GED_CMD(keep)
GED_CMD(keypoint)
GED_CMD(kill)
GED_CMD(killall)
GED_CMD(killrefs)
GED_CMD(killtree)
GED_CMD(lc)
GED_CMD(lint)
GED_CMD(list)
GED_CMD(listeval)
GED_CMD(loadview)
GED_CMD(lod)
GED_CMD(log)
GED_CMD(lookat)
GED_CMD(ls)
GED_CMD(lt)
GED_CMD(m2v_point)
GED_CMD(make)
GED_CMD(make_name)
GED_CMD(make_pnts)
GED_CMD(mat4x3pnt)
GED_CMD(mat_ae)
GED_CMD(mat_mul)
GED_CMD(mat_scale_about_pnt)
GED_CMD(match)
GED_CMD(mater)
GED_CMD(metaball_add_pnt)
GED_CMD(metaball_delete_pnt)
GED_CMD(metaball_move_pnt)
GED_CMD(mirror)
GED_CMD(model2grid_lu)
GED_CMD(model2view)
GED_CMD(model2view_lu)
GED_CMD(mouse_add_metaball_pnt)
GED_CMD(mouse_append_pipe_pnt)
GED_CMD(mouse_move_metaball_pnt)
GED_CMD(mouse_move_pipe_pnt)
GED_CMD(mouse_prepend_pipe_pnt)
GED_CMD(move)
GED_CMD(move_all)
GED_CMD(move_arb_edge)
GED_CMD(move_arb_face)
GED_CMD(mrot)
GED_CMD(mvall)
GED_CMD(nirt)
GED_CMD(nmg)
GED_CMD(nmg_collapse)
GED_CMD(nmg_fix_normals)
GED_CMD(nmg_simplify)
GED_CMD(ocenter)
GED_CMD(orient)
GED_CMD(orientation)
GED_CMD(orotate)
GED_CMD(oscale)
GED_CMD(otranslate)
GED_CMD(overlay)
GED_CMD(pathlist)
GED_CMD(paths)
GED_CMD(pathsum)
GED_CMD(perspective)
GED_CMD(pipe_append_pnt)
GED_CMD(pipe_delete_pnt)
GED_CMD(pipe_move_pnt)
GED_CMD(pipe_prepend_pnt)
GED_CMD(pix2fb)
GED_CMD(plot)
GED_CMD(pmat)
GED_CMD(pmodel2view)
GED_CMD(png)
GED_CMD(png2fb)
GED_CMD(pngwf)
GED_CMD(pnts)
GED_CMD(postscript)
GED_CMD(prcolor)
GED_CMD(prefix)
GED_CMD(preview)
GED_CMD(process)
GED_CMD(protate)
GED_CMD(ps)
GED_CMD(pscale)
GED_CMD(pset)
GED_CMD(ptranslate)
GED_CMD(pull)
GED_CMD(push)
GED_CMD(put)
GED_CMD(put_comb)
GED_CMD(putmat)
GED_CMD(qray)
GED_CMD(quat)
GED_CMD(qvrot)
GED_CMD(rcodes)
GED_CMD(rect)
GED_CMD(red)
GED_CMD(redraw)
GED_CMD(regdef)
GED_CMD(region)
GED_CMD(regions)
GED_CMD(remove)
GED_CMD(reopen)
GED_CMD(rfarb)
GED_CMD(rm)
GED_CMD(rmap)
GED_CMD(rmat)
GED_CMD(rmater)
GED_CMD(rot)
GED_CMD(rot_about)
GED_CMD(rot_point)
GED_CMD(rotate_about)
GED_CMD(rotate_arb_face)
GED_CMD(rrt)
GED_CMD(rselect)
GED_CMD(rt)
GED_CMD(rtabort)
GED_CMD(rtarea)
GED_CMD(rtcheck)
GED_CMD(rtedge)
GED_CMD(rtweight)
GED_CMD(rtwizard)
GED_CMD(savekey)
GED_CMD(saveview)
GED_CMD(sca)
GED_CMD(scale)
GED_CMD(screen_grab)
GED_CMD(search)
GED_CMD(select)
GED_CMD(set_output_script)
GED_CMD(set_transparency)
GED_CMD(set_uplotOutputMode)
GED_CMD(setview)
GED_CMD(shaded_mode)
GED_CMD(shader)
GED_CMD(shells)
GED_CMD(showmats)
GED_CMD(simulate)
GED_CMD(size)
GED_CMD(slew)
GED_CMD(solid_report)
GED_CMD(solids)
GED_CMD(solids_on_ray)
GED_CMD(sphgroup)
//GED_CMD(stat)
GED_CMD(summary)
GED_CMD(sv)
GED_CMD(sync)
GED_CMD(t)
GED_CMD(tables)
GED_CMD(tire)
GED_CMD(title)
GED_CMD(tol)
GED_CMD(tops)
GED_CMD(tra)
GED_CMD(track)
GED_CMD(tracker)
GED_CMD(tree)
GED_CMD(unhide)
GED_CMD(units)
GED_CMD(v2m_point)
GED_CMD(vdraw)
GED_CMD(version)
GED_CMD(view2grid_lu)
GED_CMD(view2model)
GED_CMD(view2model_lu)
GED_CMD(view2model_vec)
GED_CMD(view_func)
GED_CMD(viewdir)
GED_CMD(vnirt)
GED_CMD(voxelize)
GED_CMD(vrot)
GED_CMD(wcodes)
GED_CMD(whatid)
GED_CMD(which)
GED_CMD(which_shader)
GED_CMD(whichair)
GED_CMD(whichid)
GED_CMD(who)
GED_CMD(wmater)
GED_CMD(x)
GED_CMD(xpush)
GED_CMD(ypr)
GED_CMD(zap)
GED_CMD(zoom)


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

