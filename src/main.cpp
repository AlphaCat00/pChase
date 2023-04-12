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
#include <cstring>
#include <vector>

// Local includes
#include "assert.h"
#include "experiment.h"
#include "output.h"
#include "run.h"
#include "timer.h"
#include "types.h"

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
const char* config_file = "pchase.conf";
int multi_exp() {
    Timer::calibrate(10000);
    double clk_res = Timer::resolution();

    static char buf[1024];
    FILE* f = fopen(config_file, "r");
    if (f == nullptr) {
        fprintf(stderr, "fail to open the %s\n", config_file);
        return -1;
    }
    uint32_t num_exps;
    fscanf(f, "%u\n", &num_exps);
    std::vector<Experiment> e(num_exps);
    SpinBarrier sb(num_exps);
    Run r[num_exps];

    std::vector<char*> argv;
    for (uint32_t i = 0; i < num_exps; ++i) {
        argv.clear();
        fgets(buf, 1024, f);
        uint32_t len = strlen(buf);
        if (buf[len - 1] == '\n') buf[len - 1] = 0;
        for (char* p = strtok(buf, " "); p != nullptr; p = strtok(nullptr, " ")) {
            argv.push_back(p);
        }
        if (e[i].parse_args(argv.size(), &argv[0]))
            return -1;
        assert(e[i].num_threads == 1);
        int id;
        sscanf(argv[0], "%d", &id);
        r[i].set(e[i], &sb, id);
        r[i].start();
    }
    fclose(f);

    for (uint32_t i = 0; i < num_exps; i++) {
        r[i].wait();
    }

    for (uint32_t i = 0; i < num_exps; i++)
        Output::print(e[i], clk_res);
    // Output::print(e, ops, seconds, clk_res);

    return 0;
}

int main(int argc, char* argv[]) {
#ifdef DEBUG
    fprintf(stderr, "DEBUG (low performance)\n");
#else
    fprintf(stderr, "NDEBUG\n");
#endif
    if (argc == 1) {
        return multi_exp();
    }
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

    Output::print(e, clk_res);

    return 0;
}


