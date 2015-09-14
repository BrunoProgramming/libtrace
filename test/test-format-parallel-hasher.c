/*
 * This file is part of libtrace
 *
 * Copyright (c) 2007 The University of Waikato, Hamilton, New Zealand.
 * Authors: Daniel Lawson 
 *          Perry Lorier 
 *          
 * All rights reserved.
 *
 * This code has been developed by the University of Waikato WAND 
 * research group. For further information please see http://www.wand.net.nz/
 *
 * libtrace is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libtrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libtrace; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: test-rtclient.c,v 1.2 2006/02/27 03:41:12 perry Exp $
 *
 */
#ifndef WIN32
#  include <sys/time.h>
#  include <netinet/in.h>
#  include <netinet/in_systm.h>
#  include <netinet/tcp.h>
#  include <netinet/ip.h>
#  include <netinet/ip_icmp.h>
#  include <arpa/inet.h>
#  include <sys/socket.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "dagformat.h"
#include "libtrace_parallel.h"
#include "data-struct/vector.h"

void iferr(libtrace_t *trace,const char *msg)
{
	libtrace_err_t err = trace_get_err(trace);
	if (err.err_num==0)
		return;
	printf("Error: %s: %s\n", msg, err.problem);
	exit(1);
}

const char *lookup_uri(const char *type) {
	if (strchr(type,':'))
		return type;
	if (!strcmp(type,"erf"))
		return "erf:traces/100_packets.erf";
	if (!strcmp(type,"rawerf"))
		return "rawerf:traces/100_packets.erf";
	if (!strcmp(type,"pcap"))
		return "pcap:traces/100_packets.pcap";
	if (!strcmp(type,"wtf"))
		return "wtf:traces/wed.wtf";
	if (!strcmp(type,"rtclient"))
		return "rtclient:chasm";
	if (!strcmp(type,"pcapfile"))
		return "pcapfile:traces/100_packets.pcap";
	if (!strcmp(type,"pcapfilens"))
		return "pcapfile:traces/100_packetsns.pcap";
	if (!strcmp(type, "duck"))
		return "duck:traces/100_packets.duck";
	if (!strcmp(type, "legacyatm"))
		return "legacyatm:traces/legacyatm.gz";
	if (!strcmp(type, "legacypos"))
		return "legacypos:traces/legacypos.gz";
	if (!strcmp(type, "legacyeth"))
		return "legacyeth:traces/legacyeth.gz";
	if (!strcmp(type, "tsh"))
		return "tsh:traces/10_packets.tsh.gz";
	return type;
}

struct TLS {
	bool seen_start_message;
	bool seen_stop_message;
	bool seen_resumed_message;
	bool seen_pausing_message;
	int count;
};

static int totalpkts = 0;
static int expected;
static void report_result(libtrace_t *trace UNUSED, int mesg,
                          libtrace_generic_t data,
                          libtrace_thread_t *sender UNUSED) {
	static int totalthreads = 0;
	switch (mesg) {
	case MESSAGE_RESULT:
		assert(data.res->key == 0);
		printf("%d,", data.res->value.sint);
		totalthreads++;
		totalpkts += data.res->value.sint;
		assert(data.res->value.sint == 25 ||
		       data.res->value.sint == expected - 25);
		break;
	case MESSAGE_STARTING:
		// Should have two threads here
		assert(libtrace_get_perpkt_count(trace) == 2);
		printf("\tLooks like %d threads are being used!\n\tcounts(", libtrace_get_perpkt_count(trace));
		break;
	case MESSAGE_STOPPING:
		printf(")\n");
		assert(totalthreads == libtrace_get_perpkt_count(trace));
		break;
	}
}

