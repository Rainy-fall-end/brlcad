/* Commands */

MGED_EXTERN(int f_3ptarb, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_adc, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_aetview, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_aim, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_aip, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_amtrack, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_analyze, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_arbdef, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_arced, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_area, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_attach, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_bev, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_blast, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_cat, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_center, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_color, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_comb, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_comb_color, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_comb_std, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_comm, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_concat, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_copy, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_copy_inv, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_copyeval, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_debug, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_debugbu, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_debugdir, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_debuglib, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_debugmem, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_debugnmg, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_decompose, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_delay, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_delobj, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_dm, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_dup, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_eac, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_edcodes, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_edmater, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_edcolor, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_edcomb, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_edgedir, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_edit, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_eqn, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_ev, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_evedit, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_extrude, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_facedef, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_facetize, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_fhelp, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_find, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_fix, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_fracture, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_group, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_help, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_hideline, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_history, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_ill, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_in, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_inside, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_instance, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_itemair, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_joint, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_journal, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_keep, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_keypoint, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_kill, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_killall, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_killtree, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_knob, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_labelvert, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
/* MGED_EXTERN(int f_list, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv)); */
MGED_EXTERN(int f_load_dv, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_make, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_make_bb, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_mater, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_matpick, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_memprint, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_mirface, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_mirror, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_mouse, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_mvall, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_name, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_nirt, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_nmg_simplify, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_opendb, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_orientation, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_overlay, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_param, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_pathsum, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_permute, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_pl, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_plot, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_polybinout, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_pov, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_prcolor, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_prefix, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_press, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_preview, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_ps, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_push, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_putmat, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_quit, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_qorot, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_qvrot, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_rcodes, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_red, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_refresh, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_regdebug, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_regdef, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_region, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_release, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_rfarb, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_rm, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_rmater, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_rmats, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_rot_obj, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_rrt, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_rt, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_rtcheck, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_savekey, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_saveview, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_sc_obj, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_sed, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_set, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_setview, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_shader, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_shells, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_showmats, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_source, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_status, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_summary, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_slewview, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_svbase, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_sync, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_tables, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_tabobj, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_tedit, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_tie, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_title, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_tol, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_tops, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_tran, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_tr_obj, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_tree, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_units, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_untie, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_update, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_view, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_vrmgr, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_vrot, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_vrot_center, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_wcodes, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_whatid, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_which_air, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_which_id, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_winset, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_wmater, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_xpush, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_zap, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int f_zoom, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));

MGED_EXTERN(int cmd_close, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int cmd_get, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int cmd_init, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int cmd_mged_glob, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int cmd_set, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int cmd_stuff_str, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int cmd_vdraw, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int cmd_viewget, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int cmd_viewset, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
MGED_EXTERN(int cmd_who, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));
