/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * KNX subsystem-internal types.
 *
 * Ph_Data_Req_Class, Ph_Data_Ind_Class, P_Status and FRAME_* constants
 * have been moved to <zephyr/knx/knx_l1.h> (the public L1 contract).
 * This file re-exports them via that header and adds the subsystem-private
 * types used by L2..L7.
 */
#ifndef __KNX_TYPES__
#define __KNX_TYPES__

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/knx/knx_l1.h>

/* L2 frame format */
typedef enum {
	L_Data_Extended = 0x0,
	L_Data_Standard = 0x1
} FrameFormat;

/* L2/L3/L4 priority (legacy names — prefer knx_priority_t from knx_core.h) */
typedef enum {
	LowPriority = 0x3,
	NormalPriority = 0x1,
	UrgentPriority = 0x2,
	SystemPriority = 0x0,
	Invalid = 0x4,
	Repeted = 0x5
} Priority;

typedef enum {
	Individual = 0x0,
	Group = 0x1
} AddressType;
#define Multicast Group

typedef enum {
	l_ok = 0x0,
	l_not_ok = 0x1
} L_Status;
typedef enum {
	n_ok = 0x0,
	n_not_ok = 0x1
} N_Status;
typedef enum {
	t_ok = 0x0,
	t_not_ok = 0x1
} T_Status;
typedef enum {
	a_ok = 0x0,
	a_not_ok = 0x1
} A_Status;

enum HopCountType {
	NetworkLayerParameter,
	UnlimitedRouting = 7
};

typedef enum {
	T_DataBroadcast,
	T_DataGroup,
	T_DataIndividual,
	T_DataConnected,
	T_Connect,
	T_Disconnect,
	T_Ack,
	T_Nack,
} T_Pdu_Type;

/* Ph_Data_Con_Class (unused in Zephyr port but kept for reference) */
typedef enum {
	Con_OK,
	Con_bus_not_free,
	Con_collision_detected,
	Con_transceiver_fault
} Ph_Data_Con_Class;

#endif /* __KNX_TYPES__ */
