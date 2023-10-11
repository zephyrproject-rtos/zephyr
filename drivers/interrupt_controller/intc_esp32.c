/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <soc.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include "esp_attr.h"
#include <hal/cpu_hal.h>
#include <hal/interrupt_controller_hal.h>
#include <limits.h>
#include <assert.h>
#include "soc/soc.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_intc, CONFIG_LOG_DEFAULT_LEVEL);

#define ETS_INTERNAL_TIMER0_INTR_NO 6
#define ETS_INTERNAL_TIMER1_INTR_NO 15
#define ETS_INTERNAL_TIMER2_INTR_NO 16
#define ETS_INTERNAL_SW0_INTR_NO 7
#define ETS_INTERNAL_SW1_INTR_NO 29
#define ETS_INTERNAL_PROFILING_INTR_NO 11

#define VECDESC_FL_RESERVED     (1 << 0)
#define VECDESC_FL_INIRAM       (1 << 1)
#define VECDESC_FL_SHARED       (1 << 2)
#define VECDESC_FL_NONSHARED    (1 << 3)

/*
 * Define this to debug the choices made when allocating the interrupt. This leads to much debugging
 * output within a critical region, which can lead to weird effects like e.g. the interrupt watchdog
 * being triggered, that is why it is separate from the normal LOG* scheme.
 */
#ifdef CONFIG_INTC_ESP32_DECISIONS_LOG
# define INTC_LOG(...) LOG_INF(__VA_ARGS__)
#else
# define INTC_LOG(...) do {} while (false)
#endif

/* Typedef for C-callable interrupt handler function */
typedef void (*intc_handler_t)(void *);
typedef void (*intc_dyn_handler_t)(const void *);

/* shared critical section context */
static int esp_intc_csec;

static inline void esp_intr_lock(void)
{
	esp_intc_csec = irq_lock();
}

static inline void esp_intr_unlock(void)
{
	irq_unlock(esp_intc_csec);
}

/*
 * Interrupt handler table and unhandled interrupt routine. Duplicated
 * from xtensa_intr.c... it's supposed to be private, but we need to look
 * into it in order to see if someone allocated an int using
 * set_interrupt_handler.
 */
struct intr_alloc_table_entry {
	void (*handler)(void *arg);
	void *arg;
};

/* Default handler for unhandled interrupts. */
void default_intr_handler(void *arg)
{
	printk("Unhandled interrupt %d on cpu %d!\n", (int)arg, esp_core_id());
}

static struct intr_alloc_table_entry intr_alloc_table[ESP_INTC_INTS_NUM * CONFIG_MP_MAX_NUM_CPUS];

static void set_interrupt_handler(int n, intc_handler_t f, void *arg)
{
	irq_disable(n);
	intr_alloc_table[n * CONFIG_MP_MAX_NUM_CPUS].handler = f;
	irq_connect_dynamic(n, n, (intc_dyn_handler_t)f, arg, 0);
	irq_enable(n);
}

/* Linked list of vector descriptions, sorted by cpu.intno value */
static struct vector_desc_t *vector_desc_head; /* implicitly initialized to NULL */

/* This bitmask has an 1 if the int should be disabled when the flash is disabled. */
static uint32_t non_iram_int_mask[CONFIG_MP_MAX_NUM_CPUS];
/* This bitmask has 1 in it if the int was disabled using esp_intr_noniram_disable. */
static uint32_t non_iram_int_disabled[CONFIG_MP_MAX_NUM_CPUS];
static bool non_iram_int_disabled_flag[CONFIG_MP_MAX_NUM_CPUS];

/*
 * Inserts an item into vector_desc list so that the list is sorted
 * with an incrementing cpu.intno value.
 */
