/*                     G _ T R A N S F E R . C
 * BRL-CAD
 *
 * Copyright (c) 2006 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; see the file named COPYING for more
 * information.
 */
/** @file g_transfer.c
 *
 * Author: Christopher Sean Morrison
 *
 * Relatively simple example database transfer program that shows how
 * to open a database, extract a serialized version of specified
 * geometry objects, transfer those objects to a remove host, and
 * utiling standard librt routines on the remote objects.  The
 * transfer program interface is designed in a simple ttcp fashion
 * using libpkg.
 *
 * To compile from an install:
 * gcc -I/usr/brlcad/include -L/usr/brlcad/lib -o g_transfer g_transfer.c -lrt -lbu
 *
 */

#include "common.h"

/* system headers */
#include <stdlib.h>
#include <signal.h>
#include <string.h>

/* interface headers */
#include "machine.h"
#include "raytrace.h"
#include "bu.h"
#include "pkg.h"


/* used by the client to pass the dbip and opened transfer file
 * descriptor.
 */
typedef struct _my_data_ {
    struct pkg_conn *connection;
    const char *server;
    int port;
} my_data;

/* simple network transport protocol. connection starts with a HELO,
 * then a variable number of GEOM messages, then a CIAO when done.
 */
#define MAGIC_ID	"G_TRANSFER"
#define MSG_HELO	1
#define MSG_GEOM	2
#define MSG_CIAO	3

/* static size used for temporary string buffers */
#define MAX_DIGITS	32


/** print a usage statement when invoked with bad, help, or no arguments
 */
void
usage(const char *msg, const char *argv0)
{
    if (msg) {
	bu_log("%s\n", msg);
    }
    bu_log("Usage: %s [-t] [-p#] host gfile [geometry ...]\n\t-p#\tport number to send to (default 2000)\n\thost\thostname or IP address of receiving server\n\tgfile\tBRL-CAD .g database file\n\tgeometry\tname(s) of geometry to send (OPTIONAL)\n", argv0 ? argv0 : "g_transfer");
    bu_log("Usage: %s -r [-p#]\n\t-p#\tport number to listen on (default 2000)\n", argv0 ? argv0 : "g_transfer");
    exit(1);
}


void
validate_port(int port) {
    if (port < 0) {
	bu_bomb("Invalid negative port range\n");
    }
}


#define DEFAULT_DB_TITLE "Untitled BRL-CAD Database"

struct db_i *
db_open_inmem(void)
{
    register struct db_i *dbip = DBI_NULL;
    register int i;

    BU_GETSTRUCT( dbip, db_i );
    dbip->dbi_eof = -1L;
    dbip->dbi_fd = -1;
    dbip->dbi_fp = NULL;
    dbip->dbi_mf = NULL; /* fake mapped file for contents? */
    dbip->dbi_read_only = 1;

    /* Initialize fields */
    for( i=0; i<RT_DBNHASH; i++ ) {
	dbip->dbi_Head[i] = DIR_NULL;
    }

    dbip->dbi_local2base = 1.0;		/* mm */
    dbip->dbi_base2local = 1.0;
    dbip->dbi_title = DEFAULT_DB_TITLE;
    dbip->dbi_uses = 1;
    dbip->dbi_filename = NULL;
    dbip->dbi_filepath = NULL;
    dbip->dbi_version = 5;

    /* XXX need to stash an ident record so it's valid maybe? */
    /* see db_fwrite_ident() */

    bu_ptbl_init( &dbip->dbi_clients, 128, "dbi_clients[]" );
    dbip->dbi_magic = DBI_MAGIC;		/* Now it's valid */

    return dbip;
}


