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
#include "run.h"

// System includes
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstddef>
#include <algorithm>
#if defined(NUMA)
#include <numa.h>
#endif

// Local includes
#include <asmjit/core.h>
#include <asmjit/a64.h>
#include "timer.h"


//
// Implementation
//

typedef void (*benchmark)(Chain**);
static benchmark chase_pointers(asmjit::JitRuntime &rt,	Experiment &exp);

Lock Run::global_mutex;
int64 Run::_ops_per_chain = 0;
std::vector<double> Run::_seconds;

Run::Run() :
		exp(NULL), bp(NULL) {
}

Run::~Run() {
}

void Run::set(Experiment &e, SpinBarrier* sbp) {
	this->exp = &e;
	this->bp = sbp;
}

int Run::run() {
	// first allocate all memory for the chains,
	// making sure it is allocated within the
	// intended numa domains
	Chain** chain_memory = new Chain*[this->exp->chains_per_thread];
	Chain** root = new Chain*[this->exp->chains_per_thread];

#if defined(NUMA)
	// establish the node id where this thread
	// will run. threads are mapped to nodes
	// by the set-up code for Experiment.
	int run_node_id = this->exp->thread_domain[this->thread_id()];
	numa_run_on_node(run_node_id);

	// establish the node id where this thread's
	// memory will be allocated.
	for (int i=0; i < this->exp->chains_per_thread; i++) {
		int alloc_node_id = this->exp->chain_domain[this->thread_id()][i];
        bitmask* alloc_mask = numa_allocate_nodemask();
   		numa_bitmask_setbit(alloc_mask, alloc_node_id);
    	numa_set_membind(alloc_mask);
    	numa_bitmask_free(alloc_mask);

		chain_memory[i] = new Chain[ this->exp->links_per_chain ];
	}
#else
	for (int i = 0; i < this->exp->chains_per_thread; i++) {
		chain_memory[i] = new Chain[this->exp->links_per_chain];
	}
#endif

	// initialize the chains and
	// select the function that
	// will generate the tests
	for (int i = 0; i < this->exp->chains_per_thread; i++) {
		if (this->exp->access_pattern == Experiment::RANDOM) {
			root[i] = random_mem_init(chain_memory[i]);
		} else if (this->exp->access_pattern == Experiment::STRIDED) {
			if (0 < this->exp->stride) {
				root[i] = forward_mem_init(chain_memory[i]);
			} else {
				root[i] = reverse_mem_init(chain_memory[i]);
			}
		}
	}

	// compile benchmark
	asmjit::JitRuntime rt;
	benchmark bench = chase_pointers(rt, *this->exp);

	// calculate the number of iterations
	/*
	 * As soon as the thread count rises, this calculation HUGELY
	 * differs between runs. What does cause this? Threads are already
	 * limited to certain CPUs, so it's not caused by excessive switching.
	 * Strange cache behaviour?
	 */
	if (0 == this->exp->iterations) {
		volatile static double istart = 0;
		volatile static double istop = 0;
		volatile static double elapsed = 0;
		volatile static int64 iters = 1;
		volatile double bound = std::max(0.2, 10 * Timer::resolution());
		for (iters = 1; elapsed <= bound; iters = iters << 1) {
			// barrier
			this->bp->barrier();

			// start timer
			if (this->thread_id() == 0) {
				istart = Timer::seconds();
			}
			this->bp->barrier();

			// chase pointers
			for (int i = 0; i < iters; i++)
				bench(root);

			// barrier
			this->bp->barrier();

			// stop timer
			if (this->thread_id() == 0) {
				istop = Timer::seconds();
				elapsed = istop - istart;
			}
			this->bp->barrier();
		}

		// calculate the number of iterations
		if (this->thread_id() == 0) {
			if (0 < this->exp->seconds) {
				this->exp->iterations = std::max(1.0,
						0.9999 + 0.5 * this->exp->seconds * iters / elapsed);
			} else {
				this->exp->iterations = std::max(1.0, 0.9999 + iters / elapsed);
			}
			//printf("Tested %d iterations: took %f seconds; scheduling %d iterations\n", iters, elapsed, this->exp->iterations);
		}
		this->bp->barrier();
	}

	// run the experiments
	for (int e = 0; e < this->exp->experiments; e++) {
		// barrier
		this->bp->barrier();

		// start timer
		double start = 0;
		if (this->thread_id() == 0)
			start = Timer::seconds();
		this->bp->barrier();

		// chase pointers
		for (int i = 0; i < this->exp->iterations; i++)
			bench(root);

		// barrier
		this->bp->barrier();

		// stop timer
		double stop = 0;
		if (this->thread_id() == 0)
			stop = Timer::seconds();
		this->bp->barrier();

		if (0 <= e) {
			if (this->thread_id() == 0) {
				double delta = stop - start;
				if (0 < delta) {
					Run::_seconds.push_back(delta);
				}
			}
		}
	}

	this->bp->barrier();

	// clean the memory
	for (int i = 0; i < this->exp->chains_per_thread; i++) {
		if (chain_memory[i] != NULL
			) delete[] chain_memory[i];
	}
	if (chain_memory != NULL
		) delete[] chain_memory;

	return 0;
}

