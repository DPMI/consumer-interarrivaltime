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

#ifdef __cplusplus
extern "C"{
#endif
#include <cap_utils.h>
#ifdef __cplusplus
}
#endif


#include <net/if_arp.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <math.h>
#include <qd.h>

#define PICODIVIDER (double)1.0e12

#define STPBRIDGES 0x0026
#define CDPVTP 0x016E

// To add a new network type:
// add a define for the ethernet type.
// add a structure for the type:

qd_real timeOffset;

int main (int argc, char **argv)
{

  qd_real maxDiffTime, minDiffTime, diffTime,pkt1,pkt2; // when does the next sample occur,sample interval time,

  extern int optind, opterr, optopt;
  register int op;
  int this_option_optind;
  int option_index;
  static struct option long_options[]= {
            {"pkts", 0, 0, 'p'},
            {"help", 0, 0, 'h'},
            {0, 0, 0, 0}
        };


 
  struct file_header head;
  cap_head *caphead;
  FILE* infile;
  char *filename, *comments;
  int l, size;
  u_char* data;
  struct filter myfilter;

  op=0;
  int ab,ac,ad,ae;

  double pkts;
  myfilter.index=0;
  ab=optind;
  ac=opterr;
  ad=optopt;
  ae=op;


  myfilter=createfilter(argc,argv);

  optind=ab;
  opterr=ac;
  optopt=ad;
  op=ae;
  pkts=-1;


if(argc<2)
	{
				printf("use %s -h or --help for help\n",argv[0]);
				exit(0);
	}

 while (1) {
        this_option_optind = optind ? optind : 1;
        option_index = 0;
   
        op = getopt_long  (argc, argv, "hp:",
                 long_options, &option_index);
        if (op == -1)
            break;

        switch (op)
        {
          case 'p':
              pkts=atoi(optarg);
            break;
          case 'h':
               printf("-p or --pkts      Number of pkts to show [default all]\n");
               printf("-h or --help      this text\n");
               exit(0);
            break;

        default:
            printf ("?? getopt returned character code 0%o ??\n", op);
        }
    }

  l=strlen(argv[argc-1]);
  filename=(char*)malloc(l+1);
  filename=argv[argc-1];

 if(!open_cap_file(&infile,filename, &head,&comments)) {
    exit(0);
  }
//output fileheader
//  printf("Comment size: %d, ver: %d.%d id: %s \n Comment: %s\n",head.comment_size, head.version.major, head.version.minor, head.mpid, comments);
//allocate buffer memory
  cout << "Capture version: " << head.version.major << "." << head.version.minor << endl;
  cout << "Measurement Point: " << head.mpid << endl;
  cout << "Comment:\n" << comments << endl;
  cout << "---------------------------------------" << endl;
  size=alloc_buffer(&infile, &data);

//Begin Packet processing
  read_filter_post(&infile, data, size, myfilter);
  caphead=(cap_head*)data;
  pkt1=(qd_real)(double)caphead->ts.tv_sec+(qd_real)(double)(caphead->ts.tv_psec/PICODIVIDER);
  pkt2=(qd_real)(double)caphead->ts.tv_sec+(qd_real)(double)(caphead->ts.tv_psec/PICODIVIDER);
  timeOffset=floor(pkt1);
  pkt1-=timeOffset;
  minDiffTime=10e6;
  maxDiffTime=0;

  double readPkts=2;
  read_filter_post(&infile, data, size, myfilter);
  while (feof(infile)==0 )
  {
    caphead=(cap_head*)data;
    pkt2=(qd_real)(double)caphead->ts.tv_sec+(qd_real)(double)(caphead->ts.tv_psec/PICODIVIDER);
    pkt2-=timeOffset;
    diffTime=pkt2-pkt1;
    if(diffTime<-1e-7) {
      cout << "Sanity problem; pkt2 arrived prior to pkt1 " << endl;
    }
    cout << setprecision(12) << (double)diffTime << endl;
    pkt1=pkt2;

    if(pkts>0 && (readPkts+1)>pkts) {
      /* Read enough pkts lets break. */
      break;
    }
    read_filter_post(&infile, data, size, myfilter);
    readPkts++;
  }

//End Packet processing



  dealloc_buffer(&data);
  close_cap_file(&infile);
 

  return 0;
}
