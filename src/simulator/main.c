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

#include <float.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <mylib/util.h>

#include "simulator.h"

#define _SORT_

/**
 * @brief Threads.
 */
struct thread *threads = NULL;

/**
 * @name Simulation Parameters
 */
/**@{*/
static unsigned nthreads = 32;              /**< Number of threads.        */
static unsigned ntasks = 1024;              /**< Number of tasks.          */
static unsigned distribution = 0;           /**< Probability distribution. */
static unsigned scheduler = SCHEDULER_NONE; /**< Loop scheduler.           */
static unsigned niterations = 1;            /**< Number of iterations.     */
/**@}*/

/**
 * @brief Number of supported probability distributions.
 */
#define NDISTRIBUTIONS 3

/**
 * @name Probability Distributions.
 */
/**@{*/
#define DISTRIBUTION_RANDOM  0 /**< Random distribution.  */
#define DISTRIBUTION_NORMAL  1 /**< Normal distribution.  */
#define DISTRIBUTION_POISSON 2 /**< Poisson distribution. */
/**@}*/
	

/**
 * @brief Supported probability distributions.
 */
static char *distributions[NDISTRIBUTIONS] = {
	"random", /* Random.  */
	"normal", /* Normal.  */
	"poisson" /* Poisson. */
};

/**
 * @brief Prints program usage and exits.
 * 
 * @details Prints program usage and exits gracefully.
 */
static void usage(void)
{
	printf("Usage: 	scheduler [options] <scheduler>\n");
	printf("Brief: loop scheduler simulator\n");
	printf("Scheduler:\n");
	printf("  static         Simulate static loop scheduling\n");
	printf("  dynamic        Simulate dynamic loop scheduling\n");
	printf("  workload-aware Simulate workload-aware loop scheduling\n");
	printf("Options:\n");
	printf("  --iterations            Number of iterations\n");
	printf("  --nthreads <num>        Number of threads\n");
	printf("  --ntasks <num>          Number of tasks\n");
	printf("  --distribution <name>   Probability density function\n");
	printf("  --help                  Display this message\n");

	exit(EXIT_SUCCESS);
}

/**
 * @brief Reads command line arguments.
 * 
 * @details Reads command line arguments.
 */
static void readargs(int argc, const char **argv)
{
	enum states{
		STATE_READ_ARG,          /* Read argument.            */
		STATE_SET_NTHREADS,      /* Set number of threads.    */
		STATE_SET_NTASKS,        /* Set number of tasks.      */
		STATE_SET_NITERATIONS,   /* Set number of iterations. */
		STATE_SET_DISTRIBUTION}; /* Set distribution.         */
	
	unsigned state;                /* Current state.     */
	const char *distribution_name; /* Distribution name. */
	
	state = STATE_READ_ARG;
	distribution_name = NULL;
	
	/* Parse command line arguments. */
	for (int i = 1; i < argc; i++)
	{
		const char *arg = argv[i];
		
		/* Set value. */
		if (state != STATE_READ_ARG)
		{
			switch (state)
			{
				case STATE_SET_NTHREADS:
					nthreads = atoi(arg);
					state = STATE_READ_ARG;
					break;
				
				case STATE_SET_NTASKS:
					ntasks = atoi(arg);
					state = STATE_READ_ARG;
					break;
				
				case STATE_SET_DISTRIBUTION:
					distribution_name = arg;
					state = STATE_READ_ARG;
					break;

				case STATE_SET_NITERATIONS:
					niterations = atoi(arg);
					state = STATE_READ_ARG;
					break;
			}
			
			continue;
		}
		
		/* Parse command. */
		if (!strcmp(arg, "--nthreads"))
			state = STATE_SET_NTHREADS;
		else if (!strcmp(arg, "--niterations"))
			state = STATE_SET_NITERATIONS;
		else if (!strcmp(arg, "--ntasks"))
			state = STATE_SET_NTASKS;
		else if (!strcmp(arg, "--distribution"))
			state = STATE_SET_DISTRIBUTION;
		else if (!strcmp(arg, "--help"))
			usage();
		else if (!strcmp(arg, "static"))
			scheduler = SCHEDULER_STATIC;
		else if (!strcmp(arg, "dynamic"))
			scheduler = SCHEDULER_DYNAMIC;
		else if (!strcmp(arg, "workload-aware"))
			scheduler = SCHEDULER_WORKLOAD_AWARE;
	}
	
	/* Check parameters. */
	if (nthreads == 0)
		error("invalid number of threads");
	else if (niterations == 0)
		error("invalid number of iterations");
	else if (ntasks == 0)
		error("invalid number of tasks");
	else if (scheduler == SCHEDULER_NONE)
		error("invalid scheduler");
	if (distribution_name != NULL)
	{
		for (unsigned i = 0; i < NDISTRIBUTIONS; i++)
		{
			if (!strcmp(distribution_name, distributions[i]))
			{
				distribution = i;
				goto out;
			}
		}
		error("unknown distribution");
	}

out:
	return;
}