struct db_i *
db_create_inmem(void) {
    struct db_i *dbip;
    struct bu_external obj;
    struct bu_attribute_value_set avs;
    struct bu_vls units;
    struct bu_external attr;
    int result;

    dbip = db_open_inmem();

    /* create the header record */
    db5_export_object3(&obj, DB5HDR_HFLAGS_DLI_HEADER_OBJECT,
		       NULL, 0, NULL, NULL,
		       DB5_MAJORTYPE_RESERVED, 0,
		       DB5_ZZZ_UNCOMPRESSED, DB5_ZZZ_UNCOMPRESSED );
    /* XXX add objdata to mapped file pointer */

    /* Second, create the attribute-only _GLOBAL object */
    bu_vls_init( &units );
    bu_vls_printf( &units, "%.25e", dbip->dbi_local2base );
    
    bu_avs_init( &avs, 4, "db_create_inmem" );
    bu_avs_add( &avs, "title", dbip->dbi_title );
    bu_avs_add( &avs, "units", bu_vls_addr(&units) );

    db5_export_attributes( &attr, &avs );
    db5_export_object3(&obj, DB5HDR_HFLAGS_DLI_APPLICATION_DATA_OBJECT,
		       DB5_GLOBAL_OBJECT_NAME, DB5HDR_HFLAGS_HIDDEN_OBJECT, &attr, NULL,
		       DB5_MAJORTYPE_ATTRIBUTE_ONLY, 0,
		       DB5_ZZZ_UNCOMPRESSED, DB5_ZZZ_UNCOMPRESSED );
    /* XXX add objdata to mapped file pointer */

    bu_free_external( &obj );
    bu_free_external( &attr );
    bu_avs_free( &avs );
    bu_vls_free( &units );

    return dbip;
}



void
server_helo(struct pkg_conn *connection, char *buf)
{
    /* should not encounter since we listened for it specifically
     * before beginning processing of packets.
     */	
    bu_log("Unexpected HELO encountered\n");
    free(buf);
}


void
server_geom(struct pkg_conn *connection, char *buf)
{
    struct bu_external ext;

    /* length was stashed as string in first 32 bytes for
     * convenience only, more efficient methods possible.
     */
    int buflen = atoi(buf);

    BU_INIT_EXTERNAL(&ext);
    ext.ext_buf = buf + 32;
    ext.ext_nbytes = buflen;

    bu_log("GEOM encountered\n");
    bu_log("data is %d bytes long\n", buflen);

#if 0
    wdb_decode_dbip(interp, argv[3], &dbip);
or
    dbip = wdb_prep_dbip(interp, argv[i]);
then
    wdbp = wdb_dbopen(dbip, RT_WDB_TYPE_DB_INMEM);
#endif

    free(buf);
}


void
server_ciao(struct pkg_conn *connection, char *buf)
{
    bu_log("CIAO encountered\n");
    free(buf);
}


/** start up a server that listens for a single client.
 */
void
run_server(int port) {
    struct pkg_conn *client;
    int netfd;
    char portname[MAX_DIGITS] = {0};
    int pkg_result  = 0;

    struct pkg_switch callbacks[] = {
	{MSG_HELO, server_helo, "HELO"},
	{MSG_GEOM, server_geom, "GEOM"},
	{MSG_CIAO, server_ciao, "CIAO"},
	{0, 0, (char *)0}
    };
    
    validate_port(port);

    /* start up the server on the given port */
    snprintf(portname, MAX_DIGITS - 1, "%d", port);
    netfd = pkg_permserver(portname, "tcp", 0, 0);
    if (netfd < 0) {
	bu_bomb("Unable to start the server");
    }

    /* listen for a good client indefinitely */
    do {
	client = pkg_getclient(netfd, callbacks, NULL, 0);
	if (client == PKC_NULL) {
	    bu_log("Connection seems to be busy, waiting...\n");
	    sleep(10);
	    continue;
	} else if (client == PKC_ERROR) {
	    bu_log("Fatal error accepting client connection.\n");
	    pkg_close(client);
	    return;
	}

	/* got a connection, process it */
	if (pkg_bwaitfor(MSG_HELO, client) == NULL) {
	    bu_log("Failed to process the client connection, still waiting\n");
	    pkg_close(client);
	}
    } while (client <= 0);

    /* read from the connection */
    bu_log("Processing objects from client\n");
    do {
	/* suck in data from the network */
	pkg_result = pkg_suckin(client);
	if (pkg_result < 0) {
	    bu_log("Seemed to have trouble sucking in packets.\n");
	    break;
	} else if (pkg_result == 0) {
	    bu_log("Client closed the connection.\n");
	    break;
	}

	/* process packets received */
	pkg_result = pkg_process(client);
	if (pkg_result < 0) {
	    bu_log("Unable to process packets? Wierd.\n");
	} else {
	    bu_log("Processed %d packet%s\n", pkg_result, pkg_result > 1 ? "s" : "");
	}
    } while (client != NULL);

    /* shut down the server */
    pkg_close(client);
}


