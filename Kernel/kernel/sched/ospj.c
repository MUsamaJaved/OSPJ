/*
 *  OSPJ VM Scheduling class (mapped to SCHED_VMS)
 *
 *  Copyright (C) 2014, 2015 TU Berlin
 *
 */

//#define OSPJ_DEBUG

#ifdef OSPJ_DEBUG
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#endif /* OSPJ_DEBUG  */

//#include <linux/cpufreq.h>

#include "sched.h"

// #define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define dprintk(...) printk(__VA_ARGS__)
#else
#define dprintk(...) {}
#endif

#define VA_ARGS(...) , ##__VA_ARGS__
#define d(msg, ...) dprintk(KERN_DEBUG "[OSPJ c=%04d] " msg "\n", current->pid VA_ARGS(__VA_ARGS__))
#define p(msg, ...) printk(KERN_INFO "[OSPJ c=%04d] " msg "\n", current->pid VA_ARGS(__VA_ARGS__))
#define e(msg, ...) printk(KERN_ERR "[OSPJ c=%04d] " msg "\n", current->pid VA_ARGS(__VA_ARGS__))

/*
 * default timeslice is 100 msecs (default RR time slice frin RT sched class).
 */
#define OSPJ_TIMESLICE	RR_TIMESLICE /* (100 * HZ / 1000) */

/*
 * The super period value in time slices. Default is 200 ms (20 time slices).
 */
#define SUPER_PERIOD_INTERVAL (OSPJ_TIMESLICE * 2)

#define SUPER_PERIOD_TICKS SUPER_PERIOD_INTERVAL

static inline void replenish_super_period(struct ospj_rq *ospj_rq)
{
	p("replenish_super_period() SPP = %d ticks", SUPER_PERIOD_TICKS);
	ospj_rq->period_ticks = SUPER_PERIOD_TICKS;
}

void dump_list(const char *name, struct list_head *list)
{
	struct list_head *i;
	d("List '%s': ###################################", name);
	list_for_each(i, list) {
		struct task_struct *entry = list_entry(i, struct task_struct, ospj_list_node);
		d("\tpid=%d, on_rq=%d", entry->pid, entry->on_rq);
	}
	d(" ----------------------------------------------");
}

void init_ospj_rq(struct ospj_rq *ospj_rq, struct rq *rq)
{
	ospj_rq->rq = rq;
	ospj_rq->nr_running = 0;
	ospj_rq->ptr_eligible_q = &ospj_rq->eligible_q;
	ospj_rq->ptr_waiting_q = &ospj_rq->waiting_q;

	ospj_rq->idle_request = 0;

	INIT_LIST_HEAD(ospj_rq->ptr_eligible_q);
	INIT_LIST_HEAD(ospj_rq->ptr_waiting_q);

	replenish_super_period(ospj_rq);

	d("Scheduler Class run queues init. successfully.");
}

/*
 * Calculate the time slice to be assigned to task according to its share
 */
static void ospj_calc_time_slice(struct task_struct *p)
{
	unsigned ts;
	d("ospj_calc_time_slice()");

	ts = (p->share * SUPER_PERIOD_INTERVAL) / 100;

	BUG_ON(ts <= 0);

	p->ospj_time_slice = ts;
	p->ospj_assigned_time_slice = ts;

	p("pid %d share %d ts %d", p->pid, p->share, ts);
}

/*
 * Update the shares of a current running vcpu/task
 */
void ospj_update_time_slice(struct task_struct *p, unsigned int share)
{
	/* Already consumed time slices during this period */
	unsigned int consumed_ts = p->ospj_assigned_time_slice - p->ospj_time_slice;
	unsigned int new_ts;
	
	/* Update the shares */
	p->share = share;

	/* Update the time slice according to the new defined share */
	ospj_calc_time_slice(p);

	new_ts = p->ospj_time_slice;

	/* Eventually adjust the updated time slice if the vcpu already burned
	 * some cpu time
	 */
	if (consumed_ts > 0) {
		if (new_ts <= consumed_ts) 
			p->ospj_time_slice = 0;
		else
			p->ospj_time_slice -= consumed_ts;
	}
}