#ifdef _SORT_

/**
 * @brief Compares two unsigned integers.
 * 
 * @details Compares the unsigned integer pointed to by @p a with the unsigned
 *          integer pointed to by @p b.
 * 
 * @returns The difference between @p a and @p b.
 */
static int cmp(const void *a, const void *b)
{
	return ((*(double *)a) - (*(double *)b));
}

#endif

/**
 * @brief Generates tasks.
 */
static double *create_tasks(unsigned distribution, unsigned ntasks)
{
	double *tasks;
	
	/* Create tasks. */
	tasks = smalloc(ntasks*sizeof(double));
	switch (distribution)
	{
		/* Random distribution. */
		case DISTRIBUTION_RANDOM:
		{
			for (unsigned i = 0; i < ntasks; i++)
				tasks[i] = randnum()%ntasks;
		} break;
		
		/* Normal distribution. */
		case DISTRIBUTION_NORMAL:
		{
			for (unsigned i = 0; i < ntasks; i++)
			{
				double num;
				
				do
				{
					num = normalnum(32.0, 1.0);
				} while (num < 0.0);
				
				tasks[i] = num;
			}
		} break;
		
		/* Poisson Distribution. */
		case DISTRIBUTION_POISSON:
		{
			for (unsigned i = 0; i < ntasks; i++)
			{
				double num;
				
				do
				{
					num = poissonnum(8.0);
				} while (num < 0.0);
				
				tasks[i] = num;
			}
		} break;
	}
	
	return (tasks);
}

/**
 * @brief Spawn threads.
 */
static void threads_spawn(void)
{
	/* Create threads. */
	threads = smalloc(nthreads*sizeof(struct thread));
	for (unsigned i = 0; i < nthreads; i++)
	{
		threads[i].tid = i;
		threads[i].workload = 0;
		threads[i].ntasks = 0;
		threads[i].avg = 0;
		threads[i].max = 0;
		threads[i].min = DBL_MAX;
	}
}

/**
 * @brief Joins threads.
 */
static void threads_join(void)
{
	free(threads);
}

/**
 * @brief Loop scheduler simulator.
 */
int main(int argc, const const char **argv)
{
	double m;
	double *tasks;
	
	readargs(argc, argv);
	
	threads_spawn();

	for (unsigned i = 0; i < niterations; i++)
	{
		tasks = create_tasks(distribution, ntasks);
		
#ifdef _SORT_	
		qsort(tasks, ntasks, sizeof(double), cmp);
#endif
		schedule(tasks, ntasks, nthreads, scheduler);
		
		/* Print statistics. */
		if (niterations == 1)
		{
			for (unsigned i = 0; i < ntasks; i++)
				fprintf(stderr, "%lf\n", tasks[i]);
		}
		
		/* House keeping. */
		free(tasks);
	}
	
	/* Compute total workload. */
	m = 0.0;
	for (unsigned i = 0; i < nthreads; i++)
		m += threads[i].workload;
	m = 100/m;
	
	/* Print statistics. */
	for (unsigned i = 0; i < nthreads; i++)
		printf("%u;%lf\n", threads[i].tid, threads[i].workload*m);
	
	threads_join();
	
	return (EXIT_SUCCESS);
}
