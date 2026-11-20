/*
 * Copyright (c) 2021-2025 EPAM Systems
 * Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xen_xen

#include <xen/public/xen.h>
#include <xen/public/event_channel.h>

#include <zephyr/arch/arm64/hypercall.h>
#include <zephyr/xen/events.h>
#include <zephyr/sys/barrier.h>

#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(xen_events);

extern shared_info_t *HYPERVISOR_shared_info;

/* Zephyr-owned callback state for one Xen event-channel port. */
struct event_channel_handle {
	/*
	 * Protects callback/private-data replacement and the sticky missed-event
	 * flag.
	 */
	struct k_spinlock lock;
	evtchn_cb_t cb;
	void *priv;
	bool missed;
};
typedef struct event_channel_handle evtchn_handle_t;

/* Per-port Zephyr dispatch state. */
static evtchn_handle_t event_channels[EVTCHN_2L_NR_CHANNELS];

/* Current "callback port plus one" for each CPU; zero means no active callback. */
static uint32_t active_callback_port[CONFIG_MP_MAX_NUM_CPUS];

#define EVTCHN_WORD_BITS (8 * sizeof(xen_ulong_t))

static void empty_callback(void *data)
{
	ARG_UNUSED(data);
}

/*
 * Xen can update shared evtchn_pending[] and evtchn_mask[] words concurrently,
 * so reads and writes use atomic helpers instead of sys_bitfield_* load/store
 * helpers.
 */
static xen_ulong_t shared_event_bit(evtchn_port_t port)
{
	return ((xen_ulong_t)1) << (port % EVTCHN_WORD_BITS);
}

static xen_ulong_t shared_event_word(const xen_ulong_t *bitmap, uint32_t word)
{
	return __atomic_load_n(&bitmap[word], __ATOMIC_SEQ_CST);
}

static void set_shared_event_bit(xen_ulong_t *bitmap, evtchn_port_t port)
{
	uint32_t word = port / EVTCHN_WORD_BITS;
	xen_ulong_t bit = shared_event_bit(port);

	(void)__atomic_fetch_or(&bitmap[word], bit, __ATOMIC_SEQ_CST);
}

static void clear_shared_event_bit(xen_ulong_t *bitmap, evtchn_port_t port)
{
	uint32_t word = port / EVTCHN_WORD_BITS;
	xen_ulong_t bit = shared_event_bit(port);

	(void)__atomic_fetch_and(&bitmap[word], ~bit, __ATOMIC_SEQ_CST);
}

static bool read_and_set_shared_event_bit(xen_ulong_t *bitmap, evtchn_port_t port)
{
	uint32_t word = port / EVTCHN_WORD_BITS;
	xen_ulong_t bit = shared_event_bit(port);

	return (__atomic_fetch_or(&bitmap[word], bit, __ATOMIC_SEQ_CST) & bit) != 0;
}

static bool read_and_clear_shared_event_bit(xen_ulong_t *bitmap, evtchn_port_t port)
{
	uint32_t word = port / EVTCHN_WORD_BITS;
	xen_ulong_t bit = shared_event_bit(port);

	return (__atomic_fetch_and(&bitmap[word], ~bit, __ATOMIC_SEQ_CST) & bit) != 0;
}

static void reset_event_channel_state(evtchn_port_t port)
{
	event_channels[port].cb = empty_callback;
	event_channels[port].priv = NULL;
	event_channels[port].missed = false;
}

int alloc_unbound_event_channel(domid_t remote_dom)
{
	int rc;
	struct evtchn_alloc_unbound alloc = {
		.dom = DOMID_SELF,
		.remote_dom = remote_dom,
	};

	rc = HYPERVISOR_event_channel_op(EVTCHNOP_alloc_unbound, &alloc);
	if (rc == 0) {
		rc = alloc.port;
	}

	return rc;
}