/** base routine that the client uses to send an object to the server.
 * this is the hook callback function for both the primitives and
 * combinations encountered during a db_functree() traversal.
 *
 * returns 0 if unsuccessful 
 * returns 1 if successful
 */
void
send_to_server(struct db_i *dbip, struct directory *dp, genptr_t connection)
{
    my_data *stash;
    struct bu_external ext;
    char length[MAX_DIGITS] = {0};
    int bytes_sent = 0;

    RT_CK_DBI(dbip);
    RT_CK_DIR(dp);

    stash = (my_data *)connection;

    if (db_get_external(&ext, dp, dbip) < 0) {
	bu_log("Failed to read %s, skipping\n", dp->d_namep);
	return;
    }
    snprintf(length, MAX_DIGITS - 1, "%d", ext.ext_nbytes);
    
    /* send the external representation over the wire */
    bu_log("Sending %s\n",dp->d_namep);

    /* pad the data with the length in ascii for convenience */
    bytes_sent = pkg_2send(MSG_GEOM, length, MAX_DIGITS, ext.ext_buf, ext.ext_nbytes, stash->connection);
    if (bytes_sent < 0) {	
	pkg_close(stash->connection);
	bu_log("Unable to successfully send %s to %s, port %d.\n", dp->d_namep, stash->server, stash->port);
	return;
    }

    /* our responsibility to free the stuff we got */
    bu_free_external(&ext);
}


/** start up a client that connects to the given server, and sends
 *  serialized .g data.  if the user specified geometry, only that
 *  geometry is sent via send_to_server().
 */
void
run_client(const char *server, int port, struct db_i *dbip, int geomc, const char **geomv)
{
    my_data stash;
    int i;
    struct directory *dp;
    struct bu_external ext;
    struct db_tree_state init_state; /* state table for the heirarchy walker */
    char s_port[MAX_DIGITS] = {0};
    int bytes_sent = 0;

    RT_CK_DBI(dbip);

    /* open a connection to the server */
    validate_port(port);

    snprintf(s_port, MAX_DIGITS - 1, "%d", port);
    stash.connection = pkg_open(server, s_port, "tcp", NULL, NULL, NULL, NULL);
    if (stash.connection == PKC_ERROR) {
	bu_log("Connection to %s, port %d, failed.\n", server, port);
	bu_bomb("ERROR: Unable to open a connection to the server\n");
    }
    stash.server = server;
    stash.port = port;

    /* let the server know we're cool. */
    bytes_sent = pkg_send(MSG_HELO, MAGIC_ID, strlen(MAGIC_ID) + 1, stash.connection);
    if (bytes_sent < 0) {
	pkg_close(stash.connection);
	bu_log("Connection to %s, port %d, seems faulty.\n", server, port);
	bu_bomb("ERROR: Unable to communicate with the server\n");
    }

    bu_log("Database title is:\n%s\n", dbip->dbi_title);
    bu_log("Units: %s\n", rt_units_string(dbip->dbi_local2base));

    /* send geometry to the server */
    if (geomc > 0) {
	/* geometry was specified. look it up and process the
	 * hierarchy using db_functree() where all combinations and
	 * primitives are sent that get encountered.
	 */
	for (i = 0; i < geomc; i++) {
	    dp = db_lookup(dbip, geomv[i], LOOKUP_NOISY);
	    if (dp == DIR_NULL) {
		pkg_close(stash.connection);
		bu_log("Unable to lookup %s\n", geomv[i]);
		bu_bomb("ERROR: requested geometry could not be found\n");
	    }
	    db_functree(dbip, dp, send_to_server, send_to_server, &rt_uniresource, (genptr_t)&stash);
	}
    } else {
	/* no geometry was specified so traverse the array of linked
	 * lists contained in the database instance and send
	 * everything.
	 */
	for (i = 0; i < RT_DBNHASH; i++) {
	    for (dp = dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
		send_to_server(dbip, dp, (genptr_t)&stash);
	    }
	}
    }

    /* let the server know we're done.  not necessary, but polite. */
    bytes_sent = pkg_send(MSG_CIAO, "BYE", 4, stash.connection);
    if (bytes_sent < 0) {
	bu_log("Unable to cleanly disconnect from %s, port %d.\n", server, port);
    }

    /* flush output and close */
    pkg_close(stash.connection);

    return;
}