static void insert_vector_desc(struct vector_desc_t *to_insert)
{
	struct vector_desc_t *vd = vector_desc_head;
	struct vector_desc_t *prev = NULL;

	while (vd != NULL) {
		if (vd->cpu > to_insert->cpu) {
			break;
		}
		if (vd->cpu == to_insert->cpu && vd->intno >= to_insert->intno) {
			break;
		}
		prev = vd;
		vd = vd->next;
	}
	if ((vector_desc_head == NULL) || (prev == NULL)) {
		/* First item */
		to_insert->next = vd;
		vector_desc_head = to_insert;
	} else {
		prev->next = to_insert;
		to_insert->next = vd;
	}
}

/* Returns a vector_desc entry for an intno/cpu, or NULL if none exists. */
static struct vector_desc_t *find_desc_for_int(int intno, int cpu)
{
	struct vector_desc_t *vd = vector_desc_head;

	while (vd != NULL) {
		if (vd->cpu == cpu && vd->intno == intno) {
			break;
		}
		vd = vd->next;
	}
	return vd;
}

/*
 * Returns a vector_desc entry for an intno/cpu.
 * Either returns a preexisting one or allocates a new one and inserts
 * it into the list. Returns NULL on malloc fail.
 */
static struct vector_desc_t *get_desc_for_int(int intno, int cpu)
{
	struct vector_desc_t *vd = find_desc_for_int(intno, cpu);

	if (vd == NULL) {
		struct vector_desc_t *newvd = k_malloc(sizeof(struct vector_desc_t));

		if (newvd == NULL) {
			return NULL;
		}
		memset(newvd, 0, sizeof(struct vector_desc_t));
		newvd->intno = intno;
		newvd->cpu = cpu;
		insert_vector_desc(newvd);
		return newvd;
	} else {
		return vd;
	}
}

/*
 * Returns a vector_desc entry for an source, the cpu parameter is used
 * to tell GPIO_INT and GPIO_NMI from different CPUs
 */
static struct vector_desc_t *find_desc_for_source(int source, int cpu)
{
	struct vector_desc_t *vd = vector_desc_head;

	while (vd != NULL) {
		if (!(vd->flags & VECDESC_FL_SHARED)) {
			if (vd->source == source && cpu == vd->cpu) {
				break;
			}
		} else if (vd->cpu == cpu) {
			/* check only shared vds for the correct cpu, otherwise skip */
			bool found = false;
			struct shared_vector_desc_t *svd = vd->shared_vec_info;

			assert(svd != NULL);
			while (svd) {
				if (svd->source == source) {
					found = true;
					break;
				}
				svd = svd->next;
			}
			if (found) {
				break;
			}
		}
		vd = vd->next;
	}
	return vd;
}

void esp_intr_initialize(void)
{
	unsigned int num_cpus = arch_num_cpus();

	for (size_t i = 0; i < (ESP_INTC_INTS_NUM * num_cpus); ++i) {
		intr_alloc_table[i].handler = default_intr_handler;
		intr_alloc_table[i].arg = (void *)i;
	}
}

int esp_intr_mark_shared(int intno, int cpu, bool is_int_ram)
{
	if (intno >= ESP_INTC_INTS_NUM) {
		return -EINVAL;
	}
	if (cpu >= arch_num_cpus()) {
		return -EINVAL;
	}

	esp_intr_lock();
	struct vector_desc_t *vd = get_desc_for_int(intno, cpu);

	if (vd == NULL) {
		esp_intr_unlock();
		return -ENOMEM;
	}
	vd->flags = VECDESC_FL_SHARED;
	if (is_int_ram) {
		vd->flags |= VECDESC_FL_INIRAM;
	}
	esp_intr_unlock();

	return 0;
}

int esp_intr_reserve(int intno, int cpu)
{
	if (intno >= ESP_INTC_INTS_NUM) {
		return -EINVAL;
	}
	if (cpu >= arch_num_cpus()) {
		return -EINVAL;
	}

	esp_intr_lock();
	struct vector_desc_t *vd = get_desc_for_int(intno, cpu);

	if (vd == NULL) {
		esp_intr_unlock();
		return -ENOMEM;
	}
	vd->flags = VECDESC_FL_RESERVED;
	esp_intr_unlock();

	return 0;
}

