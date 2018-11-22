#include <linux/rotation.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>

static int range_equals(struct rotation_range *range,
		 struct rotation_range *target)
{
	return (range->degree == target->degree &&
        range->range == target->range);
}

static int generic_search_list(struct rotation_range *target,
			struct list_head *list, int type)
{
	int flag = 1;
	struct list_head *current_item;
	list_for_each(current_item, list) {
		struct lock_entry *entry;
		if (list == &waiters_list)
			entry = list_entry(current_item,
					struct lock_entry, list);
		else
			entry = list_entry(current_item, struct lock_entry,
					   granted_list);
		if (entry->type == type &&
		    range_equals(entry->range, target)) {
			flag = 0;
			break;
		}
	}
	return flag;
}

static int no_writer_grabbed(struct rotation_range *target)
{	
	int rc;
	spin_lock(&GRANTED_LOCK);
	rc = generic_search_list(target, &granted_list, 1);
	spin_unlock(&GRANTED_LOCK);
	return rc;
}

static int no_reader_grabbed(struct rotation_range *target)
{
	int rc;
	spin_lock(&GRANTED_LOCK);
	rc = generic_search_list(target, &granted_list, 0);
	spin_unlock(&GRANTED_LOCK);
	return rc;
}

static void grant_lock(struct lock_entry *entry)
{
	atomic_set(&entry->granted,1);

	spin_lock(&GRANTED_LOCK);
	list_add_tail(&entry->granted_list, &granted_list);
	spin_unlock(&GRANTED_LOCK);

	if (&entry->list == NULL) {
		printk("OOPS: &entry->list is NULLLLLLL ");
	}

	if (entry->list.next == NULL) {
		printk("Next entry is NULLL\n");
	}

	if(entry->list.prev == NULL) {
		printk("Preve entry is NULL");
	}

	list_del(&entry->list);
	wake_up(&sleepers);
}

static int in_range(struct rotation_range *range, int degree)
{
	if ((degree > range->degree + range->range ||
			degree < range->degree - range->degree)
			&& range->range != 0) {
		return 0;
	}

	return 1;
}


/* *
 * Determines if the task with given pid is still running.
 * Returns 1 if true and 0 if false.
 * Tested this on emulator to confirm it's working.
 */
static int is_running(int pid){

	struct pid *pid_struct;
	struct task_struct *task;
	int is_dead, is_wakekill, exit_zombie, exit_dead;
	pid_struct = find_get_pid(pid);

	if(pid_struct == NULL) {
		return 0;
	}
	task = pid_task(pid_struct,PIDTYPE_PID);
	if(task == NULL) {
		return 0;
	}

	is_dead = (task->state & TASK_DEAD) != 0;
	is_wakekill = (task->state & TASK_WAKEKILL) != 0;
	exit_zombie = (task->exit_state & EXIT_ZOMBIE) != 0;
	exit_dead = (task->exit_state & EXIT_DEAD) != 0;

	if(is_dead || is_wakekill || exit_zombie || exit_dead) {
		printk("Task is not RUNNING ever: %d", task->pid);
		return 0;
	} else {
		printk("Task is normal: %d ", task->pid);
		return 1;
	}
}

/**
 * Removes the locks for process that are no longer running
 * from the granted list.
 * NOTICE we acquire the GRANT LIST LOCK
 */
static void release_dead_tasks_locks(void) {
	struct list_head *current_item;
	struct list_head *next_item;
	int counter = 1;
	spin_lock(&GRANTED_LOCK);
	list_for_each_safe(current_item, next_item, &granted_list) {
		struct lock_entry *entry = list_entry(current_item,
						      struct lock_entry,
						      granted_list);
		int pid = entry->pid;
		if(!is_running(pid)) {
			list_del(current_item);
		}
		++counter;
	}
	spin_unlock(&GRANTED_LOCK);
}

static void process_waiter(struct list_head *current_item)
{
	struct lock_entry *entry = list_entry(current_item,
					      struct lock_entry, list);

	struct rotation_range *target = entry->range;

	if (in_range(entry->range, current_degree)) {
		if (entry->type == READER_ENTRY) { /* Reader */
			if (no_writer_grabbed(target))
				grant_lock(entry);
		}
		else { /* Writer */
			if (no_writer_grabbed(target) &&
			    no_reader_grabbed(target))
				grant_lock(entry);
		}
	}
}

SYSCALL_DEFINE1(set_rotation, int, degree)
{
	int counter;
	struct list_head *current_item, *next_item;
	spin_lock(&SET_LOCK);

	counter = 0;
	current_degree = degree;

	spin_lock(&WAITERS_LOCK);
	release_dead_tasks_locks();

	list_for_each_safe(current_item, next_item, &waiters_list) {
		process_waiter(current_item);
	}

	spin_unlock(&WAITERS_LOCK);
	spin_unlock(&SET_LOCK);
	return 0;
}

