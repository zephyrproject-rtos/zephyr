/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef TEST_DRIVERS_I3C_I3C_EMUL_TEST_TARGET_EMUL_H_
#define TEST_DRIVERS_I3C_I3C_EMUL_TEST_TARGET_EMUL_H_

#include <stdint.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i3c_emul.h>

/*
 * PIDs and address-assignment defaults for the two test peripherals
 * declared in tests/drivers/i3c/i3c_emul/boards/native_sim.overlay.
 * Used by every test suite to identify and re-establish the bus's
 * canonical address state.
 */
#define TEST_TARGET_A_PID	((uint64_t)0x1234 << 32 | 0x12345678)
#define TEST_TARGET_A_STATIC	0x55
#define TEST_TARGET_B_PID	((uint64_t)0x5678 << 32 | 0xABCDEF01)
#define TEST_TARGET_B_INIT_DA	0x42
#define TEST_TARGET_C_PID	((uint64_t)0x1112 << 32 | 0x22334455)
#define TEST_TARGET_C_STATIC	0x66
#define TEST_TARGET_D_PID	((uint64_t)0x9999 << 32 | 0xAABBCCDD)
#define TEST_TARGET_D_STATIC	0x77
#define TEST_TARGET_E_PID	((uint64_t)0x1111 << 32 | 0x22334456)
#define TEST_TARGET_E_STATIC	0x44

struct test_target_backend_api {
	uint8_t (*get_reg)(const struct emul *target, uint8_t idx);
	void (*set_reg)(const struct emul *target, uint8_t idx, uint8_t value);
	uint8_t (*get_dynamic_addr)(const struct emul *target);
	void (*install_mock)(const struct emul *target, struct i3c_emul_api *mock);
	int (*trigger_ibi)(const struct emul *target, uint8_t *payload, uint8_t len);
	int (*trigger_hj)(const struct emul *target);
	int (*trigger_crr)(const struct emul *target);
	void (*set_status_fmt1)(const struct emul *target, uint16_t status);
	uint16_t (*get_mrl)(const struct emul *target);
	uint16_t (*get_mwl)(const struct emul *target);
	bool (*deftgts_was_seen)(const struct emul *target);
	size_t (*get_deftgts)(const struct emul *target, uint8_t *out, size_t out_len);
	void (*clear_deftgts)(const struct emul *target);
	bool (*event_enabled)(const struct emul *target, uint8_t event_mask);
};

uint8_t test_target_get_reg(const struct emul *target, uint8_t idx);
void test_target_set_reg(const struct emul *target, uint8_t idx, uint8_t value);
uint8_t test_target_get_dynamic_addr(const struct emul *target);
void test_target_install_mock(const struct emul *target, struct i3c_emul_api *mock);
int test_target_trigger_ibi(const struct emul *target, uint8_t *payload, uint8_t len);
int test_target_trigger_hj(const struct emul *target);
int test_target_trigger_crr(const struct emul *target);
void test_target_set_status_fmt1(const struct emul *target, uint16_t status);
uint16_t test_target_get_mrl(const struct emul *target);
uint16_t test_target_get_mwl(const struct emul *target);
bool test_target_deftgts_was_seen(const struct emul *target);
size_t test_target_get_deftgts(const struct emul *target, uint8_t *out, size_t out_len);
void test_target_clear_deftgts(const struct emul *target);

/*
 * Per-event enable state tracked by ENEC/DISEC. event_mask is one of
 * I3C_CCC_EVT_INTR / _CR / _HJ. Returns true if the peripheral is
 * currently allowed to raise that event class.
 */
bool test_target_event_enabled(const struct emul *target, uint8_t event_mask);

/*
 * Number of HDR-DDR private xfers the peripheral has handled. Used to
 * verify the bus emulator dispatches I3C_MSG_HDR | hdr_mode = HDR_DDR
 * messages through hdr_ddr_xfers rather than the SDR xfers path.
 */
uint32_t test_target_get_hdr_ddr_xfer_count(const struct emul *target);

/*
 * Bring the bus back to the canonical address-assignment state every
 * test-suite setup expects:
 *   - target identified by pid_a is reachable at static_a (via SETDASA)
 *   - target identified by pid_b is reachable at init_dyn_b (via DAA)
 *
 * If the bus is already in that state, this is a near-no-op (just
 * confirms via i3c_device_find). Safe to call from any test setup or
 * test body to recover from RSTDAA / SETNEWDA / HJ-driven DAA effects.
 *
 * Returns 0 on success, negative errno on failure.
 */
struct device;
int test_target_bus_known_state(const struct device *bus, uint64_t pid_a, uint8_t static_a,
				uint64_t pid_b, uint8_t init_dyn_b);

#endif /* TEST_DRIVERS_I3C_I3C_EMUL_TEST_TARGET_EMUL_H_ */
