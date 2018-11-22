/*
 * orient_lock.h
 *
 *  Created on: Oct 13, 2012
 *      Author: w4118
 */

#ifndef ORIENT_LOCK_H_
#define ORIENT_LOCK_H_

#include "../linux-3.10-artik/arch/arm/include/asm/unistd.h"
#include <unistd.h>

struct rotation_range {
    int degree;
	int range;
};

/* Keeps on attempting to acquire a Read lock until we succeed */
void orient_read_lock(int degree, int range);

/* Keeps on attempting to release the read lock until we succeed */
void orient_read_unlock(int degree, int range);


/* Keeps on attempting to acquire a write lock until we succeed */
void orient_write_lock(int degree, int range);

/* Keeps on attempting to release the write lock until we succeed */
void orient_write_unlock(int degree, int range);


#endif /* ORIENT_LOCK_H_ */
