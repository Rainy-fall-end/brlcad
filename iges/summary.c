/*
 *  Authors -
 *	John R. Anderson
 *	Susanne L. Muuss
 *	Earl P. Weaver
 *
 *  Source -
 *	VLD/ASB Building 1065
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "./iges_struct.h"
#include "./iges_extern.h"

Summary()
{
int i;

	printf( "Summary of entity types found:\n" );
	for( i=0 ; i<=ntypes ; i++ )
	{
		if( typecount[i].count != 0 )
			printf( "%10d %s\n",typecount[i].count , typecount[i].name );
	}

	printf( "Independent entities:\n" );
	for( i=0 ; i<totentities ; i++ )
	{
		int subord;

		subord = (dir[i]->status/10000)%100;
		if( !subord )
		{
			int j;

			for( j=0 ; j<ntypes ; j++ )
				if( typecount[j].type == dir[i]->type )
					break;
			if( j >= ntypes )
				j = 0;
			printf( "\t#%d at D%07d (%s)\n" , i , dir[i]->direct, typecount[j].name );
		}
	}
}
