/***************************************************************************
                          main.c  -  description
                             -------------------
    begin                : Fri Jan 31 2003
    copyright            : (C) 2003 by Anders Ekberg
    email                : anders.ekberg@bth.se
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "caputils/caputils.h"
#include "caputils/stream.h"
#include "caputils/filter.h"

#include <stdio.h>
#include <net/if_arp.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#define __STDC_FORMAT_MACROS
#include <stdint.h>
#include <inttypes.h>

#define VERSION "2"
// To add a new network type:
// add a define for the ethernet type.
// add a structure for the type:

static const char* program_name;
static int keep_running = 1;

static struct option long_options[]= {
	{"pkts",  required_argument, 0, 'p'},
	{"iface", required_argument, 0, 'i'},
	{"help",  no_argument,       0, 'h'},
	{0, 0, 0, 0} /* sentinel */
};

static void show_usage(void){
	printf("%s-" VERSION " (libcap_utils-" CAPUTILS_VERSION ")\n", program_name);
	printf("(C) 2003 Anders Ekberg <anders.ekberg@bth.se>\n");
	printf("(C) 2012 Vamsi Krishna Konakalla <xvk@bth.se>\n");
	printf("(C) 2012 David Sveningsson <david.sveningsson@bth.se>\n\n");
	printf("Usage: %s [OPTIONS] STREAM\n"
	       "  -p, --pkts=INT       Number of pkts to show [default all]\n"
	       "  -h, --help           This text\n"
	       "\n", program_name);
	filter_from_argv_usage();
}

int main (int argc, char **argv){
	/* extract program name from path. e.g. /path/to/MArCd -> MArCd */
	const char* separator = strrchr(argv[0], '/');
	if ( separator ){
		program_name = separator + 1;
	} else {
		program_name = argv[0];
	}

	struct filter filter;                 // filter to filter arguments
	stream_t stream;                      // stream to read from
	const char *iface = NULL;             // ethernet iface (used only when using ethernet multicast)
	unsigned long int max_packets = 0;    // stop after N packets

	/* Create filter from command line arguments */
	if ( filter_from_argv(&argc,argv, &filter) != 0) {
		fprintf(stderr, "%s: could not create filter", program_name);
		return 1;
	}

	/* Parse command line arguments (filter arguments has been consumed) */
	int op, option_index;
	while ( (op = getopt_long(argc, argv, "hi:p:", long_options, &option_index)) != -1 ){
		switch ( op ){
		case 0:   /* long opt */
		case '?': /* unknown opt */
			break;

		case 'i': /* --iface */
			iface = optarg;
			break;

		case 'p': /* --pkts */
			max_packets = atoi(optarg);
			break;

		case 'h':
			show_usage();
			return 0;

		default:
			printf ("?? getopt returned character code 0%o ??\n", op);
		}
	}

	/* No stream address was passed */
	if ( optind == argc ){
		fprintf(stderr, "No stream address was specified\n");
		show_usage();
		return 1;
	}

	/* Open stream */
	int ret;
	if ( (ret=stream_from_getopt(&stream, argv, optind, argc, iface, "-", program_name, 0)) != 0) {
		return 1;
	}
	const struct stream_stat* stat  = stream_get_stat(stream);

	/* show info about stream */
	stream_print_info(stream, stderr);

	/* read initial packet to initialize variables */
	cap_head* caphead;
	if ( (ret=stream_read (stream, &caphead, &filter, NULL)) != 0 ){
		fprintf(stderr, "%s: stream_read() failed: %s\n", program_name, caputils_error_string(ret));
		return 1;
	}

	picotime time_offset = caphead->ts;
	picotime last = {0,0};
	char last_CI[8] = {0,};

	while (keep_running){
		switch ( (ret=stream_read(stream, &caphead, &filter, NULL)) ){
		case -1: /* eof */
			keep_running = 0;
		case EAGAIN:
		case EINTR:
			continue;
		case 0:
			break;
		default:
			fprintf(stderr, "%s: stream_read() failed: %s\n", program_name, caputils_error_string(ret));
			break;
		}

		picotime cur = timepico_sub(caphead->ts, time_offset);
		picotime delta = timepico_sub(cur, last);

		if ( (signed int)delta.tv_sec < 0 ){
			fprintf(stderr, "%s: "
			        "sanity problem; current packet arrived prior to previous\n"
			        "\t current: CI=%.8s timestamp=%d.%012"PRIu64"\n"
			        "\tprevious: CI=%.8s timestamp=%d.%012"PRIu64"\n"
			        "Verify that stream is filtered to a single direction only.\n",
			        program_name,
			        caphead->nic, cur.tv_sec, cur.tv_psec,
			        last_CI, last.tv_sec, last.tv_psec);
		}

		fprintf(stdout, "%d.%012"PRIu64" %d.%012"PRIu64"\n", cur.tv_sec, cur.tv_psec, delta.tv_sec, delta.tv_psec);
		last = cur;
		memcpy(last_CI, caphead->nic, 8);

		if( max_packets > 0 && stat->read >= max_packets ) {
			break; /* Read enough pkts lets break. */
		}
	}

	fprintf(stderr, "%s: There was a total of %'"PRIu64" packets read.\n", program_name, stat->read);
	stream_close(stream);

	return 0;
}
