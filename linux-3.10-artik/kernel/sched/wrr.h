#ifndef _SCHED_WRR_H
#define _SCHED_WRR_H

#define WRR_TIMESLICE 10
#define MAX_WRR_WEIGHT 10
#define MAX_CPUS 8

struct wrr_info {
	struct list_head run_list;
	struct task_struct *task;
	unsigned int weight;
	unsigned long time_slice;
	unsigned long time_left;
};

#endif
