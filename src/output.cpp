/*******************************************************************************
 * Copyright (c) 2006 International Business Machines Corporation.             *
 * All rights reserved. This program and the accompanying materials            *
 * are made available under the terms of the Common Public License v1.0        *
 * which accompanies this distribution, and is available at                    *
 * http://www.opensource.org/licenses/cpl1.0.php                               *
 *                                                                             *
 * Contributors:                                                               *
 *    Douglas M. Pase - initial API and implementation                         *
 *    Tim Besard - prefetching, JIT compilation                                *
 *******************************************************************************/

//
// Configuration
//

// Implementation header
#include "output.h"

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
// Implementation
//

void Output::print(Experiment &e, int64 ops, std::vector<double> seconds, double ck_res) {
	if (e.output_mode == Experiment::HEADER) {
		Output::header(e, ops, ck_res);
	} else if (e.output_mode == Experiment::CSV) {
		for (size_t i = 0; i < seconds.size(); i++)
			Output::csv(e, ops, seconds[i], ck_res);
	} else if (e.output_mode == Experiment::BOTH) {
		Output::header(e, ops, ck_res);
		for (size_t i = 0; i < seconds.size(); i++)
			Output::csv(e, ops, seconds[i], ck_res);
	} else {
		long double averaged_seconds = 0;
		for (size_t i = 0; i < seconds.size(); i++)
			averaged_seconds += seconds[i];
		Output::table(e, ops, (double) (averaged_seconds/seconds.size()), ck_res);
	}
}

void Output::header(Experiment &e, int64 ops, double ck_res) {
    printf("pointer size (bytes),");
    printf("cache line size (bytes),");
    printf("page size (bytes),");
    printf("chain size (bytes),");
    printf("thread size (bytes),");
    printf("test size (bytes),");
    printf("chains per thread,");
    printf("number of threads,");
    printf("iterations,");
    printf("loop length,");
    printf("prefetch hint,");
    printf("memory operation,");
    printf("experiments,");
    printf("access pattern,");
    printf("stride,");
    printf("numa placement,");
    printf("offset or mask,");
    printf("numa domains,");
    printf("domain map,");
    printf("operations per chain,");
    printf("total operations,");
    printf("elapsed time (seconds),");
    printf("elapsed time (timer ticks),");
    printf("clock resolution (ns),");
    printf("memory latency (ns),");
    printf("memory bandwidth (MB/s)\n");

    fflush(stdout);
}

void Output::csv(Experiment &e, int64 ops, double secs, double ck_res) {
    printf("%lld,", e.pointer_size);
    printf("%lld,", e.bytes_per_line);
    printf("%lld,", e.bytes_per_page);
    printf("%lld,", e.bytes_per_chain);
    printf("%lld,", e.bytes_per_thread);
    printf("%lld,", e.bytes_per_test);
    printf("%lld,", e.chains_per_thread);
    printf("%lld,", e.num_threads);
    printf("%lld,", e.iterations);
    printf("%lld,", e.loop_length);
    printf("%s,", prefetch_hint_string(e.prefetch_hint));
    printf("%s,", operation_string(e.mem_operation));
    printf("%lld,", e.experiments);
    printf("%s,", e.access());
    printf("%lld,", e.stride);
    printf("%s,", e.placement());
    printf("%lld,", e.offset_or_mask);
    printf("%d,", e.num_numa_domains);
    printf("\"");
    printf("%d:", e.thread_domain[0]);
    printf("%d", e.chain_domain[0][0]);
    for (int j = 1; j < e.chains_per_thread; j++) {
		printf(",%d", e.chain_domain[0][j]);
	}
	for (int i = 1; i < e.num_threads; i++) {
		printf(";%d:", e.thread_domain[i]);
		printf("%d", e.chain_domain[i][0]);
		for (int j = 1; j < e.chains_per_thread; j++) {
			printf(",%d", e.chain_domain[i][j]);
		}
	}
    printf("\",");
    printf("%lld,", ops);
    printf("%lld,", ops * e.chains_per_thread * e.num_threads);
    printf("%.3f,", secs);
    printf("%.0f,", secs/ck_res);
    printf("%.2f,", ck_res * 1E9);
    printf("%.2f,", (secs / (ops * e.iterations)) * 1E9);
    uint32_t line_num=e.mem_operation==Experiment::STORE_ALL||e.mem_operation==Experiment::LOAD_ALL?abs(e.stride):1;
    printf("%.3f\n", ((ops * e.iterations * e.chains_per_thread * e.num_threads * e.bytes_per_line * line_num) / secs) * 1E-6);

    fflush(stdout);
}

void Output::table(Experiment &e, int64 ops, double secs, double ck_res) {
    printf("pointer size         = %lld (bytes)\n", e.pointer_size);
    printf("cache line size      = %lld (bytes)\n", e.bytes_per_line);
    printf("page size            = %lld (bytes)\n", e.bytes_per_page);
    printf("chain size           = %lld (bytes)\n", e.bytes_per_chain);
    printf("thread size          = %lld (bytes)\n", e.bytes_per_thread);
    printf("test size            = %lld (bytes)\n", e.bytes_per_test);
    printf("chains per thread    = %lld\n", e.chains_per_thread);
    printf("number of threads    = %lld\n", e.num_threads);
    printf("iterations           = %lld\n", e.iterations);
    printf("loop length          = %lld\n", e.loop_length);
    printf("prefetch hint        = %s\n", prefetch_hint_string(e.prefetch_hint));
    printf("memory operation     = %s\n", operation_string(e.mem_operation));
    printf("experiments          = %lld\n", e.experiments);
    printf("access pattern       = %s\n", e.access());
    printf("stride               = %lld\n", e.stride);
    printf("numa placement       = %s\n", e.placement());
    printf("offset or mask       = %lld\n", e.offset_or_mask);
    printf("numa domains         = %d\n", e.num_numa_domains);
    printf("domain map           = ");
    printf("\"");
    printf("%d:", e.thread_domain[0]);
    printf("%d", e.chain_domain[0][0]);
    for (int j = 1; j < e.chains_per_thread; j++) {
		printf(",%d", e.chain_domain[0][j]);
	}
	for (int i = 1; i < e.num_threads; i++) {
		printf(";%d:", e.thread_domain[i]);
		printf("%d", e.chain_domain[i][0]);
		for (int j = 1; j < e.chains_per_thread; j++) {
			printf(",%d", e.chain_domain[i][j]);
		}
	}
    printf("\"\n");
    printf("operations per chain = %lld\n", ops);
    printf("total operations     = %lld\n", ops * e.chains_per_thread * e.num_threads);
    printf("elapsed time         = %.3f (seconds)\n", secs);
    printf("elapsed time         = %.0f (timer ticks)\n", secs/ck_res);
    printf("clock resolution     = %.2f (ns)\n", ck_res * 1E9);
    printf("memory latency       = %.2f (ns)\n", (secs / (ops * e.iterations)) * 1E9);
    uint32_t line_num=e.mem_operation==Experiment::STORE_ALL||e.mem_operation==Experiment::LOAD_ALL?abs(e.stride):1;
    printf("memory bandwidth     = %.3f (MB/s)\n", ((ops * e.iterations * e.chains_per_thread * e.num_threads * e.bytes_per_line * line_num) / secs) * 1E-6);

    fflush(stdout);
}
