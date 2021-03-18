#include "sched.h"

#include <trace/events/power.h>
#include <linux/cpumask.h>
#include <linux/limits.h>
#include <linux/list.h>

#define MSECS 1000000 /* msec to nanosec */
#define WRR_BASE_TIME (10 * MSECS)

#ifdef CONFIG_SMP
static int
select_task_rq_wrr(struct task_struct *p, int cpu, int sd_flag, int flags)
{
	/* returns the cpu with the lowest total_weight */
	struct rq *rq;
	int min_weight = INT_MAX, min_cpu = -1;

	for_each_online_cpu(cpu) {
		rq = cpu_rq(cpu);
		if (rq->wrr.total_weight < min_weight) {
			min_weight = rq->wrr.total_weight;
			min_cpu = cpu;
		}
	}

	return min_cpu;
}

/*
 * called in pick_next_task before per class pick_next_task is called
 */
static int
balance_wrr(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	return 0;
}
#endif

/*
 * gets called when the new ready process and the current running
 * process both uses wrr scheduling class.
 *
 * wrr is non-preemptive -> no-op
 */
static void
check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags)
{
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *prev)
{
}

static void set_next_task_wrr(struct rq *rq, struct task_struct *next)
{
}

static struct task_struct *
pick_next_task_wrr(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	struct task_struct *p;
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *wrr_se;

	wrr_rq = &rq->wrr;

	if (wrr_rq->nr_task == 0)
		return NULL;

	/* has runnable tasks */
	wrr_se = wrr_rq->curr;
	wrr_rq->curr = list_next_entry(wrr_se, entry);

	p = container_of(wrr_rq->curr, struct task_struct, wrr);
	p->se.exec_start = rq_clock_task(rq);

	return p;
}

static void
enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *wrr_se, *curr;

	wrr_rq = &rq->wrr;
	wrr_se = &p->wrr;

	if (wrr_rq->nr_task == 0) {
		INIT_LIST_HEAD(&wrr_se->entry);
		wrr_rq->curr = wrr_se;
	} else {
		/* wrr_rq is not empty: enqueue task before head */
		curr = wrr_rq->curr;
		__list_add(&wrr_se->entry, curr->entry.prev, &curr->entry);
	}

	if (wrr_se->weight == 0)
		wrr_se->weight = 1;
	wrr_se->time_slice = wrr_se->weight * WRR_BASE_TIME;

	wrr_rq->nr_task++;
	wrr_rq->total_weight += wrr_se->weight;
	add_nr_running(rq, 1);
}

static void
dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *curr;

	wrr_rq = &rq->wrr;

	if (wrr_rq->nr_task == 0)
		return;

	/* wrr_rq is not empty */
	curr = wrr_rq->curr;
	__list_del(curr->entry.prev, curr->entry.next);

	/* update wrr_rq->curr to the one before wrr_rq->curr */
	wrr_rq->curr = list_prev_entry(curr, entry);

	wrr_rq->nr_task--;
	wrr_rq->total_weight -= curr->weight;
	sub_nr_running(rq, 1);
}

/*
 * Called by do_sched_yield() with rq->lock held and upon return,
 * schedule() is called to yield the CPU to another task
 */
static void yield_task_wrr(struct rq *rq)
{
}

/*
 * scheduler tick hitting a task of our scheduling class.
 *
 * NOTE: This function can be called remotely by the tick offload that
 * goes along full dynticks. Therefore no local assumption can be made
 * and everything must be accessed through the @rq and @p passed in
 * parameters.
 */
static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	struct sched_wrr_entity *curr = &p->wrr;
	u64 now, delta_exec;

	now = rq_clock_task(rq);
	delta_exec = now - p->se.exec_start;

	/* if time is not up, return */
	if (delta_exec < curr->time_slice)
		return;

	curr->time_slice = curr->weight * WRR_BASE_TIME;
	resched_curr(rq);
}

static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
}

static void
prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{
}

/*
 * called by task_sched_runtime
 */
static void update_curr_wrr(struct rq *rq)
{
}

void init_wrr_rq(struct wrr_rq *wrr_rq)
{
	wrr_rq->total_weight = 0;
	wrr_rq->nr_task = 0;
	wrr_rq->curr = NULL;
}

const struct sched_class wrr_sched_class = {
	.next			= &fair_sched_class,

	.enqueue_task		= enqueue_task_wrr,
	.dequeue_task		= dequeue_task_wrr,
	.yield_task		= yield_task_wrr,

	.check_preempt_curr	= check_preempt_curr_wrr,

	.pick_next_task		= pick_next_task_wrr,
	.put_prev_task		= put_prev_task_wrr,
	.set_next_task          = set_next_task_wrr,

#ifdef CONFIG_SMP
	.balance		= balance_wrr,
	.select_task_rq		= select_task_rq_wrr,
	.set_cpus_allowed	= set_cpus_allowed_common,
#endif

	.task_tick		= task_tick_wrr,

	.prio_changed		= prio_changed_wrr,
	.switched_to		= switched_to_wrr,
	.update_curr		= update_curr_wrr,
};
