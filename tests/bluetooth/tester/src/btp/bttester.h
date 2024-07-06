/* bttester.h - Bluetooth tester headers */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/addr.h>
#include <sys/types.h>

static inline void tester_set_bit(uint8_t *addr, unsigned int bit)
{
	uint8_t *p = addr + (bit / 8U);

	*p |= BIT(bit % 8);
}

static inline uint8_t tester_test_bit(const uint8_t *addr, unsigned int bit)
{
	const uint8_t *p = addr + (bit / 8U);

	return *p & BIT(bit % 8);
}

void tester_init(void);

void tester_rsp(uint8_t service, uint8_t opcode, uint8_t status);
void tester_rsp_full(uint8_t service, uint8_t opcode, const void *rsp, size_t len);
void tester_event(uint8_t service, uint8_t opcode, const void *data, size_t len);
int tester_rsp_buffer_lock(void);
void tester_rsp_buffer_unlock(void);
void tester_rsp_buffer_free(void);
void tester_rsp_buffer_allocate(size_t len, uint8_t **data);

/* Used to indicate that command length is variable and that validation will
 * be done in handler.
 */
#define BTP_HANDLER_LENGTH_VARIABLE (-1)

struct btp_handler {
	uint8_t opcode;
	uint8_t index;
	k_ssize_t expect_len;
	uint8_t (*func)(const void *cmd, uint16_t cmd_len,
			void *rsp, uint16_t *rsp_len);
};

void tester_register_command_handlers(uint8_t service,
				      const struct btp_handler *handlers,
				      size_t num);

uint8_t tester_init_gatt(void);
uint8_t tester_unregister_gatt(void);

uint8_t tester_init_l2cap(void);
uint8_t tester_unregister_l2cap(void);

uint8_t tester_init_mesh(void);
uint8_t tester_unregister_mesh(void);

uint8_t tester_init_vcp(void);
uint8_t tester_unregister_vcp(void);

uint8_t tester_init_vcs(void);
uint8_t tester_unregister_vcs(void);

uint8_t tester_init_aics(void);
uint8_t tester_unregister_aics(void);

uint8_t tester_init_vocs(void);
uint8_t tester_unregister_vocs(void);

uint8_t tester_init_ias(void);
uint8_t tester_unregister_ias(void);

uint8_t tester_init_mmdl(void);
uint8_t tester_unregister_mmdl(void);

uint8_t tester_init_gap(void);
uint8_t tester_unregister_gap(void);

void tester_init_core(void);

uint8_t tester_init_pacs(void);
uint8_t tester_unregister_pacs(void);

uint8_t tester_init_ascs(void);
uint8_t tester_unregister_ascs(void);

uint8_t tester_init_bap(void);
uint8_t tester_unregister_bap(void);

uint8_t tester_init_has(void);
uint8_t tester_unregister_has(void);

uint8_t tester_init_csis(void);
uint8_t tester_unregister_csis(void);

uint8_t tester_init_csip(void);
uint8_t tester_unregister_csip(void);

uint8_t tester_init_micp(void);
uint8_t tester_unregister_micp(void);

uint8_t tester_init_mics(void);
uint8_t tester_unregister_mics(void);

uint8_t tester_init_ccp(void);
uint8_t tester_unregister_ccp(void);

uint8_t tester_init_vcp(void);
uint8_t tester_unregister_vcp(void);

uint8_t tester_init_cas(void);
uint8_t tester_unregister_cas(void);

uint8_t tester_init_cap(void);
uint8_t tester_unregister_cap(void);

uint8_t tester_init_mcp(void);
uint8_t tester_unregister_mcp(void);

uint8_t tester_init_mcs(void);
uint8_t tester_unregister_mcs(void);

uint8_t tester_init_hap(void);
uint8_t tester_unregister_hap(void);

uint8_t tester_init_tbs(void);
uint8_t tester_unregister_tbs(void);

uint8_t tester_init_tmap(void);
uint8_t tester_unregister_tmap(void);

uint8_t tester_init_ots(void);
uint8_t tester_unregister_ots(void);
