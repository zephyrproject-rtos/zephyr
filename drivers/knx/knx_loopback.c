/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * KNX loopback L1 driver — for unit testing without real hardware.
 *
 * Selected by CONFIG_KNX_LOOPBACK=y (mutually exclusive with CONFIG_NCN5130).
 *
 * Behaviour:
 *   knx_l1_send_frame() feeds the frame back into Ph_Data__ind as if it had
 *   arrived from the bus, then signals s_ldata_con_sem with p_ok.  This lets
 *   the L2..L7 RX path be exercised without any physical TP1 interface.
 *
 *   Ph_Data__req(Req_ack_char) is a no-op (no bus to ACK on).
 *   Ph_Reset__req() immediately calls Ph_Reset__con(p_ok).
 *   U_SetAddress__req() is a no-op.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/knx/knx_l1.h>

LOG_MODULE_REGISTER(knx_loopback, CONFIG_KNX_STACK_LOG_LEVEL);

/* Semaphore that signals the end of a loopback TX (always p_ok). */
static K_SEM_DEFINE(s_ldata_con_sem, 0, 1);
static volatile P_Status s_ldata_con_status;

/* -------- knx_l1 API implementation -------- */

int knx_l1_send_frame(const uint8_t *frame, uint16_t len)
{
	if (len < 7) {
		return -EMSGSIZE;
	}

	LOG_HEXDUMP_DBG(frame, len, "LOOPBACK TX:");

	/* Replay the frame back into the RX path (minus the FCS byte). */
	Ph_Data__ind(Ind_start_of_Frame, frame[0]);
	for (uint16_t i = 1; i < len - 1u; i++) {
		Ph_Data__ind(Ind_inner_Frame_char, frame[i]);
	}
	Ph_Data__ind(Ind_end_of_Frame, 0);

	/* Immediately confirm as positive. */
	s_ldata_con_status = p_ok;
	k_sem_give(&s_ldata_con_sem);
	return 0;
}

P_Status Ph_Bus_L_Data_con_wait(k_timeout_t timeout)
{
	int ret = k_sem_take(&s_ldata_con_sem, timeout);

	return (ret == 0) ? s_ldata_con_status : p_transceiver_fault;
}

void Ph_Data__req(Ph_Data_Req_Class p_class, uint8_t p_data)
{
	ARG_UNUSED(p_class);
	ARG_UNUSED(p_data);
	/* No real bus — ack chars are silent. */
}

/* Ph_Data__con is implemented in layer2_data_link.c — it's L2's callback,
 * not the driver's to provide.
 */

void Ph_Reset__req(void)
{
	/* Loopback has no transceiver to sync to — immediately confirm OK. */
	Ph_Reset__con(p_ok);
}

void Ph_Bus_Free(void)
{
	/* No bus state to track in loopback. */
}

void U_SetAddress__req(unsigned char addr_low, unsigned char addr_high)
{
	ARG_UNUSED(addr_low);
	ARG_UNUSED(addr_high);
}

/* -------- Zephyr driver init -------- */

static int knx_loopback_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	LOG_INF("KNX loopback driver initialised");
	return 0;
}

DEVICE_DEFINE(knx_loopback, "knx_loopback", knx_loopback_init, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