/* Returns true if handler for interrupt is not the default unhandled interrupt handler */
static bool intr_has_handler(int intr, int cpu)
{
	bool r;

	r = intr_alloc_table[intr * CONFIG_MP_MAX_NUM_CPUS + cpu].handler != default_intr_handler;

	return r;
}

static bool is_vect_desc_usable(struct vector_desc_t *vd, int flags, int cpu, int force)
{
	/* Check if interrupt is not reserved by design */
	int x = vd->intno;

	if (interrupt_controller_hal_get_cpu_desc_flags(x, cpu) == INTDESC_RESVD) {
		INTC_LOG("....Unusable: reserved");
		return false;
	}
	if (interrupt_controller_hal_get_cpu_desc_flags(x, cpu) == INTDESC_SPECIAL && force == -1) {
		INTC_LOG("....Unusable: special-purpose int");
		return false;
	}
	/* Check if the interrupt level is acceptable */
	if (!(flags & (1 << interrupt_controller_hal_get_level(x)))) {
		INTC_LOG("....Unusable: incompatible level");
		return false;
	}
	/* check if edge/level type matches what we want */
	if (((flags & ESP_INTR_FLAG_EDGE) &&
		(interrupt_controller_hal_get_type(x) == INTTP_LEVEL)) ||
		(((!(flags & ESP_INTR_FLAG_EDGE)) &&
		(interrupt_controller_hal_get_type(x) == INTTP_EDGE)))) {
		INTC_LOG("....Unusable: incompatible trigger type");
		return false;
	}
	/* check if interrupt is reserved at runtime */
	if (vd->flags & VECDESC_FL_RESERVED) {
		INTC_LOG("....Unusable: reserved at runtime.");
		return false;
	}

	/* Ints can't be both shared and non-shared. */
	assert(!((vd->flags & VECDESC_FL_SHARED) && (vd->flags & VECDESC_FL_NONSHARED)));
	/* check if interrupt already is in use by a non-shared interrupt */
	if (vd->flags & VECDESC_FL_NONSHARED) {
		INTC_LOG("....Unusable: already in (non-shared) use.");
		return false;
	}
	/* check shared interrupt flags */
	if (vd->flags & VECDESC_FL_SHARED) {
		if (flags & ESP_INTR_FLAG_SHARED) {
			bool in_iram_flag = ((flags & ESP_INTR_FLAG_IRAM) != 0);
			bool desc_in_iram_flag = ((vd->flags & VECDESC_FL_INIRAM) != 0);
			/*
			 * Bail out if int is shared, but iram property
			 * doesn't match what we want.
			 */
			if ((vd->flags & VECDESC_FL_SHARED) &&
				(desc_in_iram_flag != in_iram_flag)) {
				INTC_LOG("....Unusable: shared but iram prop doesn't match");
				return false;
			}
		} else {
			/*
			 * We need an unshared IRQ; can't use shared ones;
			 * bail out if this is shared.
			 */
			INTC_LOG("...Unusable: int is shared, we need non-shared.");
			return false;
		}
	} else if (intr_has_handler(x, cpu)) {
		/* Check if interrupt already is allocated by set_interrupt_handler */
		INTC_LOG("....Unusable: already allocated");
		return false;
	}

	return true;
}

/*
 * Locate a free interrupt compatible with the flags given.
 * The 'force' argument can be -1, or 0-31 to force checking a certain interrupt.
 * When a CPU is forced, the INTDESC_SPECIAL marked interrupts are also accepted.
 */
