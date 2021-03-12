#include "sched.h"

#include <trace/events/power.h>
#include <linux/printk.h>
#include <linux/list.h>

#ifdef CONFIG_SMP
static int
select_task_rq_idle(struct task_struct *p, int cpu, int sd_flag, int flags)
{
//	printk(KERN_DEBUG "wrr:select_task_rq_idle\n");
	return task_cpu(p); /* IDLE tasks as never migrated */
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
pick_next_task_idle(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	return NULL;
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

	wrr_rq->nr_task++;
	wrr_rq->total_weight += wrr_se->weight;
}

/*
 * It is not legal to sleep in the idle task - print a warning
 * message if some code attempts to do it:
 */
static void
dequeue_task_idle(struct rq *rq, struct task_struct *p, int flags)
{
//	printk(KERN_DEBUG "wrr:dequeue_task_idle\n");
	raw_spin_unlock_irq(&rq->lock);
	printk(KERN_ERR "bad scheduling from wrr class\n");
	dump_stack();
	raw_spin_lock_irq(&rq->lock);
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
//	printk(KERN_DEBUG "wrr:static void switched_to_idle\n");
	BUG();
}

static void
prio_changed_idle(struct rq *rq, struct task_struct *p, int oldprio)
{
//	printk(KERN_DEBUG "wrr:prio_changed_idle\n");
	BUG();
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

const struct sched_class wrr_sched_class = {
	.next			= &fair_sched_class,
	/* no enqueue/yield_task for idle tasks */

	.enqueue_task		= enqueue_task_wrr,
	/* dequeue is not valid, we print a debug message there: */
	.dequeue_task		= dequeue_task_idle,

	.check_preempt_curr	= check_preempt_curr_idle,

	.pick_next_task		= pick_next_task_idle,
	.put_prev_task		= put_prev_task_idle,
	.set_next_task          = set_next_task_idle,

#ifdef CONFIG_SMP
	.balance		= balance_idle,
	.select_task_rq		= select_task_rq_idle,
	.set_cpus_allowed	= set_cpus_allowed_common,
#endif

	.task_tick		= task_tick_idle,

	.get_rr_interval	= get_rr_interval_idle,

	.prio_changed		= prio_changed_idle,
	.switched_to		= switched_to_idle,
	.update_curr		= update_curr_idle,
};