static int x;
static void* per_packet(libtrace_t *trace, libtrace_thread_t *t,
                        int mesg, libtrace_generic_t data,
                        libtrace_thread_t *sender UNUSED) {
	struct TLS *tls;
	void* ret;
	tls = trace_get_tls(t);
	int a,*b,c=0;

	switch (mesg) {
	case MESSAGE_PACKET:
		assert(tls != NULL);
		assert(!(tls->seen_stop_message));
		tls->count++;
		if (tls->count>100) {
			fprintf(stderr, "Too many packets someone should stop me!!\n");
			kill(getpid(), SIGTERM);
		}
		// Do some work to even out the load on cores
		b = &c;
		for (a = 0; a < 10000000; a++) {
			c += a**b;
		}
		x = c;
		return data.pkt;
	case MESSAGE_STARTING:
		assert(tls == NULL);
		tls = calloc(sizeof(struct TLS), 1);
		ret = trace_set_tls(t, tls);
		assert(ret == NULL);
		tls->seen_start_message = true;
		break;
	case MESSAGE_STOPPING:
		assert(tls->seen_start_message);
		assert(tls != NULL);
		tls->seen_stop_message = true;
		trace_set_tls(t, NULL);

		// All threads publish to verify the thread count
		assert(tls->count == 25 || tls->count == 75);
		trace_publish_result(trace, t, (uint64_t) 0, (libtrace_generic_t){.sint=tls->count}, RESULT_USER);
		trace_post_reporter(trace);
		free(tls);
		break;
	case MESSAGE_TICK_INTERVAL:
	case MESSAGE_TICK_COUNT:
		assert(tls->seen_start_message );
		fprintf(stderr, "Not expecting a tick packet\n");
		kill(getpid(), SIGTERM);
		break;
	case MESSAGE_PAUSING:
		assert(tls->seen_start_message);
		tls->seen_pausing_message = true;
		break;
	case MESSAGE_RESUMING:
		assert(tls->seen_pausing_message  || tls->seen_start_message);
		tls->seen_resumed_message = true;
		break;
	}
	return NULL;
}


/**
 * Sends the first 25 packets to thread 0, the next 75 to thread 1
 * This is based on a few internal workings assumptions, which
 * might change and still be valid even if this test fails!!.
 */
uint64_t hash25_75(const libtrace_packet_t* packet UNUSED, void *data) {
	int *count = (int *) data;
	*count += 1;
	if (*count <= 25)
		return 0;
	return 1;
}

/**
 * Test that the hasher function works
 */
int test_hasher(const char *tracename) {
	libtrace_t *trace;
	int error = 0;
	int hashercount = 0;
	printf("Testing hasher function\n");

	// Create the trace
	trace = trace_create(tracename);
	iferr(trace,tracename);

	// Always use 2 threads for simplicity
	trace_set_perpkt_threads(trace, 2);
	trace_set_hasher(trace, HASHER_CUSTOM, &hash25_75, &hashercount);

	// Start it
	trace_pstart(trace, NULL, per_packet, report_result);
	iferr(trace,tracename);
	/* Make sure traces survive a pause and restart */
	trace_ppause(trace);
	iferr(trace,tracename);
	trace_pstart(trace, NULL, NULL, NULL);
	iferr(trace,tracename);

	/* Wait for all threads to stop */
	trace_join(trace);

	/* Now check we have all received all the packets */
	if (error == 0) {
		if (totalpkts == expected) {
			printf("success: %d packets read\n",expected);
		} else {
			printf("failure: %d packets expected, %d seen\n",expected,totalpkts);
			error = 1;
		}
	} else {
		iferr(trace,tracename);
	}
    trace_destroy(trace);
    return error;
}



int main(int argc, char *argv[]) {
	int error = 0;
	const char *tracename;
	expected = 100;

	if (argc<2) {
		fprintf(stderr,"usage: %s type\n",argv[0]);
		return 1;
	}

	tracename = lookup_uri(argv[1]);

	if (strcmp(argv[1],"rtclient")==0) expected=101;

	error = test_hasher(tracename);

    return error;
}