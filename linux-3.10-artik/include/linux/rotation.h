#ifndef _LINUX_ROTATION_H
#define _LINUX_ROTATION_H

#include <linux/wait.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <asm/atomic.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/pid.h>
#include <linux/sched.h>


/* Types for lock entry */
#define READER_ENTRY 0
#define WRITER_ENTRY 1

/* Orientation range vallue constants */
#define MIN_DEGREE 0
#define MAX_DEGREE 360

int current_degree;

struct rotation_range {
    int degree;
	int range;
};

struct lock_entry {
	struct rotation_range *range;
	atomic_t granted;
	struct list_head list; /* Waiters list */
	struct list_head granted_list;
	int type; /* 0 for read 1 for write */
	int pid; /* pid of process that runs this */
};

LIST_HEAD(waiters_list);
LIST_HEAD(granted_list);

spinlock_t WAITERS_LOCK;
spinlock_t GRANTED_LOCK;
spinlock_t SET_LOCK;

DECLARE_WAIT_QUEUE_HEAD(sleepers);
#endif
