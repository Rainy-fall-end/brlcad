/*	B O T . H
 *
 *  Header file for the "bot" specific structure.  This is shared between
 *  the bot, and ars at the moment.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 2001-2004 by the United States Army.
 *	All rights reserved.
 */


struct bot_specific {
    unsigned char bot_mode;
    unsigned char bot_orientation;
    unsigned char bot_flags;
    int bot_ntri;
    fastf_t *bot_thickness;
    struct bu_bitv *bot_facemode;
    genptr_t bot_facelist;	/* head of linked list */
    genptr_t *bot_facearray;	/* head of face array */
    unsigned int bot_tri_per_piece;	/* log # tri per peice. 1 << bot_ltpp is tri per piece */
    
};

RT_EXPORT BU_EXTERN(void rt_bot_prep_pieces,
		    (struct bot_specific	*bot,
		     struct soltab		*stp,
		     int			ntri,
		     const struct bn_tol	*tol));

RT_EXPORT BU_EXTERN(int rt_botface,
		    (struct soltab		*stp,
		     struct bot_specific	*bot,
		     fastf_t			*ap,
		     fastf_t			*bp,
		     fastf_t			*cp,
		     int			face_no,
		     const struct bn_tol	*tol));

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
