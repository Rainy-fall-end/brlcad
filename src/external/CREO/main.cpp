/*                    M A I N . C P P
 * BRL-CAD
 *
 * Copyright (c) 2017 United States Government as represented by
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
/** @file main.cpp
 *
 */

#include "common.h"
#include "creo-brl.h"

extern "C" void
creo_conv_info_init(struct creo_conv_info *cinfo)
{
    int i;

    /* Region ID */
    cinfo->reg_id = 1000;

    /* File settings */
    cinfo->logger = (FILE *)NULL;
    cinfo->logger_type=LOGGER_TYPE_NONE;
    cinfo->curr_msg_type = MSG_DEBUG;
    cinfo->print_to_console=1;

    /* units - model */
    cinfo->creo_to_brl_conv = 25.4; /* inches to mm */
    cinfo->local_tol=0.0;
    cinfo->local_tol_sq=0.0;

    /* facetization settings */
    cinfo->max_error=1.5;
    cinfo->min_error=1.5;
    cinfo->tol_dist=0.0005;
    cinfo->max_angle_cntrl=0.5;
    cinfo->min_angle_cntrl=0.5;
    cinfo->max_to_min_steps = 1;
    cinfo->error_increment=0.0;
    cinfo->angle_increment=0.0;

    /* csg settings */
    cinfo->min_hole_diameter=0.0;
    cinfo->min_chamfer_dim=0.0;
    cinfo->min_round_radius=0.0;

    cinfo->parts = new std::set<wchar_t *, WStrCmp>;
    cinfo->assems = new std::set<wchar_t *, WStrCmp>;
    cinfo->assem_child_cnts = new std::map<wchar_t *, int>;
    cinfo->empty = new std::set<wchar_t *, WStrCmp>;
    cinfo->name_map = new std::map<wchar_t *, struct bu_vls *>;
    cinfo->brlcad_names = new std::set<struct bu_vls *, StrCmp>;

}

extern "C" void
creo_conv_info_free(struct creo_conv_info *cinfo)
{

    std::set<wchar_t *, WStrCmp>::iterator d_it;
    for (d_it = cinfo->parts->begin(); d_it != cinfo->parts->end(); d_it++) {
	bu_free(*d_it, "free wchar str copy");
    }
    for (d_it = cinfo->assems->begin(); d_it != cinfo->assems->end(); d_it++) {
	bu_free(*d_it, "free wchar str copy");
    }

    std::set<struct bu_vls *, StrCmp>::iterator s_it;
    for (s_it = brlcad_names.begin(); s_it != brlcad_names.end(); s_it++) {
	struct bu_vls *v = *s_it;
	bu_vls_free(v);
	BU_PUT(v, struct bu_vls);
    }

    delete cinfo->parts;
    delete cinfo->assems;
    delete cinfo->assem_child_cnts;
    delete cinfo->empty; /* entries in empty were freed in parts and assems */
    delete cinfo->brlcad_names;
    delete cinfo->name_map; /* entries in name_map were freed in brlcad_names */

    if (cinfo->logger) fclose(cinfo->logger);
    wdb_close(cinfo->wdbp);

    /* Finally, clear the container */
    BU_PUT(cinfo, struct creo_conv_info);
}

extern "c" void
output_parts(struct creo_conv_info *cinfo)
{
    std::set<wchar_t *, WStrCmp>::iterator d_it;
    for (d_it = cinfo->parts->begin(); d_it != cinfo->parts->end(); d_it++) {
	ProMdl m = ProMdlnameInit(*d_it, PRO_PART);
	int solid_part = output_part(cinfo, m);
	if (!solid_part) cinfo->empty->insert(*d_it);
    }
}

extern "C" static ProError
assembly_check_empty( ProFeature *feat, ProError status, ProAppData app_data )
{
    ProError status;
    ProMdlType type;
    char wname[10000];
    int *has_shape = (int *)app_data;
    if (status = ProAsmcompMdlNameGet(feat, &type, wname) != PRO_TK_NO_ERROR ) return status;
    if (cinfo->empty->find(wname) == cinfo->empty->end()) (*has_shape) = 1;
    return PRO_TK_NO_ERROR;
}