static int get_available_int(int flags, int cpu, int force, int source)
{
	int x;
	int best = -1;
	int best_level = 9;
	int best_shared_ct = INT_MAX;
	/* Default vector desc, for vectors not in the linked list */
	struct vector_desc_t empty_vect_desc;

	memset(&empty_vect_desc, 0, sizeof(struct vector_desc_t));


	/* Level defaults to any low/med interrupt */
	if (!(flags & ESP_INTR_FLAG_LEVELMASK)) {
		flags |= ESP_INTR_FLAG_LOWMED;
	}

	INTC_LOG("%s: try to find existing. Cpu: %d, Source: %d", __func__, cpu, source);
	struct vector_desc_t *vd = find_desc_for_source(source, cpu);

	if (vd) {
		/* if existing vd found, don't need to search any more. */
		INTC_LOG("%s: existing vd found. intno: %d", __func__, vd->intno);
		if (force != -1 && force != vd->intno) {
			INTC_LOG("%s: intr forced but not match existing. "
				 "existing intno: %d, force: %d", __func__, vd->intno, force);
		} else if (!is_vect_desc_usable(vd, flags, cpu, force)) {
			INTC_LOG("%s: existing vd invalid.", __func__);
		} else {
			best = vd->intno;
		}
		return best;
	}
	if (force != -1) {
		INTC_LOG("%s: try to find force. "
			 "Cpu: %d, Source: %d, Force: %d", __func__, cpu, source, force);
		/* if force assigned, don't need to search any more. */
		vd = find_desc_for_int(force, cpu);
		if (vd == NULL) {
			/* if existing vd not found, just check the default state for the intr. */
			empty_vect_desc.intno = force;
			vd = &empty_vect_desc;
		}
		if (is_vect_desc_usable(vd, flags, cpu, force)) {
			best = vd->intno;
		} else {
			INTC_LOG("%s: forced vd invalid.", __func__);
		}
		return best;
	}

	INTC_LOG("%s: start looking. Current cpu: %d", __func__, cpu);
	/* No allocated handlers as well as forced intr, iterate over the 32 possible interrupts */
	for (x = 0; x < ESP_INTC_INTS_NUM; x++) {
		/* Grab the vector_desc for this vector. */
		vd = find_desc_for_int(x, cpu);
		if (vd == NULL) {
			empty_vect_desc.intno = x;
			vd = &empty_vect_desc;
		}

		INTC_LOG("Int %d reserved %d level %d %s hasIsr %d",
			 x,
			 interrupt_controller_hal_get_cpu_desc_flags(x, cpu) == INTDESC_RESVD,
			 interrupt_controller_hal_get_level(x),
			 interrupt_controller_hal_get_type(x) == INTTP_LEVEL ? "LEVEL" : "EDGE",
			 intr_has_handler(x, cpu));

		if (!is_vect_desc_usable(vd, flags, cpu, force)) {
			continue;
		}

		if (flags & ESP_INTR_FLAG_SHARED) {
			/* We're allocating a shared int. */

			/* See if int already is used as a shared interrupt. */
			if (vd->flags & VECDESC_FL_SHARED) {
				/*
				 * We can use this already-marked-as-shared interrupt. Count the
				 * already attached isrs in order to see how useful it is.
				 */
				int no = 0;
				struct shared_vector_desc_t *svdesc = vd->shared_vec_info;

				while (svdesc != NULL) {
					no++;
					svdesc = svdesc->next;
				}
				if (no < best_shared_ct ||
					best_level > interrupt_controller_hal_get_level(x)) {
					/*
					 * Seems like this shared vector is both okay and has
					 * the least amount of ISRs already attached to it.
					 */
					best = x;
					best_shared_ct = no;
					best_level = interrupt_controller_hal_get_level(x);
					INTC_LOG("...int %d more usable as a shared int: "
						 "has %d existing vectors", x, no);
				} else {
					INTC_LOG("...worse than int %d", best);
				}
			} else {
				if (best == -1) {
					/*
					 * We haven't found a feasible shared interrupt yet.
					 * This one is still free and usable, even if not
					 * marked as shared.
					 * Remember it in case we don't find any other shared
					 * interrupt that qualifies.
					 */
					if (best_level > interrupt_controller_hal_get_level(x)) {
						best = x;
						best_level = interrupt_controller_hal_get_level(x);
						INTC_LOG("...int %d usable as new shared int", x);
					}
				} else {
					INTC_LOG("...already have a shared int");
				}
			}
		} else {
			/*
			 * Seems this interrupt is feasible. Select it and break out of the loop
			 * No need to search further.
			 */
			if (best_level > interrupt_controller_hal_get_level(x)) {
				best = x;
				best_level = interrupt_controller_hal_get_level(x);
			} else {
				INTC_LOG("...worse than int %d", best);
			}
		}
	}
	INTC_LOG("%s: using int %d", __func__, best);

	/*
	 * By now we have looked at all potential interrupts and
	 * hopefully have selected the best one in best.
	 */
	return best;
}

