/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief NRF Wi-Fi debug shell module
 */
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include "host_rpu_umac_if.h"
#include "fmac_main.h"

extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;
struct nrf_wifi_ctx_zep *dbg_ctx = &rpu_drv_priv_zep.rpu_ctx_zep;


static int nrf_wifi_dbg_read_mem(const struct shell *sh,
				 size_t argc,
				 const char *argv[])
{
	enum nrf_wifi_status status;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx;
	char *ptr;
	unsigned long mem_type;
	unsigned long mem_offset;
	unsigned long num_words;
	unsigned int mem_val;
	unsigned int mem_start_addr;
	unsigned int mem_end_addr;
	unsigned int i;

	fmac_dev_ctx = dbg_ctx->rpu_ctx;
	mem_type = strtoul(argv[1], &ptr, 10);
	mem_offset = strtoul(argv[2], &ptr, 10);
	num_words = strtoul(argv[3], &ptr, 10);

	if (mem_type == 0) {
		mem_start_addr = RPU_ADDR_PKTRAM_START + (mem_offset * 4);
		mem_end_addr = RPU_ADDR_PKTRAM_END;
	} else if (mem_type == 1) {
		mem_start_addr = RPU_ADDR_GRAM_START + (mem_offset * 4);
		mem_end_addr = RPU_ADDR_GRAM_END;
	} else {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid memory type(%lu).\n",
			      mem_type);
		return -ENOEXEC;
	}

	if ((mem_start_addr % 4) != 0) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid memory word offset(%lu). Needs to be a multiple of 4\n",
			      mem_offset);
		return -ENOEXEC;
	}

	if (mem_start_addr + (num_words * 4) - 1 > mem_end_addr) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid number of words(%lu). Exceeds memory region\n",
			      num_words);
		return -ENOEXEC;
	}

	for (i = 0; i < (num_words * 4); i += 4) {
		status = hal_rpu_mem_read(fmac_dev_ctx->hal_dev_ctx,
					  &mem_val,
					  mem_start_addr + i,
					  sizeof(mem_val));

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			shell_fprintf(sh,
				      SHELL_ERROR,
				      "Failed to read memory at 0x%x.\n",
				      mem_start_addr + i);
			return -ENOEXEC;
		}

		if (i % 16 == 0) {
			shell_fprintf(sh,
				      SHELL_INFO,
				      "\n0x%x: ", mem_start_addr + i);
		}

		shell_fprintf(sh,
			      SHELL_INFO,
			      "0x%08x ",
			      mem_val);
	}

	shell_fprintf(sh,
		      SHELL_INFO,
		      "\n");
	return 0;
}


static int nrf_wifi_dbg_write_mem(const struct shell *sh,
				  size_t argc,
				  const char *argv[])
{
	enum nrf_wifi_status status;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx;
	char *ptr;
	unsigned long mem_type;
	unsigned long mem_offset;
	unsigned int val;
	unsigned int mem_start_addr;
	unsigned int mem_end_addr;

	fmac_dev_ctx = dbg_ctx->rpu_ctx;
	mem_type = strtoul(argv[1], &ptr, 10);
	mem_offset = strtoul(argv[2], &ptr, 10);
	val = strtoul(argv[3], &ptr, 10);

	if (mem_type == 0) {
		mem_start_addr = RPU_ADDR_PKTRAM_START + (mem_offset * 4);
		mem_end_addr = RPU_ADDR_PKTRAM_END;
	} else if (mem_type == 1) {
		mem_start_addr = RPU_ADDR_GRAM_START + (mem_offset * 4);
		mem_end_addr = RPU_ADDR_GRAM_END;
	} else {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid memory type(%lu).\n",
			      mem_type);
		return -ENOEXEC;
	}

	if ((mem_start_addr % 4) != 0) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid memory word offset(%lu). Needs to be a multiple of 4\n",
			      mem_offset);
		return -ENOEXEC;
	}

	if (mem_start_addr + 3 > mem_end_addr) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid memory word offset. Exceeds memory region\n");
		return -ENOEXEC;
	}

	status = hal_rpu_mem_write(fmac_dev_ctx->hal_dev_ctx,
				   mem_start_addr,
				   &val,
				   sizeof(val));

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Failed to write memory at 0x%x.\n",
			      mem_start_addr);
		return -ENOEXEC;
	}

	return 0;
}


static int nrf_wifi_dbg_read_reg(const struct shell *sh,
				 size_t argc,
				 const char *argv[])
{
	enum nrf_wifi_status status;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx;
	char *ptr;
	unsigned long reg_type;
	unsigned long reg_offset;
	unsigned long num_regs;
	unsigned int reg_val;
	unsigned int reg_start_addr;
	unsigned int reg_end_addr;
	unsigned int i;

	fmac_dev_ctx = dbg_ctx->rpu_ctx;
	reg_type = strtoul(argv[1], &ptr, 10);
	reg_offset = strtoul(argv[2], &ptr, 10);
	num_regs = strtoul(argv[3], &ptr, 10);

	if (reg_type == 0) {
		reg_start_addr = RPU_ADDR_SBUS_START + (reg_offset * 4);
		reg_end_addr = RPU_ADDR_SBUS_END;
	} else if (reg_type == 1) {
		reg_start_addr = RPU_ADDR_PBUS_START + (reg_offset * 4);
		reg_end_addr = RPU_ADDR_PBUS_END;
	} else {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid register type(%lu).\n",
			      reg_type);
		return -ENOEXEC;
	}

	if ((reg_start_addr % 4) != 0) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid register offset(%lu). Needs to be a multiple of 4\n",
			      reg_offset);
		return -ENOEXEC;
	}

	if (reg_start_addr + (num_regs * 4) - 1 > reg_end_addr) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid number of registers(%lu). Exceeds bus region\n",
			      num_regs);
		return -ENOEXEC;
	}

	for (i = 0; i < num_regs * 4; i += 4) {
		status = hal_rpu_reg_read(fmac_dev_ctx->hal_dev_ctx,
					  &reg_val,
					  reg_start_addr + i);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			shell_fprintf(sh,
				      SHELL_ERROR,
				      "Failed to read register at 0x%x.\n",
				      reg_start_addr + i);
			return -ENOEXEC;
		}

		shell_fprintf(sh,
			      SHELL_INFO,
			      "0x%x: 0x%08x\n",
			      reg_start_addr + i,
			      reg_val);
	}

	shell_fprintf(sh,
		      SHELL_INFO,
		      "\n");
	return 0;
}


static int nrf_wifi_dbg_write_reg(const struct shell *sh,
				  size_t argc,
				  const char *argv[])
{
	enum nrf_wifi_status status;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx;
	char *ptr;
	unsigned long reg_type;
	unsigned long reg_offset;
	unsigned int val;
	unsigned int reg_start_addr;
	unsigned int reg_end_addr;

	fmac_dev_ctx = dbg_ctx->rpu_ctx;
	reg_type = strtoul(argv[1], &ptr, 10);
	reg_offset = strtoul(argv[2], &ptr, 10);
	val = strtoul(argv[3], &ptr, 10);

	if (reg_type == 0) {
		reg_start_addr = RPU_ADDR_SBUS_START + (reg_offset * 4);
		reg_end_addr = RPU_ADDR_SBUS_END;
	} else if (reg_type == 1) {
		reg_start_addr = RPU_ADDR_PBUS_START + (reg_offset * 4);
		reg_end_addr = RPU_ADDR_PBUS_END;
	} else {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid register type(%lu).\n",
			      reg_type);
		return -ENOEXEC;
	}

	if ((reg_start_addr % 4) != 0) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid register offset(%lu). Needs to be a multiple of 4\n",
			      reg_offset);
		return -ENOEXEC;
	}

	if (reg_start_addr + 3 > reg_end_addr) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid register offset. Exceeds bus region\n");
		return -ENOEXEC;
	}

	status = hal_rpu_reg_write(fmac_dev_ctx->hal_dev_ctx,
				   reg_start_addr,
				   val);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Failed to write register at 0x%x.\n",
			      reg_start_addr);
		return -ENOEXEC;
	}

	return 0;
}



SHELL_STATIC_SUBCMD_SET_CREATE(
	nrf70_dbg,
	SHELL_CMD_ARG(read_mem,
		      NULL,
		      "<mem_type> <offset> <num_words>\n"
		      "where:\n"
		      "<mem_type> : One of the memory regions below\n"
		      "0 - PKTRAM\n"
		      "1 - GRAM\n"
		      "<offset> : Word offset in the memory region\n"
		      "<num_words> : Number of words to read\n",
		      nrf_wifi_dbg_read_mem,
		      4,
		      0),
	SHELL_CMD_ARG(write_mem,
		      NULL,
		      "<mem_type> <offset> <val>\n"
		      "where:\n"
		      "<mem_type> : One of the memory regions below\n"
		      "0 - PKTRAM\n"
		      "1 - GRAM\n"
		      "<offset> : Word offset in the memory region\n"
		      "<val> : Value to be written\n",
		      nrf_wifi_dbg_write_mem,
		      4,
		      0),
	SHELL_CMD_ARG(read_reg,
		      NULL,
		      "<reg_type> <offset> <num_regs>\n"
		      "where:\n"
		      "<reg_type> : One of the bus regions below\n"
		      "0 - SYSBUS\n"
		      "1 - PBUS\n"
		      "<offset> : Register offset\n"
		      "<num_words> : Number of registers to read\n",
		      nrf_wifi_dbg_read_reg,
		      4,
		      0),
	SHELL_CMD_ARG(write_reg,
		      NULL,
		      "<reg_type> <offset> <val>\n"
		      "where:\n"
		      "<reg_type> : One of the bus regions below\n"
		      "0 - SYSBUS\n"
		      "1 - PBUS\n"
		      "<offset> : Register offset\n"
		      "<val> : Value to be written\n",
		      nrf_wifi_dbg_write_reg,
		      4,
		      0),
	SHELL_SUBCMD_SET_END
);


SHELL_SUBCMD_ADD((nrf70), dbg, &nrf70_dbg,
		 "nRF70 advanced debug commands\n",
		 NULL,
		 0, 0);
