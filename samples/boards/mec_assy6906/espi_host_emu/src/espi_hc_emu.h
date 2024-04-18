/*
 * Copyright (c) 2024 Microchip Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SAMPLES_BOARDS_MEC_ASSY6906_ESPI_HC_EMU_H_
#define __SAMPLES_BOARDS_MEC_ASSY6906_ESPI_HC_EMU_H_

#include <stdint.h>
#include <stddef.h>

int espi_hc_emu_init(uint32_t freqhz);
int espi_hc_emu_espi_reset_n(uint8_t level);
int espi_hc_emu_vcc_pwrgd(uint8_t level);
int espi_hc_emu_is_target_ready(void);

int espi_hc_is_alert(void);
int espi_hc_emu_xfr(const uint8_t *msg, size_t msglen, uint8_t *response, size_t resplen);

#define ESPI_HC_EMU_MSG_BUFF_LEN		256u
#define ESPI_HC_EMU_RESP_BUFF_LEN		256u

/* 16-bit GET_CONFIGURATION addresses */
#define ESPI_GET_CONFIG_DEV_ID			0x04u
#define ESPI_GET_CONFIG_GLB_CAP			0x08u
#define ESPI_GET_CONFIG_PC_CAP			0x10u
#define ESPI_GET_CONFIG_VW_CAP			0x20u
#define ESPI_GET_CONFIG_OOB_CAP			0x30u
#define ESPI_GET_CONFIG_FC_CAP			0x40u

/* 16-bit eSPI Status bit definitions */
#define ESPI_STATUS_PC_FREE_POS			0
#define ESPI_STATUS_PC_NP_FREE_POS		1
#define ESPI_STATUS_VW_FREE_POS			2
#define ESPI_STATUS_OOB_FREE_POS		3
#define ESPI_STATUS_PC_AVAIL_POS		4
#define ESPI_STATUS_PC_NP_AVAIL_POS		5
#define ESPI_STATUS_VW_AVAIL_POS		6
#define ESPI_STATUS_OOB_AVAIL_POS		7
#define ESPI_STATUS_FC_FREE_POS			8
#define ESPI_STATUS_FC_NP_FREE_POS		9
#define ESPI_STATUS_FC_AVAIL_POS		12
#define ESPI_STATUS_FC_NP_AVAIL_POS		13

#define ESPI_STATUS_ALL_AVAIL			\
	(BIT(ESPI_STATUS_PC_AVAIL_POS) | BIT(ESPI_STATUS_PC_NP_AVAIL_POS) | \
	 BIT(ESPI_STATUS_VW_AVAIL_POS) | BIT(ESPI_STATUS_OOB_AVAIL_POS) | \
	 BIT(ESPI_STATUS_FC_AVAIL_POS) | BIT(ESPI_STATUS_FC_NP_AVAIL_POS))

#define ESPI_VW_FLAG_DIR_CT 0x1

/* Host context VWire structure */
struct espi_hc_vw {
	uint8_t host_index;
	uint8_t mask;
	uint8_t reset_val;
	uint8_t levels;
	uint8_t valid;
	uint8_t rsvd[3];
};

/* MEC5 eSPI Target has 11 Controller-to-Target and 11
 * Target-to-Controller group registers.
 * We add two for eSPI Host indices 0 and 1 which are
 * used for Serial IRQ for Target-to-Controller.
 * Format of C2T and T2C VWires (except Serial IRQ)
 *   b[7:4]=valid
 *   b[3:0]=wire value 0=low, 1=high
 *
 * Format of Serial IRQ:
 *   b[7]=interrupt level: 0=low, 1=high(interrupt assertion)
 *   b[6:0]=interrupt line
 *          Host Index 0: IRQ 0-127
 *          Host Index 1: IRQ 128-255
 */

#define ESPI_MAX_CT_VW_GROUPS		(11)
#define ESPI_MAX_TC_VW_GROUPS		(11 + 2)

