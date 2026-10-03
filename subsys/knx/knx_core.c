/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * KNX stack initialisation.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/knx/knx_pkt.h>
#include <zephyr/logging/log.h>
#include "layer2_data_link.h"
#include "layer4_transport.h"
#include "object_application_program.h"
#include "object_interface_program.h"
#include "object_device.h"
#include "object_memory.h"
#include "object_interface.h"
#include "object_group_table.h"

LOG_MODULE_REGISTER(knx_core, CONFIG_KNX_STACK_LOG_LEVEL);

/* -------- Packet slab -------- */

K_MEM_SLAB_DEFINE(knx_pkt_slab, sizeof(struct knx_pkt), CONFIG_KNX_PKT_POOL_SIZE, 4);

/* -------- Application work-queue (L3..L7 RX dispatch + L4 timers) -------- */

K_THREAD_STACK_DEFINE(knx_app_wq_stack, CONFIG_KNX_APP_WQ_STACK_SIZE);
struct k_work_q knx_app_wq;

static int knx_init(void)
{
	k_work_queue_start(&knx_app_wq, knx_app_wq_stack, K_THREAD_STACK_SIZEOF(knx_app_wq_stack),
			   CONFIG_KNX_APP_WQ_PRIO, NULL);
	k_thread_name_set(knx_app_wq.thread_id, "knx_app_wq");

	memory_read();

	/*
	 * Load State now lives inside __memory (persisted in
	 * NVS), so memory_read() already restored the correct value for every
	 * table object — whether that is a genuinely persisted state or the
	 * CONFIG_KNX_AUTO_LOAD_TABLES default memory_reset() applies when
	 * there is nothing to restore. What memory_read() does NOT restore is
	 * table_reference_addresses/associations/group_object/
	 * application_program/interface_program: those are separate RAM-only
	 * mirrors of PID_TABLE_REFERENCE (never persisted — see
	 * sync_table_reference() in object_interface.c), initialised from the
	 * CONFIG_KNX_AUTO_LOAD_TABLES compile-time default regardless of what
	 * Load State actually came back. Left alone, a device that persisted
	 * LS_LOADED with CONFIG_KNX_AUTO_LOAD_TABLES=n would report
	 * PID_TABLE_REFERENCE=0 for an object ETS believes is loaded — the
	 * exact "allocation failed" failure §2.4b already fixed once, from a
	 * different cause.
	 *
	 * Feeding a LE_NOOP through the load state machine changes nothing
	 * about the state itself but does run its unconditional tail
	 * (sync_table_reference()), so this resyncs every table_reference_*
	 * to match whatever Load State memory_read() just produced.
	 */
	{
		uint8_t noop[1] = {LE_NOOP};

		load_state_machine_event(OBJ_IDX_ADDRESS_TABLE, noop, sizeof(noop));
		load_state_machine_event(OBJ_IDX_ASSOCIATION_TABLE, noop, sizeof(noop));
		load_state_machine_event(OBJ_IDX_GROUP_OBJ_TABLE, noop, sizeof(noop));
		load_state_machine_event(OBJ_IDX_APPLICATION_PROG, noop, sizeof(noop));
		load_state_machine_event(OBJ_IDX_INTERFACE_PROG, noop, sizeof(noop));
	}

	/*
	 * Run State is never persisted (only Load State is), so both
	 * Application Program objects start every boot at RS_HALTED
	 * regardless of what memory_read() just restored. Re-running their
	 * PID_LOAD_STATE_CONTROL write callback applies the same "auto
	 * Halted -> Running once Load State is Loaded" rule (KNX spec §9.7)
	 * an explicit ETS load-control write would — without this, a device
	 * that persisted LS_LOADED would boot sitting Halted over data it
	 * believes is loaded.
	 */
	object_application_program_properties_written(PID_LOAD_STATE_CONTROL);
	object_interface_program_properties_written(PID_LOAD_STATE_CONTROL);

	/* After memory_read(): fills PID_PROGRAM_VERSION from
	 * CONFIG_KNX_APPLICATION_PROGRAM_ID/VERSION only when nothing is stored,
	 * so a value ETS has written always wins.
	 */
	object_application_program_init();

	T_Init();

	LOG_DBG("KNX Device %u.%u.%u", KNX_ADDR_VAL(device_individual_address()));
	L_Init();

	/*
	 * Group Object 'I' flag (read-on-init). A live ETS
	 * download reaches this through object_group_table_properties_written()
	 * when PID_LOAD_STATE_CONTROL transitions the table to LS_LOADED, but a
	 * device that already persisted LS_LOADED before this boot never runs
	 * that write — so re-derive it here too, only now that L_Init() has
	 * started the L2 TX thread the read requests need to actually go out.
	 */
	group_object_read_on_init_trigger();

	return 0;
}

SYS_INIT(knx_init, POST_KERNEL, CONFIG_KNX_STACK_INIT_PRIO);