/* run this only *after* output_parts - need that information */
extern "c" void
find_empty_assemblies(struct creo_conv_info *cinfo)
{
    int steady_state = 0;
    if (cinfo->empty->size() == 0) return;
    while (!steady_state) {
	std::set<wchar_t *, WStrCmp>::iterator d_it;
	steady_state = 1;
	for (d_it = cinfo->assems->begin(); d_it != cinfo->parts->end(); d_it++) {
	    /* TODO - for each assem, verify at least one child is non-empty.  If all
	     * children are empty, add to empty set and unset steady_state. */
	    int has_shape = 0;
	    ProSolidFeatVisit(ProMdlToPart(model), assembly_check_empty, (ProFeatureFilterAction)assembly_filter, (ProAppData)&has_shape);
	}
    }
}

extern "c" void
output_assems(struct creo_conv_info *cinfo)
{
    std::set<wchar_t *, WStrCmp>::iterator d_it;
    for (d_it = cinfo->assems->begin(); d_it != cinfo->assems->end(); d_it++) {
	ProMdl parent = ProMdlnameInit(*d_it, PRO_ASSEM);
	output_assembly(cinfo, parent);
    }
}

/* routine to output the top level object that is currently displayed in Pro/E */
extern "c" void
output_top_level_object(struct creo_conv_info *cinfo, promdl model, promdltype type )
{
    wchar_t wname[10000];
    char name[10000];
    wchar *wname_saved;

    /* get object name */
    if (ProMdlNameGet( model, wname ) != PRO_TK_NO_ERROR ) return;
    (void)ProWstringToString(name, wname);

    /* save name */
    wname_saved = (wchar *)bu_calloc(wcslen(wname)+1, sizeof(wchar), "CREO name");
    wcsncpy(wname_saved, wname, wsclen(wname)+1);

    /* output the object */
    if ( type == PRO_MDL_PART ) {
	/* tessellate part and output triangles */
	cinfo->parts->insert(wname_saved);
	output_parts(cinfo);
    } else if ( type == PRO_MDL_ASSEMBLY ) {
	/* visit all members of assembly */
	cinfo->assems->insert(wname_saved);
	ProSolidFeatVisit(ProMdlToPart(model), assembly_gather, (ProFeatureFilterAction)assembly_filter, (ProAppData)&adata);
	output_parts(cinfo);
	find_empty_assemblies(cinfo);
	output_assems(cinfo);
    } else {
	bu_log("Object %s is neither PART nor ASSEMBLY, skipping", cinfo->curr_part_name );
    }

    /* TODO - Make a final toplevel comb with the file name to hold the orientation matrix */
    /* xform to rotate the model into standard BRL-CAD orientation */
    /*0 0 1 0 1 0 0 0 0 1 0 0 0 0 0 1*/

}



extern "C" void
doit( char *dialog, char *compnent, ProAppData appdata )
{
    ProError status;
    ProMdl model;
    ProMdlType type;
    ProLine tmp_line;
    ProCharLine astr;
    ProFileName msgfil;
    wchar_t *w_output_file;
    wchar_t *w_name_file;
    wchar_t *tmp_str;
    char output_file[128];
    char name_file[128];
    char log_file[128];
    int n_selected_names;
    char **selected_names;
    char logger_type_str[128];
    int ret_status=0;

    /* This replaces the global variables used in the original Pro/E converter */
    struct creo_conv_info *cinfo = new creo_conv_info;
    creo_conv_info_init(cinfo);

    ProStringToWstring( tmp_line, "Not processing" );
    status = ProUILabelTextSet( "creo_brl", "curr_proc", tmp_line );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to update dialog label for currently processed part\n" );
    }
#if 0
    status = ProUIDialogActivate( "creo_brl", &ret_status );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Error in creo-brl Dialog, error = %d\n",
		status );
	fprintf( stderr, "\t dialog returned %d\n", ret_status );
    }