struct espi_msg_pkt {
	uint8_t buf[ESPI_HC_EMU_MSG_BUFF_LEN];
	uint16_t msglen;
	uint8_t msgofs;
	uint8_t crc8;
};

struct espi_resp_pkt {
	uint8_t buf[ESPI_HC_EMU_RESP_BUFF_LEN];
	uint32_t recvlen;
};

struct espi_hc_context {
	uint32_t version_id;
	uint32_t global_cap_cfg;
	uint32_t pc_cap_cfg;
	uint32_t vw_cap_cfg;
	uint32_t oob_cap_cfg;
	uint32_t fc_cap_cfg;
	uint32_t other_cfg;
	uint16_t pkt_status;
	uint8_t vwg_cnt_max;
	uint8_t vwg_cnt;
	struct espi_hc_vw c2t_vw[ESPI_MAX_CT_VW_GROUPS];
	struct espi_hc_vw t2c_vw[ESPI_MAX_TC_VW_GROUPS];
	struct espi_msg_pkt emsg;
	struct espi_resp_pkt ersp;
};

int espi_hc_ctx_emu_init(struct espi_hc_context *hc);

int espi_hc_ctx_emu_ctrl(struct espi_hc_context *hc, uint8_t enable);
int espi_hc_ctx_get_status(struct espi_hc_context *hc);
int espi_hc_ctx_emu_get_config(struct espi_hc_context *hc, uint16_t cfg_offset);
int espi_hc_ctx_emu_set_config(struct espi_hc_context *hc, uint16_t cfg_offset,
			       uint32_t new_config);

/* Set state of a single VWire at specified host index in the context.
 * Does not transmit VWire. If level changed, set the valid bit.
 * Use another API to transmit.
 */
int espi_hc_set_ct_vwire(struct espi_hc_context *hc, uint8_t host_index,
			 uint8_t src, uint8_t level);

/* Send/Get VWires.
 * vws array of struct espi_vwire
 */
int espi_hc_ctx_emu_put_host_index(struct espi_hc_context *hc, uint8_t host_index);

#define ESPI_HC_EMU_PUT_VW_RST_VAL 0x01
int espi_hc_ctx_emu_put_mult_host_index(struct espi_hc_context *hc, uint8_t *hidxs, uint8_t nhidx,
					uint32_t flags);

int espi_hc_ctx_emu_put_vws(struct espi_hc_context *hc, struct espi_hc_vw *vws, uint8_t nvws);
int espi_hc_ctx_emu_get_vws(struct espi_hc_context *hc, struct espi_hc_vw *vws, uint8_t nvws,
			    uint8_t *nrecv);
int espi_hc_ctx_emu_get_vws2(struct espi_hc_context *hc, struct espi_hc_vw *vws, uint8_t nvws,
			     uint8_t *nrecv);

int espi_hc_ctx_emu_update_t2c_vw(struct espi_hc_context *hc, struct espi_hc_vw *vws, uint8_t nvws);

int espi_hc_get_vw_group_data(struct espi_hc_context *hc, uint8_t host_index,
			      struct espi_hc_vw *vwgroup);

int espi_hc_emu_get_status(struct espi_hc_context *hc, uint16_t *espi_status);
int espi_hc_emu_get_config(struct espi_hc_context *hc, uint16_t config_offset,
			   uint32_t *config, uint16_t *cmd_status);
int espi_hc_emu_set_config(struct espi_hc_context *hc, uint16_t config_offset,
			   uint32_t config, uint16_t *cmd_status);

/* Peripheral Channel */
int espi_hc_emu_put_iowr(struct espi_hc_context *hc, uint32_t io_addr_len,
			 uint32_t data, uint16_t *cmd_status);
int espi_hc_emu_put_iord(struct espi_hc_context *hc, uint32_t io_addr_len,
			 uint32_t *data, uint16_t *cmd_status);

#endif /* __SAMPLES_BOARDS_MEC_ASSY6906_ESPI_HC_EMU_H_ */