// TODO: Fix the list traversal in all SYS calls : use list_for_each_safe.
SYSCALL_DEFINE2(rotlock_read, int, degree, int, range)
{
	struct rotation_range *korient;
	struct lock_entry *entry;
	DEFINE_WAIT(wait);
	
	korient = kmalloc(sizeof(struct rotation_range), GFP_KERNEL);

	if (korient == NULL) {
		printk("Kmalloc failure: rotlock_read");
		return  -ENOMEM;
	}
	korient->degree = degree;
	korient->range = range;

	entry = kmalloc(sizeof(struct lock_entry), GFP_KERNEL);

	if (entry == NULL) {
		printk("\nKmalloc failure: rotlock_read");
		return -ENOMEM;
	}

	entry->range = korient;
	entry->pid = current->pid;
	atomic_set(&entry->granted,0);
	INIT_LIST_HEAD(&entry->list);
	INIT_LIST_HEAD(&entry->granted_list);
	entry->type = READER_ENTRY;

	spin_lock(&WAITERS_LOCK);
	list_add_tail(&entry->list, &waiters_list);
	spin_unlock(&WAITERS_LOCK);

	add_wait_queue(&sleepers, &wait);
	while(!atomic_read(&entry->granted)) {
	       prepare_to_wait(&sleepers, &wait, TASK_INTERRUPTIBLE);
	       schedule();
	}
	finish_wait(&sleepers, &wait);

	return 0;
}

SYSCALL_DEFINE2(rotlock_write, int, degree, int, range)
{
	struct rotation_range *korient;
	struct lock_entry *entry;
	DEFINE_WAIT(wait);

	korient = kmalloc(sizeof(struct rotation_range), GFP_KERNEL);

	if (korient == NULL) {
		printk("Kmalloc failure: orientlock_write\n");
		return  -ENOMEM;
	}
	korient->degree = degree;
	korient->range = range;

	entry = kmalloc(sizeof(entry), GFP_KERNEL);

	if (entry == NULL) {
		printk("Kmalloc failure!...\n");
		return -ENOMEM;
	}
	
	entry->range = korient;
	entry->pid = current->pid;
	atomic_set(&entry->granted, 0);
	INIT_LIST_HEAD(&entry->list);
	INIT_LIST_HEAD(&entry->granted_list);
	entry->type = WRITER_ENTRY;

	spin_lock(&WAITERS_LOCK);
	list_add_tail(&entry->list, &waiters_list);
	spin_unlock(&WAITERS_LOCK);

	add_wait_queue(&sleepers, &wait);
	while(!atomic_read(&entry->granted)) {
		prepare_to_wait(&sleepers, &wait, TASK_INTERRUPTIBLE);
		schedule();
	}
	finish_wait(&sleepers, &wait);
	return 0;
}

SYSCALL_DEFINE2(rotunlock_read, int, degree, int, range)
{
	struct rotation_range *korient;
	struct list_head *current_item, *next_item;
	struct lock_entry *entry = NULL;
	int did_unlock = 0;
	
	korient = kmalloc(sizeof(struct rotation_range), GFP_KERNEL);

	if (korient == NULL) {
		printk("Kmalloc failure: orientlock_write\n");
		return  -ENOMEM;
	}
	korient->degree = degree;
	korient->range = range;
	
	spin_lock(&GRANTED_LOCK);
	list_for_each_safe(current_item, next_item, &granted_list) {
		entry = list_entry(current_item,
				   struct lock_entry, granted_list);
		if (range_equals(korient, entry->range) &&
		    entry->type == READER_ENTRY) {
			int curr_pid = current->pid;
			if (curr_pid != entry->pid)
				continue;

			list_del(current_item);
			kfree(entry->range);
			kfree(entry);
			spin_unlock(&GRANTED_LOCK);
			did_unlock = 1;
			break;
		}
		else
			; //no locks with the rotation_range available
	}


	if (!did_unlock) {
		spin_unlock(&GRANTED_LOCK);
		return -1;
	} else {
		return 0;
	}
}

SYSCALL_DEFINE2(rotunlock_write, int, degree, int, range)
{
	struct rotation_range *korient;
	struct list_head *current_item, *next_item;

	korient = kmalloc(sizeof(struct rotation_range), GFP_KERNEL);

	if (korient == NULL) {
		printk("Kmalloc failure: orientlock_write\n");
		return  -ENOMEM;
	}
	korient->degree = degree;
	korient->range = range;
	
	spin_lock(&GRANTED_LOCK);
	list_for_each_safe(current_item, next_item, &granted_list) {
		struct lock_entry *entry;
		entry = list_entry(current_item,
				   struct lock_entry, granted_list);
		if (range_equals(korient, entry->range) &&
		    entry->type == WRITER_ENTRY) {
			list_del(current_item);
			kfree(entry->range);
			kfree(entry);
			spin_unlock(&GRANTED_LOCK);
			return 0;
		}	
		else
			; //no locks with the rotation_range available
	}
	spin_unlock(&GRANTED_LOCK);
	printk("Undefined error\n");
	return 0;
}