#endif
    /* get logger type */
    status = ProUIRadiogroupSelectednamesGet( "creo_brl", "log_file_type_rg", &n_selected_names, &selected_names );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get log file type\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }
    sprintf(logger_type_str,"%s", selected_names[0]);
    ProStringarrayFree(selected_names, n_selected_names);

    /* get the name of the log file */
    status = ProUIInputpanelValueGet( "creo_brl", "log_file", &tmp_str );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get log file name\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }
    ProWstringToString( log_file, tmp_str );
    ProWstringFree( tmp_str );

    /* get the name of the output file */
    status = ProUIInputpanelValueGet( "creo_brl", "output_file", &w_output_file );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get output file name\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }
    ProWstringToString( output_file, w_output_file );
    ProWstringFree( w_output_file );

    /* get the name of the part number to part name mapping file */
    status = ProUIInputpanelValueGet( "creo_brl", "name_file", &w_name_file );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get name of part number to part name mapping file\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }
    ProWstringToString( name_file, w_name_file );
    ProWstringFree( w_name_file );

    /* get starting ident */
    status = ProUIInputpanelValueGet( "creo_brl", "starting_ident", &tmp_str );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get starting ident\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }

    ProWstringToString( astr, tmp_str );
    ProWstringFree( tmp_str );
    cinfo->reg_id = atoi( astr );
    V_MAX(cinfo->reg_id, 1);

    /* get max error */
    status = ProUIInputpanelValueGet( "creo_brl", "max_error", &tmp_str );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get max tesellation error\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }

    ProWstringToString( astr, tmp_str );
    ProWstringFree( tmp_str );
    cinfo->max_error = atof( astr );

    /* get min error */
    status = ProUIInputpanelValueGet( "creo_brl", "min_error", &tmp_str );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get min tesellation error\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }

    ProWstringToString( astr, tmp_str );
    ProWstringFree( tmp_str );
    cinfo->min_error = atof( astr );

    V_MAX(cinfo->max_error, cinfo->min_error);

    /* get the max angle control */
    status = ProUIInputpanelValueGet( "creo_brl", "max_angle_ctrl", &tmp_str );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get angle control\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }

    ProWstringToString( astr, tmp_str );
    ProWstringFree( tmp_str );
    cinfo->max_angle_cntrl = atof( astr );

    /* get the min angle control */
    status = ProUIInputpanelValueGet( "creo_brl", "min_angle_ctrl", &tmp_str );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get angle control\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }

    ProWstringToString( astr, tmp_str );
    ProWstringFree( tmp_str );
    cinfo->min_angle_cntrl = atof( astr );

    V_MAX(cinfo->max_angle_cntrl, cinfo->min_angle_cntrl);

    /* get the max to min steps */
    status = ProUIInputpanelValueGet( "creo_brl", "isteps", &tmp_str );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get max to min steps\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }

    ProWstringToString( astr, tmp_str );
    ProWstringFree( tmp_str );
    cinfo->max_to_min_steps = atoi( astr );
    if (cinfo->max_to_min_steps <= 0) {
	cinfo->max_to_min_steps = 0;
	cinfo->error_increment = 0;
	cinfo->angle_increment = 0;
    } else {
	if (ZERO((cinfo->max_error - cinfo->min_error)))
	    cinfo->error_increment = 0;
	else
	    cinfo->error_increment = (cinfo->max_error - cinfo->min_error) / (double)cinfo->max_to_min_steps;

	if (ZERO((cinfo->max_angle_cntrl - cinfo->min_angle_cntrl)))
	    cinfo->angle_increment = 0;
	else
	    cinfo->angle_increment = (cinfo->max_angle_cntrl - cinfo->min_angle_cntrl) / (double)cinfo->max_to_min_steps;

	if (cinfo->error_increment == 0 && cinfo->angle_increment == 0)
	    cinfo->max_to_min_steps = 0;
    }

    /* check if user wants to do any CSG */
    status = ProUICheckbuttonGetState( "creo_brl", "facets_only", &cinfo->do_facets_only );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get checkbutton setting (facetize only)\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }

    /* check if user wants to eliminate small features */
    status = ProUICheckbuttonGetState( "creo_brl", "elim_small", &cinfo->do_elims );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get checkbutton setting (eliminate small features)\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }

    /* check if user wants surface normals in the BOT's */
    status = ProUICheckbuttonGetState( "creo_brl", "get_normals", &cinfo->get_normals );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get checkbutton setting (extract surface normals)\n" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }

    if ( cinfo->do_elims ) {

	/* get the minimum hole diameter */
	status = ProUIInputpanelValueGet( "creo_brl", "min_hole", &tmp_str );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Failed to get minimum hole diameter\n" );
	    ProUIDialogDestroy( "creo_brl" );
	    return;
	}

	ProWstringToString( astr, tmp_str );
	ProWstringFree( tmp_str );
	cinfo->min_hole_diameter = atof( astr );
	V_MAX(cinfo->min_hole_diameter, 0.0);

	/* get the minimum chamfer dimension */
	status = ProUIInputpanelValueGet( "creo_brl", "min_chamfer", &tmp_str );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Failed to get minimum chamfer dimension\n" );
	    ProUIDialogDestroy( "creo_brl" );
	    return;
	}

	ProWstringToString( astr, tmp_str );
	ProWstringFree( tmp_str );
	cinfo->min_chamfer_dim = atof( astr );
	V_MAX(cinfo->min_chamfer_dim, 0.0);

	/* get the minimum round radius */
	status = ProUIInputpanelValueGet( "creo_brl", "min_round", &tmp_str );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Failed to get minimum round radius\n" );
	    ProUIDialogDestroy( "creo_brl" );
	    return;
	}

	ProWstringToString( astr, tmp_str );
	ProWstringFree( tmp_str );
	cinfo->min_round_radius = atof( astr );

	V_MAX(cinfo->min_round_radius, 0.0);

    } else {
	cinfo->min_hole_diameter = 0.0;
	cinfo->min_round_radius = 0.0;
	cinfo->min_chamfer_dim = 0.0;
    }

    /* open log file, if a name was provided */
    if ( strlen( log_file ) > 0 ) {
	if ( BU_STR_EQUAL( log_file, "stderr" ) ) {
	    cinfo->logger = stderr;
	} else if ( (cinfo->logger=fopen( log_file, "wb" ) ) == NULL ) {
	    (void)ProMessageDisplay(msgfil, "USER_ERROR", "Cannot open log file" );
	    ProMessageClear();
	    fprintf( stderr, "Cannot open log file\n" );
	    perror( "\t" );
	    ProUIDialogDestroy( "creo_brl" );
	    return;
	}

	/* Set logger type */
	if (BU_STR_EQUAL("Failure", logger_type_str))
	    cinfo->logger_type = LOGGER_TYPE_FAILURE;
	else if (BU_STR_EQUAL("Success", logger_type_str))
	    cinfo->logger_type = LOGGER_TYPE_SUCCESS;
	else if (BU_STR_EQUAL("Failure/Success", logger_type_str))
	    cinfo->logger_type = LOGGER_TYPE_FAILURE_OR_SUCCESS;
	else
	    cinfo->logger_type = LOGGER_TYPE_ALL;
    } else {
	cinfo->logger = (FILE *)NULL;
	cinfo->logger_type = LOGGER_TYPE_NONE;
    }

    /* open part name mapper file, if a name was provided */
    if ( strlen( name_file ) > 0 ) {
	FILE *name_fd;

	if ( cinfo->logger_type == LOGGER_TYPE_ALL ) {
	    fprintf( cinfo->logger, "Opening part name map file (%s)\n", name_file );
	}

	if ( (name_fd=fopen( name_file, "rb" ) ) == NULL ) {
	    struct bu_vls error_msg = BU_VLS_INIT_ZERO;
	    int dialog_return=0;
	    wchar_t w_error_msg[512];

	    if ( cinfo->logger_type == LOGGER_TYPE_ALL ) {
		fprintf( cinfo->logger, "Failed to open part name map file (%s)\n", name_file );
		fprintf( cinfo->logger, "%s\n", strerror( errno ) );
	    }

	    (void)ProMessageDisplay(msgfil, "USER_ERROR", "Cannot open part name file" );
	    ProMessageClear();
	    fprintf( stderr, "Cannot open part name file\n" );
	    perror( name_file );
	    status = ProUIDialogCreate( "creo_brl_gen_error", "creo_brl_gen_error" );
	    if ( status != PRO_TK_NO_ERROR ) {
		fprintf( stderr, "Failed to create error dialog (%d)\n", status );
	    }
	    (void)ProUIPushbuttonActivateActionSet( "creo_brl_gen_error",
		    "ok_button",
		    kill_gen_error_dialog, NULL );
	    bu_vls_printf( &error_msg, "\n\tCannot open part name file (%s)\n\t",
		    name_file );
	    bu_vls_strcat( &error_msg, strerror( errno ) );
	    ProStringToWstring( w_error_msg, bu_vls_addr( &error_msg ) );
	    status = ProUITextareaValueSet( "creo_brl_gen_error", "error_message", w_error_msg );
	    if ( status != PRO_TK_NO_ERROR ) {
		fprintf( stderr, "Failed to set message for error dialog (%d)\n", status );
	    }
	    bu_vls_free( &error_msg );
	    status = ProUIDialogActivate( "creo_brl_gen_error", &dialog_return );
	    if ( status != PRO_TK_NO_ERROR ) {
		fprintf( stderr, "Failed to activate error dialog (%d)\n", status );
	    }
	    ProUIDialogDestroy( "creo_brl" );
	    return;
	}

	/* create a hash table of part numbers to part names */
	if ( cinfo->logger_type == LOGGER_TYPE_ALL ) {
	    fprintf( cinfo->logger, "Creating name hash\n" );
	}

	ProStringToWstring( tmp_line, "Processing part name file" );
	status = ProUILabelTextSet( "creo_brl", "curr_proc", tmp_line );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Failed to update dialog label for currently processed part\n" );
	}
	status = ProUIDialogActivate( "creo_brl", &ret_status );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Error in creo-brl Dialog, error = %d\n",
		    status );
	    fprintf( stderr, "\t dialog returned %d\n", ret_status );
	}

	cinfo->name_hash = create_name_hash(cinfo, name_fd );
	fclose( name_fd );

    } else {
	if ( cinfo->logger_type == LOGGER_TYPE_ALL ) {
	    fprintf( cinfo->logger, "No name hash used\n" );
	}
	/* create an empty hash table */
	cinfo->name_hash = bu_hash_create( 512 );
    }

    /* Safety check - don't overwrite a pre-existing file */
    if (bu_file_exists(output_file, NULL)) {
	struct bu_vls error_msg = BU_VLS_INIT_ZERO;
	wchar_t w_error_msg[512];
	bu_vls_printf( &error_msg, "Cannot create file %s - file already exists.\n", output_file );
	ProStringToWstring( w_error_msg, bu_vls_addr( &error_msg ) );
	status = ProUIDialogCreate( "creo_brl_gen_error", "creo_brl_gen_error" );
	(void)ProUIPushbuttonActivateActionSet( "creo_brl_gen_error", "ok_button", kill_gen_error_dialog, NULL );
	status = ProUITextareaValueSet( "creo_brl_gen_error", "error_message", w_error_msg );
	status = ProUIDialogActivate( "creo_brl_gen_error", &dialog_return );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }

    /* open output file */
    if ( (cinfo->dbip = db_create(output_file, 5) ) == DBI_NULL ) {
	(void)ProMessageDisplay(msgfil, "USER_ERROR", "Cannot open output file" );
	ProMessageClear();
	fprintf( stderr, "Cannot open output file\n" );
	perror( "\t" );
	ProUIDialogDestroy( "creo_brl" );
	return;
    }
    cinfo->wdbp = wdb_dbopen(cinfo->dbip, RT_WDB_TYPE_DB_DISK);


    /* get the currently displayed model in Pro/E */
    status = ProMdlCurrentGet( &model );
    if ( status == PRO_TK_BAD_CONTEXT ) {
	(void)ProMessageDisplay(msgfil, "USER_NO_MODEL" );
	ProMessageClear();
	fprintf( stderr, "No model is displayed!!\n" );
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	ProUIDialogDestroy( "creo_brl" );
	creo_conv_info_free(cinfo);
	return;
    }

    /* get its type */
    status = ProMdlTypeGet( model, &type );
    if ( status == PRO_TK_BAD_INPUTS ) {
	(void)ProMessageDisplay(msgfil, "USER_NO_TYPE" );
	ProMessageClear();
	fprintf( stderr, "Cannot get type of current model\n" );
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	ProUIDialogDestroy( "creo_brl" );
	creo_conv_info_free(cinfo);
	return;
    }

    /* can only do parts and assemblies, no drawings, etc. */
    if ( type != PRO_MDL_ASSEMBLY && type != PRO_MDL_PART ) {
	(void)ProMessageDisplay(msgfil, "USER_TYPE_NOT_SOLID" );
	ProMessageClear();
	fprintf( stderr, "Current model is not a solid object\n" );
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	ProUIDialogDestroy( "creo_brl" );
	creo_conv_info_free(cinfo);
	return;
    }

    /* TODO -verify this is working correctly */
    ProUnitsystem us;
    ProUnititem lmu;
    ProUnititem mmu;
    ProUnitConversion conv;
    ProMdlPrincipalunitsystemGet(model, &us);
    ProUnitsystemUnitGet(&us, PRO_UNITTYPE_LENGTH, &lmu);
    ProUnitInit(model, L"mm", &mmu);
    ProUnitConversionGet(&lmu, &conv, &mmu);
    cinfo->creo_to_brl_conv = conv.scale;

    /* adjust tolerance for Pro/E units */
    cinfo->local_tol = cinfo->tol_dist / cinfo->creo_to_brl_conv;
    cinfo->local_tol_sq = cinfo->local_tol * cinfo->local_tol;

    cinfo->vert_tree_root = create_vert_tree();
    cinfo->norm_tree_root = create_vert_tree();

    /* output the top level object
     * this will recurse through the entire model
     */
    output_top_level_object(cinfo, model, type );

    /* let user know we are done */
    ProStringToWstring( tmp_line, "Conversion complete" );
    ProUILabelTextSet( "creo_brl", "curr_proc", tmp_line );

    creo_conv_info_free(cinfo);
    return;
}