/* Common shared isr handler. Chain-call all ISRs. */
static void IRAM_ATTR shared_intr_isr(void *arg)
{
	struct vector_desc_t *vd = (struct vector_desc_t *)arg;
	struct shared_vector_desc_t *sh_vec = vd->shared_vec_info;

	esp_intr_lock();
	while (sh_vec) {
		if (!sh_vec->disabled) {
			if (!(sh_vec->statusreg) || (*sh_vec->statusreg & sh_vec->statusmask)) {
				sh_vec->isr(sh_vec->arg);
			}
		}
		sh_vec = sh_vec->next;
	}
	esp_intr_unlock();
}

int esp_intr_alloc_intrstatus(int source,
			      int flags,
			      uint32_t intrstatusreg,
			      uint32_t intrstatusmask,
			      intr_handler_t handler,
			      void *arg,
			      struct intr_handle_data_t **ret_handle)
{
	struct intr_handle_data_t *ret = NULL;
	int force = -1;

	INTC_LOG("%s (cpu %d): checking args", __func__, esp_core_id());
	/* Shared interrupts should be level-triggered. */
	if ((flags & ESP_INTR_FLAG_SHARED) && (flags & ESP_INTR_FLAG_EDGE)) {
		return -EINVAL;
	}
	/* You can't set an handler / arg for a non-C-callable interrupt. */
	if ((flags & ESP_INTR_FLAG_HIGH) && (handler)) {
		return -EINVAL;
	}
	/* Shared ints should have handler and non-processor-local source */
	if ((flags & ESP_INTR_FLAG_SHARED) && (!handler || source < 0)) {
		return -EINVAL;
	}
	/* Statusreg should have a mask */
	if (intrstatusreg && !intrstatusmask) {
		return -EINVAL;
	}
	/*
	 * If the ISR is marked to be IRAM-resident, the handler must not be in the cached region
	 * If we are to allow placing interrupt handlers into the 0x400c0000—0x400c2000 region,
	 * we need to make sure the interrupt is connected to the CPU0.
	 * CPU1 does not have access to the RTC fast memory through this region.
	 */
	if ((flags & ESP_INTR_FLAG_IRAM) &&
	    (ptrdiff_t) handler >= SOC_RTC_IRAM_HIGH &&
	    (ptrdiff_t) handler < SOC_RTC_DATA_LOW) {
		return -EINVAL;
	}

	/*
	 * Default to prio 1 for shared interrupts.
	 * Default to prio 1, 2 or 3 for non-shared interrupts.
	 */
	if ((flags & ESP_INTR_FLAG_LEVELMASK) == 0) {
		if (flags & ESP_INTR_FLAG_SHARED) {
			flags |= ESP_INTR_FLAG_LEVEL1;
		} else {
			flags |= ESP_INTR_FLAG_LOWMED;
		}
	}
	INTC_LOG("%s (cpu %d): Args okay."
		"Resulting flags 0x%X", __func__, esp_core_id(), flags);

