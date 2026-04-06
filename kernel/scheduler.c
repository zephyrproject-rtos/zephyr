#include <run_q.h>


void z_sched_init(void)
{
#ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	for (int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		_priq_run_init(&_kernel.cpus[i].ready_q.runq);
	}
#else
	_priq_run_init(&_kernel.ready_q.runq);
#endif /* CONFIG_SCHED_CPU_MASK_PIN_ONLY */
}