/** main application
 */
int
main(int argc, char *argv[]) {
    const char * const argv0 = argv[0];
    int c;
    int server = 0; /* not a server by default */
    int port = 2000;

    /* client stuff */
    const char *server_name = NULL;
    const char *geometry_file = NULL;
    const char ** geometry = NULL;
    int ngeometry = 0;
    struct db_i *dbip = NULL;

    if (argc < 2) {
	usage("ERROR: Missing arguments", argv[0]);
    }

    /* process the command-line arguments after the application name */
    while ((c = bu_getopt(argc, argv, "tTrRp:P:hH")) != EOF) {
	switch (c) {
	    case 't':
	    case 'T':
		/* sending */
		server = 0;
		break;
	    case 'r':
	    case 'R':
		/* receiving */
		server = 1;
		break;
	    case 'p':
	    case 'P':
		port = atoi(bu_optarg);
		break;
	    case 'h':
	    case 'H':
		/* help */
		usage(NULL, argv0);
		break;
	    default:
		usage("ERROR: Unknown argument", argv0);
	}
    }

    argc -= bu_optind;
    argv += bu_optind;

    if (server) {
	if (argc > 0) {
	    usage("ERROR: Unexpected extra arguments", argv0);
	}

	/* mark the database as in-memory only */
	//	XXX = wdb_dbopen(dbip, RT_WDB_TYPE_DB_INMEM);

	/* ignore broken pipes */
	(void)signal(SIGPIPE, SIG_IGN);

	/* fire up the server */
	bu_log("Listening on port %d\n", port);
	run_server(port);

	return 0;
    }

    /* prep up the client */
    if (argc < 1) {
	usage("ERROR: Missing hostname and geometry file arguments", argv0);
    } else if (argc < 2) {
	usage("ERROR: Missing geometry file argument", argv0);
    } else {
	geometry = (const char **)(argv + 2);
	ngeometry = argc - 2;
    }

    server_name = *argv++;
    geometry_file = *argv++;

    /* make sure the geometry file exists */
    if (!bu_file_exists(geometry_file)) {
	bu_log("Geometry file does not exist: %s\n", geometry_file);
	bu_bomb("Need a BRL-CAD .g geometry database file\n");
    }

    /* XXX fixed in latest db_open(), but call for now just in case
       until 7.8.0 release */
    rt_init_resource( &rt_uniresource, 0, NULL );

    /* make sure the geometry file is a geometry database, get a
     * database instance pointer.
     */
    dbip = db_open(geometry_file, "r");
    if (dbip == DBI_NULL) {
	bu_log("Cannot open %s\n", geometry_file);
	perror(argv0);
	bu_bomb("Need a geometry file");
    }

    /* load the database directory into memory */
    if (db_dirbuild(dbip) < 0) {
	db_close(dbip);
	bu_log("Unable to load the database directory for file: %s\n", geometry_file);
	bu_bomb("Can't read geometry file");
     }

    /* fire up the client */
    bu_log("Connecting to %s, port %d\n", server_name, port);
    run_client(server_name, port, dbip, ngeometry, geometry);

    /* done with the database */
    db_close(dbip);

    return 0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */