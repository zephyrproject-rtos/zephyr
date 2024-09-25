/* Copyright 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <stdint.h>

struct toc2_data {
	uint32_t toc2_size;
	uint32_t l1_app_descr_addr;
	uint32_t service_app_descr_addr;
	uint32_t debug_cert_addr;
} __packed;

struct l1_desc {
	uint32_t l1_app_descr_size;
	uint32_t boot_strap_addr;
	uint32_t boot_strap_dst_addr;
	uint32_t boot_strap_size;
	uint32_t reserved[3];
} __packed;

struct l1_usr_app_hdr {
	uint8_t reserved[32];
} __packed;

struct app_header {
	struct toc2_data toc2_data;
	struct l1_desc l1_desc;
	uint8_t padding[4];
	struct l1_usr_app_hdr l1_usr_app_hdr;
} __packed;

const struct app_header app_header Z_GENERIC_SECTION(.app_header) = {
	.toc2_data = {.toc2_size = sizeof(struct toc2_data),
		      .l1_app_descr_addr = offsetof(struct app_header, l1_desc)},
	.l1_desc = {.l1_app_descr_size = sizeof(struct l1_desc),
		    .boot_strap_addr = DT_REG_ADDR(DT_NODELABEL(bootstrap_region)) -
				       DT_REG_ADDR(DT_NODELABEL(flash0)),
		    .boot_strap_dst_addr = DT_REG_ADDR(DT_NODELABEL(sram_bootstrap)),
		    .boot_strap_size = DT_REG_SIZE(DT_NODELABEL(sram_bootstrap))},
};