int dummy = 0;
void Run::mem_check(Chain *m) {
	if (m == NULL
		) dummy += 1;
}

// exclude 2 and Mersenne primes, i.e.,
// primes of the form 2**n - 1, e.g.,
// 3, 7, 31, 127
static const int prime_table[] = { 5, 11, 13, 17, 19, 23, 37, 41, 43, 47, 53,
		61, 71, 73, 79, 83, 89, 97, 101, 103, 109, 113, 131, 137, 139, 149, 151,
		157, 163, };
static const int prime_table_size = sizeof prime_table / sizeof prime_table[0];

Chain*
Run::random_mem_init(Chain *mem) {
	// initialize pointers --
	// choose a page at random, then use
	// one pointer from each cache line
	// within the page.  all pages and
	// cache lines are chosen at random.
	Chain* root = 0;
	Chain* prev = 0;
	int link_within_line = 0;
	int64 local_ops_per_chain = 0;

	// we must set a lock because random()
	// is not thread safe
	Run::global_mutex.lock();
	setstate(this->exp->random_state[this->thread_id()]);
	int page_factor = prime_table[random() % prime_table_size];
	int page_offset = random() % this->exp->pages_per_chain;
	Run::global_mutex.unlock();

	// loop through the pages
	for (int i = 0; i < this->exp->pages_per_chain; i++) {
		int page = (page_factor * i + page_offset) % this->exp->pages_per_chain;
		Run::global_mutex.lock();
		setstate(this->exp->random_state[this->thread_id()]);
		int line_factor = prime_table[random() % prime_table_size];
		int line_offset = random() % this->exp->lines_per_page;
		Run::global_mutex.unlock();

		// loop through the lines within a page
		for (int j = 0; j < this->exp->lines_per_page; j++) {
			int line_within_page = (line_factor * j + line_offset)
					% this->exp->lines_per_page;
			int link = page * this->exp->links_per_page
					+ line_within_page * this->exp->links_per_line
					+ link_within_line;

			if (root == 0) {
				prev = root = mem + link;
				local_ops_per_chain += 1;
			} else {
				prev->next = mem + link;
				prev = prev->next;
				local_ops_per_chain += 1;
			}
		}
	}

	prev->next = root;

	Run::global_mutex.lock();
	Run::_ops_per_chain = local_ops_per_chain;
	Run::global_mutex.unlock();

	return root;
}

Chain*
Run::forward_mem_init(Chain *mem) {
	Chain* root = 0;
	Chain* prev = 0;
	int link_within_line = 0;
	int64 local_ops_per_chain = 0;

	for (int i = 0; i < this->exp->lines_per_chain; i += this->exp->stride) {
		int link = i * this->exp->links_per_line + link_within_line;
		if (root == NULL) {
			prev = root = mem + link;
			local_ops_per_chain += 1;
		} else {
			prev->next = mem + link;
			prev = prev->next;
			local_ops_per_chain += 1;
		}
	}

	prev->next = root;

	Run::global_mutex.lock();
	Run::_ops_per_chain = local_ops_per_chain;
	Run::global_mutex.unlock();

	return root;
}