extern "C" void
elim_small_activate( char *dialog_name, char *button_name, ProAppData data )
{
    ProBoolean state;
    ProError status;

    status = ProUICheckbuttonGetState( dialog_name, button_name, &state );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "checkbutton activate routine: failed to get state\n" );
	return;
    }

    if ( state ) {
	status = ProUIInputpanelEditable( dialog_name, "min_hole" );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Failed to activate \"minimum hole diameter\"\n" );
	    return;
	}
	status = ProUIInputpanelEditable( dialog_name, "min_chamfer" );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Failed to activate \"minimum chamfer dimension\"\n" );
	    return;
	}
	status = ProUIInputpanelEditable( dialog_name, "min_round" );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Failed to activate \"minimum round radius\"\n" );
	    return;
	}
    } else {
	status = ProUIInputpanelReadOnly( dialog_name, "min_hole" );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Failed to de-activate \"minimum hole diameter\"\n" );
	    return;
	}
	status = ProUIInputpanelReadOnly( dialog_name, "min_chamfer" );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Failed to de-activate \"minimum chamfer dimension\"\n" );
	    return;
	}
	status = ProUIInputpanelReadOnly( dialog_name, "min_round" );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Failed to de-activate \"minimum round radius\"\n" );
	    return;
	}
    }
}

