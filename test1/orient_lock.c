/*
 * orient_lock.c
 *
 *  Created on: Oct 13, 2012
 *      Author: w4118
 */
#include "orient_lock.h"

/*
 *
 * System Calls
 *
__NR_orientlock_write
__NR_orientunlock_read
__NR_orientunlock_write
__NR_orientlock_read
 */

/* Keeps on attempting to acquire a Read lock until we succeed */
void orient_read_lock(int degree, int range)
{
	int ret = -1;
	do {
		ret = syscall(381, degree, range);
	} while (ret != 0);
}

/* Keeps on attempting to release the read lock until we succeed */
void orient_read_unlock(int degree, int range)
{
	int ret = -1;
	do {
		ret = syscall(383, degree, range);
	} while (ret != 0);
}

/* Keeps on attempting to acquire a write lock until we succeed */
void orient_write_lock(int degree, int range)
{
	int ret = -1;
	do {
		ret = syscall(382, degree, range);
	} while (ret != 0);
}

/* Keeps on attempting to release the write lock until we succeed */
void orient_write_unlock(int degree, int range)
{
	int ret = -1;
	do {
		ret = syscall(384, degree, range);
	} while (ret != 0);
}