Chain*
Run::reverse_mem_init(Chain *mem) {
	Chain* root = 0;
	Chain* prev = 0;
	int link_within_line = 0;
	int64 local_ops_per_chain = 0;

	int stride = -this->exp->stride;
	int last;
	for (int i = 0; i < this->exp->lines_per_chain; i += stride) {
		last = i;
	}

	for (int i = last; 0 <= i; i -= stride) {
		int link = i * this->exp->links_per_line + link_within_line;
		if (root == 0) {
			prev = root = mem + link;
			local_ops_per_chain += 1;
		} else {
			prev->next = mem + link;
			prev = prev->next;
			local_ops_per_chain += 1;
		}
	}

	prev->next = root;

	Run::global_mutex.lock();
	Run::_ops_per_chain = local_ops_per_chain;
	Run::global_mutex.unlock();

	return root;
}

static benchmark chase_pointers(asmjit::JitRuntime &rt,	Experiment &exp) {
	using namespace asmjit;
	using namespace a64;
	// Create Compiler.
	CodeHolder code;                  // Holds code and relocation information.
  	code.init(rt.environment());      // Initialize code to match the JIT environment. 

	Compiler c(&code);	// Create and attach Compiler to code.

  	// Tell compiler the function prototype we want. It allocates variables representing
	// function arguments that can be accessed through Compiler or Function instance.
	FuncNode* funcNode = c.addFunc(FuncSignatureT<void, Chain**>());

	// Try to generate function without prolog/epilog code:
	// c.getFunction()->setHint(asmjit::FUNCTION_HINT_NAKED, true);

	// Create labels.
	Label L_Loop = c.newLabel();

	// Function arguments.
	Gp chain=c.newUIntPtr();
	funcNode->setArg(0,chain);


	// Save the head
	Gp head = c.newUIntPtr();
	c.ldr(head, ptr(chain));

	// Current position
	std::vector<Gp> positions(exp.chains_per_thread);
	for (int i = 0; i < exp.chains_per_thread; i++) {
		Gp position = c.newUIntPtr();
		c.ldr(position,ptr(chain,i*sizeof(Chain *)));
		positions[i] = position;
	}

	Gp val = c.newUInt64();
	c.ldr(val,100);
	// Loop.
	c.bind(L_Loop);

	// Process all links
	for (int i = 0; i < exp.chains_per_thread; i++) {
		// Chase pointer
		c.ldr(positions[i], ptr(positions[i], offsetof(Chain, next)));
		if(exp.mem_operation==Experiment::LOAD){
			c.ldr(val, ptr(positions[i], offsetof(Chain, data)));
		}else if(exp.mem_operation==Experiment::STORE){
			c.str(val, ptr(positions[i], offsetof(Chain, data)));
		}		

		// Prefetch next
		// switch (prefetch_hint)
		// {
		// case Experiment::T0:
		// 	c.prefetch(ptr(positions[i]), asmjit::PREFETCH_T0);
		// 	break;
		// case Experiment::T1:
		// 	c.prefetch(ptr(positions[i]), asmjit::PREFETCH_T1);
		// 	break;
		// case Experiment::T2:
		// 	c.prefetch(ptr(positions[i]), asmjit::PREFETCH_T2);
		// 	break;
		// case Experiment::NTA:
		// 	c.prefetch(ptr(positions[i]), asmjit::PREFETCH_NTA);
		// 	break;
		// case Experiment::NONE:
		// default:
		// 	break;
		// }
	}

	// Wait
	for (int i = 0; i < exp.loop_length; i++)
		c.nop();

	// Test if end reached
	c.cmp(head, positions[0]);
	c.b(CondCode::kNE,L_Loop);


	// Finish.
	c.endFunc();
	c.finalize();

	// Add the generated code to the runtime.
  	benchmark fn;
  	Error err = rt.add(&fn, &code);
	// Handle a possible error returned by AsmJit.
	if (err) {
		printf("Error making jit function (%u).\n", err);
		return 0;
	}

	return fn;
}