	/*
	 * Check 'special' interrupt sources. These are tied to one specific
	 * interrupt, so we have to force get_available_int to only look at that.
	 */
	switch (source) {
	case ETS_INTERNAL_TIMER0_INTR_SOURCE:
		force = ETS_INTERNAL_TIMER0_INTR_NO;
		break;
	case ETS_INTERNAL_TIMER1_INTR_SOURCE:
		force = ETS_INTERNAL_TIMER1_INTR_NO;
		break;
	case ETS_INTERNAL_TIMER2_INTR_SOURCE:
		force = ETS_INTERNAL_TIMER2_INTR_NO;
		break;
	case ETS_INTERNAL_SW0_INTR_SOURCE:
		force = ETS_INTERNAL_SW0_INTR_NO;
		break;
	case ETS_INTERNAL_SW1_INTR_SOURCE:
		force = ETS_INTERNAL_SW1_INTR_NO;
		break;
	case ETS_INTERNAL_PROFILING_INTR_SOURCE:
		force = ETS_INTERNAL_PROFILING_INTR_NO;
		break;
	default:
		break;
	}

	/* Allocate a return handle. If we end up not needing it, we'll free it later on. */
	ret = k_malloc(sizeof(struct intr_handle_data_t));
	if (ret == NULL) {
		return -ENOMEM;
	}

	esp_intr_lock();
	int cpu = esp_core_id();
	/* See if we can find an interrupt that matches the flags. */
	int intr = get_available_int(flags, cpu, force, source);

	if (intr == -1) {
		/* None found. Bail out. */
		esp_intr_unlock();
		k_free(ret);
		return -ENODEV;
	}
	/* Get an int vector desc for int. */
	struct vector_desc_t *vd = get_desc_for_int(intr, cpu);

	if (vd == NULL) {
		esp_intr_unlock();
		k_free(ret);
		return -ENOMEM;
	}

	/* Allocate that int! */
	if (flags & ESP_INTR_FLAG_SHARED) {
		/* Populate vector entry and add to linked list. */
		struct shared_vector_desc_t *sv = k_malloc(sizeof(struct shared_vector_desc_t));

		if (sv == NULL) {
			esp_intr_unlock();
			k_free(ret);
			return -ENOMEM;
		}
		memset(sv, 0, sizeof(struct shared_vector_desc_t));
		sv->statusreg = (uint32_t *)intrstatusreg;
		sv->statusmask = intrstatusmask;
		sv->isr = handler;
		sv->arg = arg;
		sv->next = vd->shared_vec_info;
		sv->source = source;
		sv->disabled = 0;
		vd->shared_vec_info = sv;
		vd->flags |= VECDESC_FL_SHARED;
		/* (Re-)set shared isr handler to new value. */
		set_interrupt_handler(intr, shared_intr_isr, vd);
	} else {
		/* Mark as unusable for other interrupt sources. This is ours now! */
		vd->flags = VECDESC_FL_NONSHARED;
		if (handler) {
			set_interrupt_handler(intr, handler, arg);
		}
		if (flags & ESP_INTR_FLAG_EDGE) {
			xthal_set_intclear(1 << intr);
		}
		vd->source = source;
	}
	if (flags & ESP_INTR_FLAG_IRAM) {
		vd->flags |= VECDESC_FL_INIRAM;
		non_iram_int_mask[cpu] &= ~(1 << intr);
	} else {
		vd->flags &= ~VECDESC_FL_INIRAM;
		non_iram_int_mask[cpu] |= (1 << intr);
	}
	if (source >= 0) {
		intr_matrix_set(cpu, source, intr);
	}

	/* Fill return handle data. */
	ret->vector_desc = vd;
	ret->shared_vector_desc = vd->shared_vec_info;

	/* Enable int at CPU-level; */
	irq_enable(intr);

