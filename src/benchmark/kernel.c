/*
 * Copyright(C) 2015 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
	* the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <omp.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <common.h>
#include <mylib/util.h>

#include "benchmark.h"

/**
 * @brief Cache line size.
 */
#define CACHE_LINE_SIZE 64

/**
 * @brief Kernel load.
 */
#define KERNEL_LOAD 1000000

/* Workloads. */
unsigned *__tasks; /* Tasks.           */
unsigned __ntasks; /* Number of tasks. */

/**
 * @brief Dummy variable.
 */
static int *foobar;

/**
 * @brief Benchmark kernel.
 */
void kernel(int tid, int n)
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < KERNEL_LOAD; j++)
			foobar[tid*CACHE_LINE_SIZE]++;		
	}
}

/**
 * @brief Synthetic benchmark. 
 */
void benchmark(const unsigned *tasks, unsigned ntasks, unsigned niterations, unsigned nthreads, unsigned scheduler)
{
	uint64_t end;
	uint64_t start;
	
	foobar = smemalign(64, nthreads*CACHE_LINE_SIZE*sizeof(int));
	
	start = get_time();
	
	for (unsigned k = 0; k < niterations; k++)
	{	
		/* Dynamic scheduler. */
		if (scheduler == SCHEDULER_DYNAMIC)
		{
			#pragma omp parallel for schedule(dynamic)
			for (unsigned i = 0; i < ntasks; i++)
				kernel(omp_get_thread_num(), tasks[i]);
		}
		
		/* Smart round-robin scheduler. */
		else if (scheduler == SCHEDULER_SMART_ROUND_ROBIN)
		{		
			__ntasks = ntasks;
			__tasks = smalloc(ntasks*sizeof(unsigned));
			memcpy(__tasks, tasks, ntasks*sizeof(unsigned));
			
			#pragma omp parallel for schedule(runtime)
			for (unsigned i = 0; i < ntasks; i++)
				kernel(omp_get_thread_num(), tasks[i]);
			
			free(__tasks);
		}
		
		/* Static scheduler. */
		else
		{
			#pragma omp parallel for schedule(static)
			for (unsigned i = 0; i < ntasks; i++)
				kernel(omp_get_thread_num(), tasks[i]);
		}
	}
	
	end = get_time();
	
	printf("%.2lf\n", (end - start)/1000.0);
	
	/* House keeping. */
	free(foobar);
}
