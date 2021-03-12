#include "sched.h"

#include <trace/events/power.h>
#include <linux/printk.h>
#include <linux/cpumask.h> 
#include <linux/limits.h>
#include <linux/list.h>

#ifdef CONFIG_SMP
static int
select_task_rq_wrr(struct task_struct *p, int cpu, int sd_flag, int flags)
{
	/* returns the cpu with the lowest total_weight */
	struct rq *rq;
	int min_weight = INT_MAX, min_cpu = -1;

	printk(KERN_INFO "Inside [select_task_rq_wrr]\n");

	for_each_possible_cpu(cpu){
		rq = cpu_rq(cpu);
		if(rq->wrr.total_weight < min_weight){
			min_weight = rq->wrr.total_weight;
			min_cpu = cpu;
		}
	}
	printk(KERN_INFO "[select_task_rq_wrr] min_weight=%d, min_cpu = %d\n", min_weight, min_cpu);
	return min_cpu;
}

static int
balance_idle(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
//	printk(KERN_DEBUG "wrr:balance_idle\n");
	return 0;
}
#endif

/*
 * Idle tasks are unconditionally rescheduled:
 */
static void check_preempt_curr_idle(struct rq *rq, struct task_struct *p, int flags)
{
//	printk(KERN_DEBUG "wrr:check_preempt_curr_idle\n");
	resched_curr(rq);
}

static void put_prev_task_idle(struct rq *rq, struct task_struct *prev)
{
//	printk(KERN_DEBUG "wrr:put_prev_task_idle\n");
}

static void set_next_task_idle(struct rq *rq, struct task_struct *next)
{
//	printk(KERN_DEBUG "wrr:set_next_task_idle\n");
	update_idle_core(rq);
	schedstat_inc(rq->sched_goidle);
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
	printk(KERN_INFO "Inside [pick_next_task]\n");

	wrr_se = wrr_rq->curr;
	p = container_of(wrr_se, struct task_struct, wrr);

	wrr_rq->curr = list_next_entry(wrr_se, entry);
	
	printk(KERN_INFO "[pick_next_task] picked %s\n", p->comm);

	return p;
}

static void
enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *wrr_se, *curr;
	
	printk(KERN_INFO "Inside [enqueue_task_wrr]\n");

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
	
	printk(KERN_INFO "[enqueue_task_wrr] added %s\n", p->comm);

	wrr_rq->nr_task++;
	wrr_rq->total_weight += wrr_se->weight;
}

/*
 * It is not legal to sleep in the idle task - print a warning
 * message if some code attempts to do it:
 */
static void
dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *curr;

	wrr_rq = &rq->wrr;

	printk(KERN_INFO "Inside [dequeue_task_rq_wrr]\n");

	if (wrr_rq->nr_task == 0)
		return;

	/* wrr_rq is not empty */
	curr = wrr_rq->curr;
	__list_del(curr->entry.prev, curr->entry.next);

	wrr_rq->nr_task--;
	wrr_rq->total_weight -= curr->weight;
	
	printk(KERN_INFO "[dequeue_task_rq_wrr] dequeued task, weight = %d\n", wrr_rq->total_weight);
}

/*
 * scheduler tick hitting a task of our scheduling class.
 *
 * NOTE: This function can be called remotely by the tick offload that
 * goes along full dynticks. Therefore no local assumption can be made
 * and everything must be accessed through the @rq and @curr passed in
 * parameters.
 */
static void task_tick_idle(struct rq *rq, struct task_struct *curr, int queued)
{
//	printk(KERN_DEBUG "wrr:static void task_tick_idle\n");
}

static void switched_to_idle(struct rq *rq, struct task_struct *p)
{
	/* DELETED BUG() */
}

static void
prio_changed_idle(struct rq *rq, struct task_struct *p, int oldprio)
{
	/* DELETED BUG() */
}

static unsigned int get_rr_interval_idle(struct rq *rq, struct task_struct *task)
{
//	printk(KERN_DEBUG "wrr:get_rr_interval_idle\n");
	return 0;
}

static void update_curr_idle(struct rq *rq)
{
//	printk(KERN_DEBUG "wrr:update_curr_idle\n");
}
void init_wrr_rq(struct wrr_rq* wrr_rq)
{
	wrr_rq->total_weight = 0;
	wrr_rq->nr_task = 0;
	wrr_rq->curr = NULL;
}

const struct sched_class wrr_sched_class = {
	.next			= &fair_sched_class,
	/* no enqueue/yield_task for idle tasks */

	.enqueue_task		= enqueue_task_wrr,
	/* dequeue is not valid, we print a debug message there: */
	.dequeue_task		= dequeue_task_wrr,

	.check_preempt_curr	= check_preempt_curr_idle,

	.pick_next_task		= pick_next_task_wrr,
	.put_prev_task		= put_prev_task_idle,
	.set_next_task          = set_next_task_idle,

#ifdef CONFIG_SMP
	.balance		= balance_idle,
	.select_task_rq		= select_task_rq_wrr,
	.set_cpus_allowed	= set_cpus_allowed_common,
#endif

	.task_tick		= task_tick_idle,

	.get_rr_interval	= get_rr_interval_idle,

	.prio_changed		= prio_changed_idle,
	.switched_to		= switched_to_idle,
	.update_curr		= update_curr_idle,
};