	/*
	 * If interrupt has to be started disabled, do that now; ints won't be enabled for
	 * real until the end of the critical section.
	 */
	if (flags & ESP_INTR_FLAG_INTRDISABLED) {
		esp_intr_disable(ret);
	}

	esp_intr_unlock();

	/* Fill return handle if needed, otherwise free handle. */
	if (ret_handle != NULL) {
		*ret_handle = ret;
	} else {
		k_free(ret);
	}

	LOG_DBG("Connected src %d to int %d (cpu %d)", source, intr, cpu);
	return 0;
}

int esp_intr_alloc(int source,
		int flags,
		intr_handler_t handler,
		void *arg,
		struct intr_handle_data_t **ret_handle)
{
	/*
	 * As an optimization, we can create a table with the possible interrupt status
	 * registers and masks for every single source there is. We can then add code here to
	 * look up an applicable value and pass that to the esp_intr_alloc_intrstatus function.
	 */
	return esp_intr_alloc_intrstatus(source, flags, 0, 0, handler, arg, ret_handle);
}

int IRAM_ATTR esp_intr_set_in_iram(struct intr_handle_data_t *handle, bool is_in_iram)
{
	if (!handle) {
		return -EINVAL;
	}
	struct vector_desc_t *vd = handle->vector_desc;

	if (vd->flags & VECDESC_FL_SHARED) {
		return -EINVAL;
	}
	esp_intr_lock();
	uint32_t mask = (1 << vd->intno);

	if (is_in_iram) {
		vd->flags |= VECDESC_FL_INIRAM;
		non_iram_int_mask[vd->cpu] &= ~mask;
	} else {
		vd->flags &= ~VECDESC_FL_INIRAM;
		non_iram_int_mask[vd->cpu] |= mask;
	}
	esp_intr_unlock();
	return 0;
}

int esp_intr_free(struct intr_handle_data_t *handle)
{
	bool free_shared_vector = false;

	if (!handle) {
		return -EINVAL;
	}

	esp_intr_lock();
	esp_intr_disable(handle);
	if (handle->vector_desc->flags & VECDESC_FL_SHARED) {
		/* Find and kill the shared int */
		struct shared_vector_desc_t *svd = handle->vector_desc->shared_vec_info;
		struct shared_vector_desc_t *prevsvd = NULL;

		assert(svd); /* should be something in there for a shared int */
		while (svd != NULL) {
			if (svd == handle->shared_vector_desc) {
				/* Found it. Now kill it. */
				if (prevsvd) {
					prevsvd->next = svd->next;
				} else {
					handle->vector_desc->shared_vec_info = svd->next;
				}
				k_free(svd);
				break;
			}
			prevsvd = svd;
			svd = svd->next;
		}
		/* If nothing left, disable interrupt. */
		if (handle->vector_desc->shared_vec_info == NULL) {
			free_shared_vector = true;
		}
		INTC_LOG("%s: Deleting shared int: %s. "
			"Shared int is %s", __func__, svd ? "not found or last one" : "deleted",
			free_shared_vector ? "empty now." : "still in use");
	}

	if ((handle->vector_desc->flags & VECDESC_FL_NONSHARED) || free_shared_vector) {
		INTC_LOG("%s: Disabling int, killing handler", __func__);
		/* Reset to normal handler */
		set_interrupt_handler(handle->vector_desc->intno,
				      default_intr_handler,
				      (void *)((int)handle->vector_desc->intno));
		/*
		 * Theoretically, we could free the vector_desc... not sure if that's worth the
		 * few bytes of memory we save.(We can also not use the same exit path for empty
		 * shared ints anymore if we delete the desc.) For now, just mark it as free.
		 */
		handle->vector_desc->flags &= !(VECDESC_FL_NONSHARED | VECDESC_FL_RESERVED);
		/* Also kill non_iram mask bit. */
		non_iram_int_mask[handle->vector_desc->cpu] &= ~(1 << (handle->vector_desc->intno));
	}
	esp_intr_unlock();
	k_free(handle);
	return 0;
}