#ifdef CONFIG_XEN_DOM0
int alloc_unbound_event_channel_dom0(domid_t dom, domid_t remote_dom)
{
	int rc;
	struct evtchn_alloc_unbound alloc = {
		.dom = dom,
		.remote_dom = remote_dom,
	};

	rc = HYPERVISOR_event_channel_op(EVTCHNOP_alloc_unbound, &alloc);
	if (rc == 0) {
		rc = alloc.port;
	}

	return rc;
}
#endif /* CONFIG_XEN_DOM0 */

int bind_interdomain_event_channel(domid_t remote_dom, evtchn_port_t remote_port,
		evtchn_cb_t cb, void *data)
{
	int rc;
	int close_rc;
	struct evtchn_bind_interdomain bind = {
		.remote_dom = remote_dom,
		.remote_port = remote_port,
	};

	rc = HYPERVISOR_event_channel_op(EVTCHNOP_bind_interdomain, &bind);
	if (rc < 0) {
		return rc;
	}

	rc = bind_event_channel(bind.local_port, cb, data);
	if (rc < 0) {
		return rc;
	}

	rc = unmask_event_channel(bind.local_port);
	if (rc < 0) {
		close_rc = evtchn_close(bind.local_port);
		if (close_rc != 0) {
			LOG_ERR("%s: close evtchn #%u after unmask failure failed: %d",
				__func__, bind.local_port, close_rc);
		}

		return rc;
	}

	return bind.local_port;
}

int evtchn_status(evtchn_status_t *status)
{
	return HYPERVISOR_event_channel_op(EVTCHNOP_status, status);
}

/* The caller holds event_channels[port].lock. */
static int evtchn_close_locked(evtchn_port_t port)
{
	shared_info_t *s = HYPERVISOR_shared_info;
	struct evtchn_close close = {
		.port = port,
	};
	struct evtchn_unmask unmask = {
		.port = port,
	};
	evtchn_cb_t cb;
	void *priv;
	bool missed;
	bool was_masked;
	bool was_pending;
	int restore_rc;
	int rc;

	cb = event_channels[port].cb;
	priv = event_channels[port].priv;
	missed = event_channels[port].missed;

	/*
	 * Quiesce all Zephyr-owned state before Xen can make this port number
	 * available to another CPU. No old-port cleanup may happen after a
	 * successful EVTCHNOP_close because it could then modify a newly allocated
	 * channel that happens to reuse the same number.
	 */
	was_masked = read_and_set_shared_event_bit(s->evtchn_mask, port);
	was_pending = read_and_clear_shared_event_bit(s->evtchn_pending, port);
	reset_event_channel_state(port);

	rc = HYPERVISOR_event_channel_op(EVTCHNOP_close, &close);
	if (rc != 0) {
		event_channels[port].cb = cb;
		event_channels[port].priv = priv;
		event_channels[port].missed = missed;

		if (was_pending) {
			set_shared_event_bit(s->evtchn_pending, port);
		}

		if (!was_masked) {
			restore_rc = HYPERVISOR_event_channel_op(EVTCHNOP_unmask, &unmask);
			if (restore_rc != 0) {
				LOG_ERR("%s: restore evtchn #%u unmask failed: %d",
					__func__, port, restore_rc);
			}
		}
	}

	return rc;
}

int evtchn_close(evtchn_port_t port)
{
	unsigned int irq_key;
	uint32_t active_port;
	uint32_t cpu;
	k_spinlock_key_t key;
	int rc;

	if (port >= EVTCHN_2L_NR_CHANNELS) {
		return -EINVAL;
	}

	/* Keep the CPU ID stable until the callback-owner bypass is complete. */
	irq_key = arch_irq_lock();
	cpu = arch_curr_cpu()->id;
	active_port = __atomic_load_n(&active_callback_port[cpu], __ATOMIC_RELAXED);

	/* A callback already owns this port's lock through process_event(). */
	if (active_port == port + 1U) {
		rc = evtchn_close_locked(port);
		arch_irq_unlock(irq_key);
		return rc;
	}

	/*
	 * A callback holds its own port lock until it returns. Taking another
	 * port lock here would allow callbacks on two CPUs to wait on each other.
	 */
	if (active_port != 0U) {
		arch_irq_unlock(irq_key);
		return -EDEADLK;
	}
	arch_irq_unlock(irq_key);

	key = k_spin_lock(&event_channels[port].lock);
	rc = evtchn_close_locked(port);
	k_spin_unlock(&event_channels[port].lock, key);

	return rc;
}