/*
 * Update the current task's runtime statistics. Skip current tasks that are
 * not in our scheduling class.
 */
static void update_curr_ospj(struct rq *rq)
{
	struct task_struct *curr;
	u64 now, delta_exec; /* runtime */

	d("update_curr_ospj()");

	curr = rq->curr;
	now = rq->clock_task;

	if (curr->sched_class != &ospj_sched_class) {
		return;
	}

	/*
	 * Get the amount of time the current task was running
	 * since the last time we changed load (this cannot
	 * overflow on 32 bits):
	 */
	delta_exec = (unsigned long)(now - curr->se.exec_start);

	if (unlikely((s64)delta_exec <= 0))
	{ return; }

	schedstat_set(curr->se.statistics.exec_max,
				  max(curr->se.statistics.exec_max, delta_exec));

	/* update the task's over all runtime */
	curr->se.sum_exec_runtime += delta_exec;

	curr->se.exec_start = now;
	cpuacct_charge(curr, delta_exec);
}

/*
 * Adds new process to run queue. This function is called when the task enters
 * a runnable state.
 */
static void enqueue_task_ospj(struct rq *rq, struct task_struct *p, int flags)
{
	if (p == rq->ospj.idle)
		return;

	list_add_tail(&p->ospj_list_node, rq->ospj.ptr_eligible_q);

	rq->ospj.nr_running++;

	d("enqueue(): add %d, nr_running = %d", p->pid, rq->ospj.nr_running );
}

/*
 * Takes a process off the run queue. This happens when a process switches from
 * a runnable to an un-runnable state, or when the decides to take it off the
 * runqueue for other reasons.
 */
static void dequeue_task_ospj(struct rq *rq, struct task_struct *p, int flags)
{
	if (p == rq->ospj.idle)
		return;

	/* Update runtime statistics */
	update_curr_ospj(rq);

	list_del_init(&p->ospj_list_node);

	rq->ospj.nr_running--;

	dprintk(KERN_INFO "[OSPJ] dequeue(): del %d, nr_running = %d\n",
		   p->pid, rq->ospj.nr_running );
}

/*
 * When a task yields the CPU or is preempted, it is requeued on the waiting Q
 * till the end of the current super period.
 */
static void requeue_task_ospj(struct rq *rq, struct task_struct *p)
{
	list_move_tail(&p->ospj_list_node, rq->ospj.ptr_waiting_q);
	dprintk(KERN_DEBUG "[OSPJ] requeue(): del %d, n = %d\n",
		   p->pid, rq->ospj.nr_running );
}

/*
 * This is called when the current process is relinquishing control of the CPU
 * voluntary. This is invoked from the 'sched_yield' system call. This moves the
 * current task to the end of waiting queue.
 */
