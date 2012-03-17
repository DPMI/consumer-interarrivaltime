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
#include <qd/qd_real.h>

#define VERSION "2"
// To add a new network type:
// add a define for the ethernet type.
// add a structure for the type:

static const char* program_name;
static qd_real timeOffset;

static struct option long_options[]= {
	{"pkts", required_argument, 0, 'p'},
	{"help", no_argument,       0, 'h'},
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

	qd_real maxDiffTime, minDiffTime, diffTime,pkt1,pkt2; // when does the next sample occur,sample interval time,

	extern int optind, opterr, optopt;
	register int op;
	int option_index;

	//libcap .7
	struct filter filter; // filter to filter arguments
	stream_t stream; // stream to read from
	//struct file_header head;
	struct cap_header *caphead;// cap_head *caphead;
	char *iface = 0;
	//FILE* infile;
	struct timeval tv = {2,0} ;

	if ( filter_from_argv(&argc,argv, &filter) != 0) {
		fprintf(stderr, "%s: could not create filter", program_name);
		return 1;
	}

	op=0;
	int ab,ac,ad,ae;

	double pkts;
	//myfilter.index=0;
	ab=optind;
	ac=opterr;
	ad=optopt;
	ae=op;


	//myfilter=createfilter(argc,argv);

	optind=ab;
	opterr=ac;
	optopt=ad;
	op=ae;
	pkts=-1;

	while ( (op = getopt_long(argc, argv, "hp:", long_options, &option_index)) != -1 ){
		switch ( op ){
		case 0:   /* long opt */
		case '?': /* unknown opt */
			break;

		case 'p':
			pkts = atoi(optarg);
			break;

		case 'h':
			show_usage();
			return 0;

		default:
			printf ("?? getopt returned character code 0%o ??\n", op);
		}
	}

	if ( optind == argc ){
		show_usage();
		return 1;
	}

	int ret;

	/* open stream */
	if ( (ret=stream_from_getopt(&stream, argv, optind, argc, iface, "-", program_name, 0)) != 0) {
		return 1;
	}

	stream_print_info(stream, stderr);

//Begin Packet processing

//  stream read..
	ret = stream_read (stream,&caphead,&filter,&tv);

	//read_filter_post(&infile, data, size, myfilter);
	//caphead=(cap_head*)data;
	pkt1=(qd_real)(double)caphead->ts.tv_sec+(qd_real)(double)(caphead->ts.tv_psec/PICODIVIDER);
	pkt2=(qd_real)(double)caphead->ts.tv_sec+(qd_real)(double)(caphead->ts.tv_psec/PICODIVIDER);
	timeOffset=floor(pkt1);
	pkt1-=timeOffset;
	minDiffTime=10e6;
	maxDiffTime=0.00001;

	double readPkts=2;
	ret = stream_read (stream,&caphead,&filter,&tv);
	while (ret == 0)// while (feof(infile)==0 )
		{
			//caphead=(cap_head*)data;
			pkt2=(qd_real)(double)caphead->ts.tv_sec+(qd_real)(double)(caphead->ts.tv_psec/PICODIVIDER);
			pkt2-=timeOffset;
			diffTime=pkt2-pkt1;
			if(diffTime<-1e-7) {
				std::cerr << "Sanity problem; pkt2 arrived prior to pkt1, check timestamp:  "<< std::setiosflags(std::ios::fixed) << std::setprecision(12)<< to_double(pkt2) << std::endl;
			}
			// cout << setiosflags(ios::fixed) << setprecision(6) << to_double(lastEvent+timeOffset)<< ":" << sampleValue << endl;
			std::cout << setiosflags(std::ios::fixed) << std::setprecision(12)<< to_double(pkt2)<<"\t"<<to_double(diffTime) << std::endl;
			pkt1=pkt2;

			if(pkts>0 && (readPkts+1)>pkts) {
				/* Read enough pkts lets break. */
				break;
			}
			ret = stream_read (stream,&caphead,&filter,&tv);
			readPkts++;
		}

//End Packet processing



	// dealloc_buffer(&data);
	//close_cap_file(&infile);

	stream_close(stream);
	return 0;
}
