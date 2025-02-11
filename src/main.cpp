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

// System includes
#include <cstdio>

// Local includes
#include "run.h"
#include "timer.h"
#include "types.h"
#include "output.h"
#include "experiment.h"

// This program allocates and accesses
// a number of blocks of memory, one or more
// for each thread that executes.  Blocks
// are divided into sub-blocks called
// pages, and pages are divided into
// sub-blocks called cache lines.
//
// All pages are collected into a list.
// Pages are selected for the list in
// a particular order.   Each cache line
// within the page is similarly gathered
// into a list in a particular order.
// In both cases the order may be random
// or linear.
//
// A root pointer points to the first
// cache line.  A pointer in the cache
// line points to the next cache line,
// which contains a pointer to the cache
// line after that, and so on.  This
// forms a pointer chain that touches all
// cache lines within the first page,
// then all cache lines within the second
// page, and so on until all pages are
// covered.  The last pointer contains
// NULL, terminating the chain.
//
// Depending on compile-time options,
// pointers may be 32-bit or 64-bit
// pointers.


//
// Implementation
//

int verbose = 0;

int main(int argc, char* argv[]) {
#ifdef DEBUG
	fprintf(stderr, "DEBUG (low performance)\n");
#else
    fprintf(stderr, "NDEBUG\n");
#endif
	Timer::calibrate(10000);
	double clk_res = Timer::resolution();

	Experiment e;
	if (e.parse_args(argc, argv)) {
		return 0;
	}

	SpinBarrier sb(e.num_threads);
	Run r[e.num_threads];
	for (int i = 0; i < e.num_threads; i++) {
		r[i].set(e, &sb);
		r[i].start();
	}

	for (int i = 0; i < e.num_threads; i++) {
		r[i].wait();
	}

	int64 ops = Run::ops_per_chain();
	std::vector<double> seconds = Run::seconds();

	Output::print(e, ops, seconds, clk_res);

	return 0;
}