static void yield_task_ospj(struct rq *rq)
{
	dprintk(KERN_DEBUG "[OSPJ] yield() %d\n", rq->curr->pid);

#ifdef CONFIG_SCHED_VMS_3
	/*
	 * Here task are stay in the eligible_q as it is allow to come back at
	 * wake up.
	 * IMPORTANT/TODO: in this case the Idle period will not be entered
	 * before all sleeping tasks wake ups. Correct solution is to preempt
	 * properly the idle task during idle period in case of a task wake up
	 */
	return;
#else
	requeue_task_ospj(rq, rq->curr);
#endif /* CONFIG_SCHED_VMS_3 */
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_curr_ospj(struct rq *rq, struct task_struct *p, int flags)
{
	if (p->policy != SCHED_VMS || rq->curr == rq->idle) {
		d("check_preempt_curr_ospj() called for non-VMS task or Idle time");
		return;
	}

	/* Update runtime statistic */
	update_curr_ospj(rq);

	dprintk(KERN_DEBUG "[OSPJ] check_preempt_curr_ospj(); // old: %d; new: %d\n",
		   p->pid, rq->curr->pid);

#ifdef CONFIG_SCHED_VMS_1
	/*
	 * a vcpu that has not exhausted its time slice during this super
	 * period and now wakes upis not allowed to come back. Task is already
	 * on the waiting_q
	 */
	return;
#endif /* CONFIG_SCHED_VMS_1 */

#ifdef CONFIG_SCHED_VMS_2
	if (!is_idle_period()) {
		set_tsk_need_resched(rq->curr);
		return
	}
#endif /* CONFIG_SCHED_VMS_2 */

#ifdef CONFIG_SCHED_VMS_3
	set_tsk_need_resched(rq->curr);
	return;
#endif /* CONFIG_SCHED_VMS_3 */
}

static unsigned int is_idle_period(struct rq *rq)
{
	if (list_empty(rq->ospj.ptr_eligible_q) != 0
		&& list_empty(rq->ospj.ptr_waiting_q) == 0
	   ) {
		return 1;
	}

	return 0;
}

/*
 * Happens when we need to preempt the current task with a newly woken task. This
 * is called, for instance, when a new task is woken up with 'wake_up_new_task'.
 */
static struct task_struct *pick_next_task_ospj(struct rq *rq)
{
	struct task_struct *next = NULL;
	struct list_head *tmp;

	if (rq->ospj.nr_running == 0) {
		return NULL;
	}

	dump_list("eligible_q", rq->ospj.ptr_eligible_q);
	dump_list("waiting_q", rq->ospj.ptr_waiting_q);

	if (is_idle_period(rq)) {
		d("pick_next_task_ospj()");

		/*
		 * During the idle Period we "steal" the Idle Task from the Idle
		 * scheduler class.
		 */
		next = rq->ospj.idle;
		next->sched_class = &ospj_sched_class;

		/* If we have at least one tick() to save energy */
		if (rq->ospj.period_ticks > 0) {
			
			/*
			 * If we're entering the idle period (if it is starting now)
			 * set up the idle time slice with the remaining super
			 * period ticks
			 */
			if (!rq->ospj.idle_request) {
				d("Idle task class set to ospj sched class");
				next->ospj_time_slice = rq->ospj.period_ticks;
				next->ospj_assigned_time_slice = rq->ospj.period_ticks;
				//cpufreq_ospj_idle(cpu_of(rq), 1);
			}

			p("Idle time slice %d", next->ospj_time_slice);	

			rq->ospj.idle_request = 1;
			d("Idle from ospj!");

			return next;

		} else {

			next->sched_class = &idle_sched_class;
			d("pick_next() Idle task class set back to idle sched class");
			rq->ospj.idle_request = 0;
			replenish_super_period(&rq->ospj);
			d("Entering next Super Period");
		}

		/* Swap Qs */
		tmp = rq->ospj.ptr_waiting_q;
		rq->ospj.ptr_waiting_q = rq->ospj.ptr_eligible_q;
		rq->ospj.ptr_eligible_q = tmp;
		d("Queues swaped\n");
	}

	d("pick_next_task_ospj() is served from eligible_q");

	next = list_entry(rq->ospj.ptr_eligible_q->next, struct task_struct, ospj_list_node);
	next->se.exec_start = rq->clock_task;

	dprintk(KERN_DEBUG "[OSPJ] pick_next(): next to run is %d (%d running)\n", next->pid, rq->ospj.nr_running);

	return next;
}

/*
 * This function is called before the currently executing task is replaced with
 * another one.
 */
static void put_prev_task_ospj(struct rq *rq, struct task_struct *p)
{
	d("put_prev_task_ospj()");
	/* Update runtime statistic */
	update_curr_ospj(rq);
	p->se.exec_start = 0;
}

/*
 * updates statitics for the _current_ task when scheduling policy changes
 */
static void set_curr_task_ospj(struct rq *rq)
{
	struct task_struct *p;

	d("set_curr_task_ospj()");

	p = rq->curr;
	p->se.exec_start = rq->clock_task;
}

/*
 * Scheduler's timer 'tick'
 */
static void task_tick_ospj(struct rq *rq, struct task_struct *p, int queued)
{
	/* Update runtime statistics */
	update_curr_ospj(rq);

	if (rq->ospj.period_ticks > 0)
		/* Update super period counter */
		rq->ospj.period_ticks--;

	dprintk(KERN_INFO "[OSPJ] task_tick(); pid %d - ts %d\n", p->pid, p->ospj_time_slice);

	if (p->ospj_time_slice > 0)
		p->ospj_time_slice--;

	if (p->ospj_time_slice <= 0) {
		if (rq->idle == p) {
			p->sched_class = &idle_sched_class;
			set_tsk_need_resched(p);
			//cpufreq_ospj_idle(cpu_of(rq), 0);
			rq->ospj.idle_request = 0;
			d("Clear Idle request");
			return;
		}

		/* replenish time slice */
		p->ospj_time_slice = p->ospj_assigned_time_slice;
		set_tsk_need_resched(p);
		yield_task_ospj(rq);
		return;
	}
	else {
		p("Remaining time slice: %d; period_ticks=%d, idle=%s", 
			p->ospj_time_slice,
			rq->ospj.period_ticks,
			is_idle_period(rq) ? "true" : "false");
	}
}

/*
 * SCHED_VMS doesn't perform any fork operation on tasks.
 */
static void task_fork_ospj(struct task_struct *p)
{
	d("Task %d (p=%d) is being forked into %d (p=%d).",
		   p->real_parent->pid, p->real_parent->policy,  p->pid, p->policy);
	return;
}

/*
 * The current task switches from our scheduler policy to an another one.
 */
static void switched_from_ospj(struct rq *this_rq, struct task_struct *task)
{
	printk("switched_from_ospj()\n");
}

/*
 * The current process switched to SCHED_VMS policy
 */
static void switched_to_ospj(struct rq *rq, struct task_struct *task)
{
	printk("switched_to_ospj()\n");

	/*
	 * When a task switches to SCHED_VMS compute its time slice first
	 */
	ospj_calc_time_slice(task);

#ifdef CONFIG_SCHED_VMS_1
	/*
	 * Here there nothing to be done. New switched just wait for its turn
	 * to be scheduled
	 */
	return;
#endif /* CONFIG_SCHED_VMS_1 */

#if defined(CONFIG_SCHED_VMS_2) || defined(CONFIG_SCHED_VMS_3)
	/*
	 * Let new switched tasks preempt the current running. If we already running
	 * then nothing needs to be done
	 */
	if (task->on_rq && rq->curr != task) {
		d("switched_to_ospj(): resched_task");
		resched_task(rq->curr);
	}
#endif
}

static void prio_changed_ospj(struct rq *this_rq, struct task_struct *task,
							  int oldprio)
{
	d("prio_changed_ospj()");
	return;
}

static unsigned int get_rr_interval_ospj(struct rq *rq, struct task_struct *task)
{
	return (unsigned int)task->ospj_time_slice;
}

#ifdef CONFIG_SMP
/*
 * called when a task that was not previously on a run queue (e,g because it was
 * blocked or newly created) needs to be placed on a run queue;
 * Chooses on which run queue (i.e, on which CPU) a waking-au task has to be
 * enqueued.
 */
static int select_task_rq_ospj(struct task_struct *p, int sd_flag, int flags)
{
	//TODO
	return task_cpu(p);
}

static void pre_schedule_ospj(struct rq *this_rq, struct task_struct *task)
{
}

static void post_schedule_ospj(struct rq *this_rq)
{
}

static void task_woken_ospj(struct rq *this_rq, struct task_struct *task)
{
}

static void set_cpus_allowed_ospj(struct task_struct *p,
								  const struct cpumask *newmask)
{
}

static void rq_online_ospj(struct rq *rq)
{
}

static void rq_offline_ospj(struct rq *rq)
{
}
#endif /* CONFIG_SMP */

#ifdef OSPJ_DEBUG
/* Iterator */
static void *debug_start(struct seq_file *debug, loff_t *pos)
{
	d("debug_start()");

	if (!pos)
	{ return SEQ_START_TOKEN; }

	return NULL;
}

static void *debug_next(struct seq_file *debug, void *v, loff_t *pos)
{
	return NULL;
}


static void debug_stop(struct seq_file *debug, void *v)
{
	d("debug_stop()");
}

static int debug_show(struct seq_file *debug, void *v)
{
	if (v == SEQ_START_TOKEN) {
		seq_puts(debug, "Eligible_Q\t\tWaiting_Q\t\tnr_runnig\n");
		return 0;
	}

	seq_puts(debug, "--\t\t--\t\t--");

	return 0;
}

static const struct seq_operations debug_op = {
	.start =	debug_start,
	.next =		debug_next,
	.stop =		debug_stop,
	.show =		debug_show
};

static void switched_from_ospj(struct rq *this_rq, struct task_struct *task)
{
	dprintk(KERN_DEBUG "[OSPJ] switched_from(); pid %d - policy %d\n",
		   task->pid, task->policy);
}

/*
 * The current process switched to SCHED_VMS policy
 */
static void switched_to_ospj(struct rq *this_rq, struct task_struct *task)
{
	dprintk(KERN_DEBUG "[OSPJ] switched_to(); pid %d - policy %d\n",
		   task->pid, task->policy);

	/*
	 * kick off the schedule if running, otherwise just see
	 * if we can still preempt the current task.
	 */
	if (this_rq->curr == task)
		resched_task(this_rq->curr);
	else
		check_preempt_curr(this_rq, task, 0);
}

static int ospj_debug_open(struct inode *inode, struct file *file)
{
	d("ospj_debug_open()");
	return seq_open(file, &debug_op);
	// dprintk(KERN_INFO "[OSPJ] prio_changed() pid %d current prio %d old prio%d\n",
	// 		this_rq->curr->pid, this_rq->curr->prio, oldprio);
	// return;
}

static const struct file_operations ospj_debug_operations = {
	.open		= ospj_debug_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_osjp_init(void)
{
	if (!proc_create("ospj_debug", 0, NULL, &ospj_debug_operations))
	{ return -ENOMEM; }

	return 0;
}
#endif /* OSPJ_DEBUG */

/*
 * All the scheduling class methods:
 */
const struct sched_class ospj_sched_class = {
	.next			= &idle_sched_class,
	.enqueue_task		= enqueue_task_ospj,
	.dequeue_task		= dequeue_task_ospj,
	.yield_task		= yield_task_ospj,

	.check_preempt_curr	= check_preempt_curr_ospj,

	.pick_next_task		= pick_next_task_ospj,
	.put_prev_task		= put_prev_task_ospj,
	.set_curr_task          = set_curr_task_ospj,
	.task_tick		= task_tick_ospj,
	.task_fork		= task_fork_ospj,

	.prio_changed		= prio_changed_ospj,
	.switched_from		= switched_from_ospj,
	.switched_to		= switched_to_ospj,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_ospj,

	.pre_schedule		= pre_schedule_ospj,
	.post_schedule		= post_schedule_ospj,
	.task_woken		= task_woken_ospj,
	.set_cpus_allowed	= set_cpus_allowed_ospj,
	.rq_online		= rq_online_ospj,
	.rq_offline		= rq_offline_ospj,
#endif

	.get_rr_interval	= get_rr_interval_ospj
};


__init void init_sched_ospj_class(void)
{
#ifdef OSPJ_DEBUG
	proc_osjp_init();
#endif /* OSPJ_DEBUG */
}