int evtchn_set_priority(evtchn_port_t port, uint32_t priority)
{
	struct evtchn_set_priority set = {
		.port = port,
		.priority = priority,
	};

	return HYPERVISOR_event_channel_op(EVTCHNOP_set_priority, &set);
}

int notify_evtchn(evtchn_port_t port)
{
	struct evtchn_send send;

	__ASSERT(port < EVTCHN_2L_NR_CHANNELS,
		"%s: trying to send notify for invalid evtchn #%u\n",
		__func__, port);

	send.port = port;

	return HYPERVISOR_event_channel_op(EVTCHNOP_send, &send);
}

int bind_event_channel(evtchn_port_t port, evtchn_cb_t cb, void *data)
{
	k_spinlock_key_t key;

	__ASSERT(port < EVTCHN_2L_NR_CHANNELS,
		"%s: trying to bind invalid evtchn #%u\n",
		__func__, port);
	__ASSERT(cb != NULL, "%s: NULL callback for evtchn #%u\n",
		__func__, port);

	key = k_spin_lock(&event_channels[port].lock);
	event_channels[port].priv = data;
	event_channels[port].cb = cb;
	k_spin_unlock(&event_channels[port].lock, key);

	return 0;
}

int unbind_event_channel(evtchn_port_t port)
{
	k_spinlock_key_t key;

	__ASSERT(port < EVTCHN_2L_NR_CHANNELS,
		"%s: trying to unbind invalid evtchn #%u\n",
		__func__, port);

	key = k_spin_lock(&event_channels[port].lock);
	reset_event_channel_state(port);
	k_spin_unlock(&event_channels[port].lock, key);

	return 0;
}

int get_missed_events(evtchn_port_t port)
{
	k_spinlock_key_t key;
	bool missed;

	__ASSERT(port < EVTCHN_2L_NR_CHANNELS,
		"%s: trying to get missed event from invalid port #%u\n",
		__func__, port);

	key = k_spin_lock(&event_channels[port].lock);
	missed = event_channels[port].missed;
	event_channels[port].missed = false;
	k_spin_unlock(&event_channels[port].lock, key);

	if (missed) {
		return 1;
	}

	return 0;
}

int mask_event_channel(evtchn_port_t port)
{
	shared_info_t *s = HYPERVISOR_shared_info;

	__ASSERT(port < EVTCHN_2L_NR_CHANNELS,
		"%s: trying to mask invalid evtchn #%u\n",
		__func__, port);

	set_shared_event_bit(s->evtchn_mask, port);

	return 0;
}

int unmask_event_channel(evtchn_port_t port)
{
	struct evtchn_unmask unmask = {
		.port = port,
	};

	__ASSERT(port < EVTCHN_2L_NR_CHANNELS,
		"%s: trying to unmask invalid evtchn #%u\n",
		__func__, port);

	return HYPERVISOR_event_channel_op(EVTCHNOP_unmask, &unmask);
}

void clear_event_channel(evtchn_port_t port)
{
	shared_info_t *s = HYPERVISOR_shared_info;

	__ASSERT(port < EVTCHN_2L_NR_CHANNELS,
		"%s: trying to clear invalid evtchn #%u\n",
		__func__, port);

	clear_shared_event_bit(s->evtchn_pending, port);
}

/*
 * Called while the ISR is draining the current CPU's evtchn_pending_sel.
 * pos selects one word in the domain-wide pending bitmap. The returned word has
 * one bit set for each unmasked port in that word that still needs dispatch.
 */