extern "C" void
do_quit( char *dialog, char *compnent, ProAppData appdata )
{
    ProUIDialogDestroy( "creo_brl" );
}

/* driver routine for converting CREO to BRL-CAD */
extern "C" int
creo_brl( uiCmdCmdId command, uiCmdValue *p_value, void *p_push_cmd_data )
{
    ProFileName msgfil;
    ProError status;
    int ret_status=0;

    ProStringToWstring(msgfil, CREO_BRL_MSG_FILE);

    ProMessageDisplay(msgfil, "USER_INFO", "Launching creo_brl...");

    /* use UI dialog */
    status = ProUIDialogCreate( "creo_brl", "creo_brl" );
    if ( status != PRO_TK_NO_ERROR ) {
	struct bu_vls vls = BU_VLS_INIT_ZERO;

	bu_vls_printf(&vls, "Failed to create dialog box for creo-brl, error = %d\n", status );
	ProMessageDisplay(msgfil, "USER_INFO", bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return 0;
    }

    status = ProUICheckbuttonActivateActionSet( "creo_brl", "elim_small", elim_small_activate, NULL );
    if ( status != PRO_TK_NO_ERROR ) {
	struct bu_vls vls = BU_VLS_INIT_ZERO;

	bu_vls_printf(&vls, "Failed to set action for \"eliminate small features\" checkbutton, error = %d\n", status );
	ProMessageDisplay(msgfil, "USER_INFO", bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return 0;
    }

    status = ProUIPushbuttonActivateActionSet( "creo_brl", "doit", doit, NULL );
    if ( status != PRO_TK_NO_ERROR ) {
	struct bu_vls vls = BU_VLS_INIT_ZERO;

	bu_vls_printf(&vls, "Failed to set action for 'Go' button\n" );
	ProMessageDisplay(msgfil, "USER_INFO", bu_vls_addr(&vls));
	ProUIDialogDestroy( "creo_brl" );
	bu_vls_free(&vls);
	return 0;
    }

    status = ProUIPushbuttonActivateActionSet( "creo_brl", "quit", do_quit, NULL );
    if ( status != PRO_TK_NO_ERROR ) {
	struct bu_vls vls = BU_VLS_INIT_ZERO;

	bu_vls_printf(&vls, "Failed to set action for 'Quit' button\n" );
	ProMessageDisplay(msgfil, "USER_INFO", bu_vls_addr(&vls));
	ProUIDialogDestroy( "creo_brl" );
	bu_vls_free(&vls);
	return 0;
    }

    status = ProUIDialogActivate( "creo_brl", &ret_status );
    if ( status != PRO_TK_NO_ERROR ) {
	struct bu_vls vls = BU_VLS_INIT_ZERO;

	bu_vls_printf(&vls, "Error in creo-brl Dialog, error = %d\n",
		status );
	bu_vls_printf(&vls, "\t dialog returned %d\n", ret_status );
	ProMessageDisplay(msgfil, "USER_INFO", bu_vls_addr(&vls));
	bu_vls_free(&vls);
    }

    return 0;
}



/* this routine determines whether the "creo-brl" menu item in Pro/E
 * should be displayed as available or greyed out
 */
extern "C" static uiCmdAccessState
creo_brl_access( uiCmdAccessMode access_mode )
{
    /* doing the correct checks appears to be unreliable */
    return ACCESS_AVAILABLE;
}

extern "C" int user_initialize()
{
    ProError status;
    ProCharLine astr;
    ProFileName msgfil;
    int i;
    uiCmdCmdId cmd_id;
    wchar_t errbuf[80];

    ProStringToWstring(msgfil, CREO_BRL_MSG_FILE);

    /* Pro/E says always check the size of w_char */
    status = ProWcharSizeVerify (sizeof (wchar_t), &i);
    if ( status != PRO_TK_NO_ERROR || (i != sizeof (wchar_t)) ) {
	sprintf(astr, "ERROR wchar_t Incorrect size (%d). Should be: %d",
		sizeof(wchar_t), i );
	status = ProMessageDisplay(msgfil, "USER_ERROR", astr);
	printf("%s\n", astr);
	ProStringToWstring(errbuf, astr);
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	return -1;
    }

    /* add a command that calls our creo-brl routine */
    status = ProCmdActionAdd( "CREO-BRL", (uiCmdCmdActFn)creo_brl, uiProe2ndImmediate,
	    creo_brl_access, PRO_B_FALSE, PRO_B_FALSE, &cmd_id );
    if ( status != PRO_TK_NO_ERROR ) {
	sprintf( astr, "Failed to add creo-brl action" );
	fprintf( stderr, "%s\n", astr);
	ProMessageDisplay(msgfil, "USER_ERROR", astr);
	ProStringToWstring(errbuf, astr);
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	return -1;
    }

    /* add a menu item that runs the new command */
    status = ProMenubarmenuPushbuttonAdd( "File", "CREO-BRL", "CREO-BRL", "CREO-BRL-HELP",
	    "File.psh_exit", PRO_B_FALSE, cmd_id, msgfil );
    if ( status != PRO_TK_NO_ERROR ) {
	sprintf( astr, "Failed to add creo-brl menu button" );
	fprintf( stderr, "%s\n", astr);
	ProMessageDisplay(msgfil, "USER_ERROR", astr);
	ProStringToWstring(errbuf, astr);
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	return -1;
    }

    ShowMsg();

    /* let user know we are here */
    ProMessageDisplay( msgfil, "OK" );
    (void)ProWindowRefresh( PRO_VALUE_UNUSED );

    return 0;
}

extern "C" void user_terminate()
{
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
