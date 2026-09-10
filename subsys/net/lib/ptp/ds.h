/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ds.h
 * @brief Datasets types.
 *
 * References are to version 2019 of IEEE 1588, ("PTP")
 */

#ifndef ZEPHYR_INCLUDE_PTP_DS_H_
#define ZEPHYR_INCLUDE_PTP_DS_H_

#include <zephyr/net/ptp_time.h>

#include "ddt.h"
#include "state_machine.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PTP Default Dataset.
 * @note 8.2.1 - defaultDS data set member specification
 */
struct ptp_default_ds {
	/* static */
	/** Clock ID */
	ptp_clk_id	       clk_id;
	/** Indicates number of ptp ports on the PTP Instance. */
	uint16_t	       n_ports;
	/* dynamic */
	/** Quality of a clock. */
	struct ptp_clk_quality clk_quality;
	/** Parameter used in the execution BTCA. */
	uint8_t		       priority1;
	/** Parameter used in the execution BTCA. */
	uint8_t		       priority2;
	/** ID number of the instance in a domain. */
	uint8_t		       domain;
	/** sdoId attribute. */
	uint16_t	       sdo_id: 12;
	/** Flag indicating timeReceiver mode. */
	bool		       time_receiver_only;
	/* optional */
	/** Current value of the PTP Instance Time. */
	struct net_ptp_time    current_time;
	/** Enable flag. */
	bool		       enable;
	/** Flag indication if external port configuration option is enabled. */
	bool		       external_port_conf_en;
	/** Maximum value of steps removed of an Announce messages to be considered in BTCA. */
	uint8_t		       max_steps_rm;
	/** PTP Instance type. */
	uint8_t		       type;
};

/**
 * @brief PTP Current Dataset.
 * @note 8.2.2 - currentDS data set member specification
 */
struct ptp_current_ds {
	/** Number of PTP Communication Paths traversed between PTP Instance and the GM. */
	uint16_t	 steps_rm;
	/** Current value of time difference between a Transmitter and Receiver.
	 *
	 * @note it is computed as <time on the Receiver> - <time on the Transmitter>
	 */
	ptp_timeinterval offset_from_tt;
	/** Mean propagation time. */
	ptp_timeinterval mean_delay;
	/* optional */
	/** Flag inticating if port is synchronized. */
	bool		 sync_uncertain;
};

/**
 * @brief PTP Parent Dataset.
 * @note 8.2.3 - parentDS data set member specification
 */
struct ptp_parent_ds {
	/** PTP Port's ID */
	struct ptp_port_id     port_id;
	/** Flag indication if the Instance has a Port in Receiver state or has estimates
	 * of obsreved_parent_offset_scaled_log_variance or obsreved_parent_clk_phase_change_rate.
	 */
	bool		       stats;
	/** Estimate of the variance of the phase offset. */
	uint16_t	       obsreved_parent_offset_scaled_log_variance;
	/** Estimate of the phase change rate. */
	int32_t		       obsreved_parent_clk_phase_change_rate;
	/** Grandmaster's ID. */
	ptp_clk_id	       gm_id;
	/** Grandmaster's Clock quality. */
	struct ptp_clk_quality gm_clk_quality;
	/** Value of Grandmaster's priority1 attribute. */
	uint8_t		       gm_priority1;
	/** Value of Grandmaster's priority2 attribute. */
	uint8_t		       gm_priority2;
	/** Flag inticating use of sync_uncertain flag in Announce message. */
	bool		       sync_uncertain;
	/** Address of the PTP Port issuing sync messages used to synchronize this PTP Instance. */
	struct ptp_port_addr   protocol_addr;
};

/**
 * @brief PTP Time Properties Dataset.
 * @note 8.2.4 - timePropertiesDS data set member specification
 */
struct ptp_time_prop_ds {
	/** Value of dLS received from Grandmaster */
	int16_t current_utc_offset;
	/** Flags used for operation of time received from Grandmaster PTP Instance. */
	uint8_t flags;
	/** Source of time used by Grandmaster PTP Instance. */
	uint8_t time_src;
};

/**
 * @brief PTP Non-volatile Storage Dataset.
 * @note 8.2.7 - nonvolatileStorageDS
 */
struct ptp_nvs_ds {
	/** Reset non-volatile storage. */
	bool reset;
	/** Save current values of applicable dynamic or configurable data set members
	 * to non-volatile storage.
	 */
	bool save;
};

/**
 * @brief Enumeration for types of delay mechanisms for PTP Clock.
 */
enum ptp_delay_mechanism {
	PTP_DM_E2E = 1,
	PTP_DM_P2P,
	PTP_DM_COMMON_P2P,
	PTP_DM_SPECIAL,
	PTP_DM_NO_MECHANISM = 0xFE
};

/**
 * @brief PTP Port Dataset
 * @note 8.2.15 - portDS data set member specification
 */
struct ptp_port_ds {
	/* static */
	/** PTP Port's ID. */
	struct ptp_port_id	 id;
	/* dynamic */
	/** State of a PTP Port. */
	enum ptp_port_state	 state;
	/** Logarithm to the base 2  minimal Delay_Req interval in nanoseconds. */
	int8_t			 log_min_delay_req_interval;
	/** Current one-way propagation delay. */
	ptp_timeinterval	 mean_link_delay;
	/* configurable */
	/** Logarithm to the base 2 Announce interval in nanoseconds. */
	int8_t			 log_announce_interval;
	/** Number of Announce intervals before timeout. */
	uint8_t			 announce_receipt_timeout;
	/** Logarithm to the base 2 Sync interval in nanoseconds. */
	int8_t			 log_sync_interval;
	/** Delay mechanism used by the PTP Port. */
	enum ptp_delay_mechanism delay_mechanism;
	/** Logarithm to the base 2  minimal Pdelay_Req interval in nanoseconds. */
	int8_t			 log_min_pdelay_req_interval;
	/** Version of supported PTP standard. */
	uint8_t			 version;
	/** Value of delay asymmetry. */
	ptp_timeinterval	 delay_asymmetry;
	/* optional */
	/** Enable flag. */
	bool			 enable;
	/** Flag setting PTP Port in timeTransmitter mode. */
	bool			 time_transmitter_only;
};

/**
 * @brief PTP Description Port Dataset.
 * @note 8.2.18 - descriptionPortDS
 */
struct ptp_dest_port_ds {
	/** PTP profile identifier for the PTP Port. */
	union {
		struct {
			uint8_t byte[6];
		};
		uint64_t id: 48;
	} profile_id;
	/** Protocol address of the PTP Port */
	struct ptp_port_addr protocol_addr;
};

/**
 * @brief Generic Data set type used for dataset comparison algorithm.
 */
struct ptp_dataset {
	/** Parameter used in the execution BTCA. */
	uint8_t		       priority1;
	/** PTP Clock's ID. */
	ptp_clk_id	       clk_id;
	/** PTP Clock's quality. */
	struct ptp_clk_quality clk_quality;
	/** Parameter used in the execution BTCA. */
	uint8_t		       priority2;
	/** Number of PTP Communication Paths traversed between PTP Instance and the GM. */
	uint16_t	       steps_rm;
	/** timeTransmitter ID. */
	struct ptp_port_id     sender;
	/** timeReceiver ID. */
	struct ptp_port_id     receiver;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PTP_DS_H_ */