static inline xen_ulong_t get_pending_events(uint32_t pos)
{
	shared_info_t *s = HYPERVISOR_shared_info;
	xen_ulong_t pending = shared_event_word(s->evtchn_pending, pos);
	xen_ulong_t mask = shared_event_word(s->evtchn_mask, pos);

	return pending & ~mask;
}

/*
 * Dispatch one pending port. If the port has no bound callback, record the
 * missed event so the driver can query it later via get_missed_events().
 */
static void process_event(evtchn_port_t port)
{
	uint32_t cpu = arch_curr_cpu()->id;
	k_spinlock_key_t key;

	clear_event_channel(port);

	key = k_spin_lock(&event_channels[port].lock);
	if (event_channels[port].cb == empty_callback) {
		event_channels[port].missed = true;
	} else {
		/*
		 * Keep event_channels[port].lock held while invoking the handler, so
		 * bind/unbind cannot replace or remove callback state until an
		 * in-flight callback returns. Event-channel callbacks run in IRQ
		 * context and must not call APIs that take this lock for the same
		 * port, except evtchn_close(), which detects this path and reuses the
		 * lock already held here.
		 */
		__atomic_store_n(&active_callback_port[cpu], port + 1U, __ATOMIC_RELAXED);
		event_channels[port].cb(event_channels[port].priv);
		__atomic_store_n(&active_callback_port[cpu], 0U, __ATOMIC_RELAXED);
	}

	k_spin_unlock(&event_channels[port].lock, key);
}

static void events_isr(void *data)
{
	ARG_UNUSED(data);

	/* Needed for 2-level unwrapping */
	xen_ulong_t pos_selector;   /* bits are positions in pending array */
	xen_ulong_t events_pending; /* bits - events in pos_selector element */
	uint32_t pos_index, event_index; /* bit indexes */

	evtchn_port_t port; /* absolute event index */

	/* TODO: SMP? XEN_LEGACY_MAX_VCPUS == 1*/
	vcpu_info_t *vcpu = &HYPERVISOR_shared_info->vcpu_info[0];

	/*
	 * Need to set it to 0 /before/ checking for pending work, thus
	 * avoiding a set-and-check race (check struct vcpu_info_t)
	 */
	vcpu->evtchn_upcall_pending = 0;

	barrier_dmem_fence_full();

	/* Can not use system atomic_t/atomic_set() due to 32-bit casting */
	pos_selector = __atomic_exchange_n(&vcpu->evtchn_pending_sel,
					0, __ATOMIC_SEQ_CST);

	while (pos_selector) {
		/* Find first position, clear it in selector and process */
		pos_index = __builtin_ffsl(pos_selector) - 1;
		pos_selector &= ~(((xen_ulong_t) 1) << pos_index);

		/* Find all active evtchn on selected position */
		while ((events_pending = get_pending_events(pos_index)) != 0) {
			event_index =  __builtin_ffsl(events_pending) - 1;
			events_pending &= (((xen_ulong_t) 1) << event_index);

			port = (pos_index * EVTCHN_WORD_BITS) + event_index;
			process_event(port);
		}
	}
}

int xen_events_init(void)
{
	int i;

	if (!HYPERVISOR_shared_info) {
		/* shared info was not mapped */
		LOG_ERR("%s: shared_info - NULL, can't setup events\n", __func__);
		return -EINVAL;
	}

	/* bind all ports with default callback */
	for (i = 0; i < EVTCHN_2L_NR_CHANNELS; i++) {
		reset_event_channel_state(i);
	}

	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 0, irq),
		DT_INST_IRQ_BY_IDX(0, 0, priority), events_isr,
		NULL, DT_INST_IRQ_BY_IDX(0, 0, flags));

	irq_enable(DT_INST_IRQ_BY_IDX(0, 0, irq));

	LOG_INF("%s: events inited\n", __func__);
	return 0;
}
