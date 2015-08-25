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

#include <common.h>
#include <mylib/util.h>

#define N 100

extern bool use_floating_point;
extern unsigned chunksize;

/* Workloads. */
unsigned *__tasks; /* Tasks.           */
unsigned __ntasks; /* Number of tasks. */

const double barf[] = {
	 1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,
	 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0,
	17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
	25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0, 32.0,
	33.0, 34.0, 35.0, 36.0, 37.0, 38.0, 39.0, 40.0,
	41.0, 42.0, 43.0, 44.0, 45.0, 46.0, 47.0, 48.0,
	49.0, 50.0, 51.0, 52.0, 53.0, 54.0, 55.0, 56.0,
	57.0, 58.0, 59.0, 60.0, 61.0, 62.0, 63.0, 64.0
};

const unsigned bari[] = {
	 1,  2,  3,  4,  5,  6,  7,  8,
	 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24,
	25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, 52, 53, 54, 55, 56,
	57, 58, 59, 60, 61, 62, 63, 64
};

/**
 * @brief Performs some integer precision work.
 */
static double worki(double foo)
{
	unsigned bar;
	unsigned n2 = N*foo*N*foo;
	unsigned dummy = 0;
	
	for (unsigned i = 0; i < n2; i++)
	{
		for (unsigned k = 0; k < 64; k++)
		{
			__asm__("movl $1, %%eax\n"
			        "movl $2, %%ebx\n"
			        "movl $3, %%ebx\n"
			        "movl $4, %%edx\n"
			        "imul %%eax, %%ebx\n"
			        "imul %%eax, %%ebx\n"
			        "imul %%ebx, %%ecx\n"
			        "imul %%ecx, %%edx\n"
			        "imul %%eax, %%ecx\n"
			        "imul %%eax, %%ebx\n"
			        "imul %%ebx, %%ecx\n"
			        "imul %%ecx, %%edx\n"
			        "imul %%eax, %%ecx\n"
			        "imul %%ebx, %%eax\n"
			        "imul %%eax, %%ebx\n"
			        "imul %%eax, %%ebx\n"
			        "imul %%ebx, %%ecx\n"
			        "imul %%ecx, %%edx\n"
			        "imul %%eax, %%ecx\n"
			        "imul %%eax, %%ebx\n"
			        "imul %%ebx, %%ecx\n"
			        "imul %%ecx, %%edx\n"
			        "imul %%eax, %%ecx\n"
			        "imul %%eax, %0"
			         : "=r" (bar));
			dummy += bar;
		}
	}
	
	return (dummy);
}

/**
 * @brief Performs some floating-point precision work.
 */
static double workf(double foo)
{
	unsigned bar;
	unsigned n2 = N*foo*N*foo;
	double dummy = 0.0;
	
	for (unsigned i = 0; i < n2; i++)
	{
		for (unsigned k = 0; k < 64; k++)
		{
			bar = barf[k];
			dummy += (bar + 1)*(bar + 2)*(bar + 3)*(bar + 4);
		}
	}
	
	return (dummy);
}


/**
 * @brief Simulates a synthetic numeric kernel.
 */
void kernel
(const double *tasks, unsigned ntasks, unsigned nthreads, unsigned scheduler)
{
	double total = 0.0;
	double (*work)(double);
	
	work = (use_floating_point) ? workf : worki;
	
	(void) nthreads;

	/* Dynamic scheduler. */
	if (scheduler == SCHEDULER_STATIC)
	{
		#pragma omp parallel for schedule(dynamic, chunksize) private(total)
		for (unsigned i = 0; i < ntasks; i++)
			total += work(tasks[i]);
	}
	
	/* Smart round-robin scheduler. */
	else if (scheduler == SCHEDULER_SMART_ROUND_ROBIN)
	{
		__ntasks = ntasks;
		__tasks = smalloc(ntasks*sizeof(unsigned));
		for (unsigned i = 0; i < ntasks; i++)
			__tasks[i] = (unsigned)(tasks[i]*1000.0);
		
		#pragma omp parallel for schedule(runtime) private(total)
		for (unsigned i = 0; i < ntasks; i++)
			total += work(tasks[i]);
		
		free(__tasks);
	}
	
	/* Static scheduler. */
	else
	{
		#pragma omp parallel for schedule(static, chunksize) private(total)
		for (unsigned i = 0; i < ntasks; i++)
			total += work(tasks[i]);
	}
}