int esp_intr_get_intno(struct intr_handle_data_t *handle)
{
	return handle->vector_desc->intno;
}

int esp_intr_get_cpu(struct intr_handle_data_t *handle)
{
	return handle->vector_desc->cpu;
}

/**
 * Interrupt disabling strategy:
 * If the source is >=0 (meaning a muxed interrupt), we disable it by muxing the interrupt to a
 * non-connected interrupt. If the source is <0 (meaning an internal, per-cpu interrupt).
 * This allows us to, for the muxed CPUs, disable an int from
 * the other core. It also allows disabling shared interrupts.
 */

/*
 * Muxing an interrupt source to interrupt 6, 7, 11, 15, 16 or 29
 * cause the interrupt to effectively be disabled.
 */
#define INT_MUX_DISABLED_INTNO 6

int IRAM_ATTR esp_intr_enable(struct intr_handle_data_t *handle)
{
	if (!handle) {
		return -EINVAL;
	}
	esp_intr_lock();
	int source;

	if (handle->shared_vector_desc) {
		handle->shared_vector_desc->disabled = 0;
		source = handle->shared_vector_desc->source;
	} else {
		source = handle->vector_desc->source;
	}
	if (source >= 0) {
		/* Disabled using int matrix; re-connect to enable */
		intr_matrix_set(handle->vector_desc->cpu, source, handle->vector_desc->intno);
	} else {
		/* Re-enable using cpu int ena reg */
		if (handle->vector_desc->cpu != esp_core_id()) {
			return -EINVAL; /* Can only enable these ints on this cpu */
		}
		irq_enable(handle->vector_desc->intno);
	}
	esp_intr_unlock();
	return 0;
}

int IRAM_ATTR esp_intr_disable(struct intr_handle_data_t *handle)
{
	if (!handle) {
		return -EINVAL;
	}
	esp_intr_lock();
	int source;
	bool disabled = 1;

	if (handle->shared_vector_desc) {
		handle->shared_vector_desc->disabled = 1;
		source = handle->shared_vector_desc->source;

		struct shared_vector_desc_t *svd = handle->vector_desc->shared_vec_info;

		assert(svd != NULL);
		while (svd) {
			if (svd->source == source && svd->disabled == 0) {
				disabled = 0;
				break;
			}
			svd = svd->next;
		}
	} else {
		source = handle->vector_desc->source;
	}

	if (source >= 0) {
		if (disabled) {
			/* Disable using int matrix */
			intr_matrix_set(handle->vector_desc->cpu, source, INT_MUX_DISABLED_INTNO);
		}
	} else {
		/* Disable using per-cpu regs */
		if (handle->vector_desc->cpu != esp_core_id()) {
			esp_intr_unlock();
			return -EINVAL; /* Can only enable these ints on this cpu */
		}
		irq_disable(handle->vector_desc->intno);
	}
	esp_intr_unlock();
	return 0;
}


void IRAM_ATTR esp_intr_noniram_disable(void)
{
	int oldint;
	int cpu = esp_core_id();
	int non_iram_ints = ~non_iram_int_mask[cpu];

	if (non_iram_int_disabled_flag[cpu]) {
		abort();
	}
	non_iram_int_disabled_flag[cpu] = true;
	oldint = interrupt_controller_hal_read_interrupt_mask();
	interrupt_controller_hal_disable_interrupts(non_iram_ints);
	/* Save which ints we did disable */
	non_iram_int_disabled[cpu] = oldint & non_iram_ints;
}

void IRAM_ATTR esp_intr_noniram_enable(void)
{
	int cpu = esp_core_id();
	int non_iram_ints = non_iram_int_disabled[cpu];

	if (!non_iram_int_disabled_flag[cpu]) {
		abort();
	}
	non_iram_int_disabled_flag[cpu] = false;
	interrupt_controller_hal_enable_interrupts(non_iram_ints);
}
