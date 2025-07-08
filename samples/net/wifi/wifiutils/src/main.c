/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing RPU utils that can be invoked from shell.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/rpu_hw_if.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/qspi_if.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF70_BUSLIB_LOG_LEVEL);

#define SW_VER "3.0"

static int wifi_on_flag;
static bool hl_flag;
static int selected_blk;
static struct qspi_dev *dev;

enum {
	SHELL_OK = 0,
	SHELL_FAIL = 1
};

void print_memmap(const struct shell *shel)
{
	shell_print(shel, "\n");
	shell_print(shel, " ==================================================\n");
	shell_print(shel, "		Wezen memory map\n");
	shell_print(shel, " ==================================================\n");
	for (int i = 0; i < NUM_MEM_BLOCKS; i++) {
		shell_print(shel, " %-14s : 0x%06x - 0x%06x (%05d words)\n", blk_name[i],
			    rpu_7002_memmap[i][0], rpu_7002_memmap[i][1],
			    1 + ((rpu_7002_memmap[i][1] - rpu_7002_memmap[i][0]) >> 2));
	}
}

/* Convert to shell return values */
static inline int shell_ret(int ret)
{
	if (ret) {
		return SHELL_FAIL;
	} else {
		return SHELL_OK;
	}
}

static int cmd_write_wrd(const struct shell *shel, size_t argc, char **argv)
{
	uint32_t val;
	uint32_t addr;

	addr = strtoul(argv[1], NULL, 0);
	val = strtoul(argv[2], NULL, 0);

	if (wifi_on_flag == 0) {
		shell_print(shel, "Err!! Please run wifi_on first");
		return -1;
	}

	if (argc != 3) {
		shell_print(shel, "incorrect arguments!!");
		shell_print(shel, "$ wifiutils write_wrd <addr> <value>");
		return -1;
	}

	return shell_ret(rpu_write(addr, &val, 4));
}

static int cmd_write_blk(const struct shell *shel, size_t argc, char **argv)
{
	uint32_t pattern;
	uint32_t addr;
	uint32_t num_words;
	uint32_t offset;
	uint32_t *buff;
	int i;

	addr = strtoul(argv[1], NULL, 0);
	pattern = strtoul(argv[2], NULL, 0);
	offset = strtoul(argv[3], NULL, 0);
	num_words = strtoul(argv[4], NULL, 0);

	if (wifi_on_flag == 0) {
		shell_print(shel, "Err!! Please run wifi_on first");
		return -1;
	}

	if (argc != 5) {
		shell_print(shel, "incorrect arguments!!");
		shell_print(shel, "$ wifiutils write_blk <addr> "
				   "<start_pattern> <pattern_incr> <num_words>");
		return -1;
	}

	if (num_words > 2000) {
		shell_print(shel,
			    "Presently supporting block read/write only up to 2000 32-bit words");
		return SHELL_FAIL;
	}

	buff = (uint32_t *)k_malloc(num_words * 4);

	for (i = 0; i < num_words; i++) {
		buff[i] = pattern + i * offset;
	}

	if (!rpu_write(addr, buff, num_words * 4)) {
		return SHELL_FAIL;
	}

	k_free(buff);

	return SHELL_OK;
}

static int cmd_read_wrd(const struct shell *shel, size_t argc, char **argv)
{
	uint32_t val;
	uint32_t addr;

	addr = strtoul(argv[1], NULL, 0);

	if (wifi_on_flag == 0) {
		shell_print(shel, "Err!! Please run wifi_on first");
		return -1;
	}

	if (argc != 2) {
		shell_print(shel, "incorrect arguments!!");
		shell_print(shel, "$ wifiutils read_wrd <addr>");
		return -1;
	}

	if (rpu_read(addr, &val, 4)) {
		return SHELL_FAIL;
	}

	shell_print(shel, "0x%08x\n", val);
	return SHELL_OK;
}

static int cmd_read_blk(const struct shell *shel, size_t argc, char **argv)
{
	uint32_t *buff;
	uint32_t addr;
	uint32_t num_words;
	uint32_t rem;
	int i;

	addr = strtoul(argv[1], NULL, 0);
	num_words = strtoul(argv[2], NULL, 0);

	if (wifi_on_flag == 0) {
		shell_print(shel, "Err!! Please run wifi_on first");
		return -1;
	}

	if (argc != 3) {
		shell_print(shel, "incorrect arguments!!");
		shell_print(shel, "$ wifiutils read_blk <addr> <num_words>");
		return -1;
	}

	if (num_words > 2000) {
		shell_print(shel,
			    "Presently supporting block read/write only up to 2000 32-bit words");
		return SHELL_FAIL;
	}

	buff = (uint32_t *)k_malloc(num_words * 4);

	if (rpu_read(addr, buff, num_words * 4)) {
		return SHELL_FAIL;
	}

	for (i = 0; i < num_words; i += 4) {
		rem = num_words - i;
		switch (rem) {
		case 1:
			shell_print(shel, "%08x", buff[i]);
			break;
		case 2:
			shell_print(shel, "%08x %08x", buff[i], buff[i + 1]);
			break;
		case 3:
			shell_print(shel, "%08x %08x %08x", buff[i], buff[i + 1], buff[i + 2]);
			break;
		default:
			shell_print(shel, "%08x %08x %08x %08x", buff[i], buff[i + 1], buff[i + 2],
				    buff[i + 3]);
			break;
		}
	}

	k_free(buff);
	return SHELL_OK;
}

static int cmd_memtest(const struct shell *shel, size_t argc, char **argv)
{
	/* $write_blk 0xc0000 0xdeadbeef 16 */
	uint32_t pattern;
	uint32_t addr;
	uint32_t num_words;
	uint32_t offset;
	uint32_t *buff, *rxbuff;
	int i;
	int32_t rem_words;
	uint32_t test_chunk, chunk_no = 0;

	addr = strtoul(argv[1], NULL, 0);
	pattern = strtoul(argv[2], NULL, 0);
	offset = strtoul(argv[3], NULL, 0);
	num_words = strtoul(argv[4], NULL, 0);

	if (wifi_on_flag == 0) {
		shell_print(shel, "Err!! Please run wifi_on first");
		return -1;
	}

	if (argc != 5) {
		shell_print(shel, "incorrect arguments!!");
		shell_print(shel, "$ wifiutils memtest <start_addr> "
				   "<start_pattern> <pattern_incr> <num_words>");
		return -1;
	}

	if (!rpu_validate_addr(addr, num_words * 4, &hl_flag)) {
		return -1;
	}

	buff = (uint32_t *)k_malloc(2000 * 4);
	rxbuff = (uint32_t *)k_malloc(2000 * 4);

	rem_words = num_words;
	test_chunk = chunk_no = 0;

	while (rem_words > 0) {
		test_chunk = (rem_words < 2000) ? rem_words : 2000;

		for (i = 0; i < test_chunk; i++) {
			buff[i] = pattern + (i + chunk_no * 2000) * offset;
		}

		addr = addr + chunk_no * 2000;

		if (rpu_write(addr, buff, test_chunk * 4) ||
		    rpu_read(addr, rxbuff, test_chunk * 4)) {
			goto err;
		}

		if (memcmp(buff, rxbuff, test_chunk * 4) != 0) {
			goto err;
		}
		rem_words -= 2000;
		chunk_no++;
	}

	shell_print(shel, "memtest PASSED");

	k_free(rxbuff);
	k_free(buff);

	return SHELL_OK;
err:
	shell_print(shel, "memtest failed");

	k_free(rxbuff);
	k_free(buff);

	return SHELL_FAIL;
}

static int cmd_sleep_stats(const struct shell *shel, size_t argc, char **argv)
{
	uint32_t addr;
	uint32_t wrd_len;
	uint32_t *buff;

	addr = strtoul(argv[1], NULL, 0);
	wrd_len = strtoul(argv[2], NULL, 0);

	if (wifi_on_flag == 0) {
		shell_print(shel, "Err!! Please run wifi_on first");
		return -1;
	}

	if (argc != 3) {
		shell_print(shel, "incorrect arguments!!");
		shell_print(shel, "$ wifiutils sleep_stats <addr> <num_words>");
		return -1;
	}

	if (!rpu_validate_addr(addr, wrd_len * 4, &hl_flag)) {
		return -1;
	}
#ifdef CONFIG_WIFI_NRF71
	if (selected_blk == CODERAM) {
#else  /* CONFIG_WIFI_NRF71 */
	if ((selected_blk == LMAC_ROM) || (selected_blk == UMAC_ROM)) {
#endif /* !CONFIG_WIFI_NRF71 */
		shell_print(shel, "Error... Cannot write to ROM blocks");
		return -1;
	}

	buff = (uint32_t *)k_malloc(wrd_len * 4);

	rpu_get_sleep_stats(addr, buff, wrd_len);

	for (int i = 0; i < wrd_len; i++) {
		shell_print(shel, "0x%08x\n", buff[i]);
	}

	k_free(buff);
	return SHELL_OK;
}

static int cmd_gpio_config(const struct shell *shel, size_t argc, char **argv)
{
	return shell_ret(rpu_gpio_config());
}

static int cmd_pwron(const struct shell *shel, size_t argc, char **argv)
{
	return shell_ret(rpu_pwron());
}

static int cmd_qspi_init(const struct shell *shel, size_t argc, char **argv)
{
	return shell_ret(rpu_init());
}

static int cmd_rpuwake(const struct shell *shel, size_t argc, char **argv)
{
	return shell_ret(rpu_wakeup());
}

static int cmd_wrsr2(const struct shell *shel, size_t argc, char **argv)
{
	return shell_ret(rpu_wrsr2(strtoul(argv[1], NULL, 0) & 0xff));
}

static int cmd_rdsr2(const struct shell *shel, size_t argc, char **argv)
{
	uint8_t val = rpu_rdsr2();

	shell_print(shel, "RDSR2 = 0x%x\n", val);

	return SHELL_OK;
}

static int cmd_rdsr1(const struct shell *shel, size_t argc, char **argv)
{
	uint8_t val = rpu_rdsr1();

	shell_print(shel, "RDSR1 = 0x%x\n", val);

	return SHELL_OK;
}

static int cmd_rpuclks_on(const struct shell *shel, size_t argc, char **argv)
{
	return shell_ret(rpu_clks_on());
}

void dummy_irq_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("\n !!! IRQ Hit !!!!\n");
}

static int cmd_wifi_on(const struct shell *shel, size_t argc, char **argv)
{
	int ret;

	/* rpu_disable(); */

	dev = qspi_dev();

	ret = rpu_init();

	if (ret) {
		shell_print(shel, "%s: RPU init failed with error %d", __func__, ret);
		return -1;
	}

	ret = dev->init(qspi_defconfig());
	if (ret) {
		shell_print(shel, "%s: QSPI device init failed", __func__);
		return -1;
	}

	ret = rpu_enable();
	if (ret) {
		shell_print(shel, "%s: RPU enable failed with error %d", __func__, ret);
		return -1;
	}
	k_sleep(K_MSEC(10));

	shell_print(shel, "Wi-Fi ON done");
	wifi_on_flag = 1;

	return SHELL_OK;
}

static int cmd_wifi_off(const struct shell *shel, size_t argc, char **argv)
{
	int ret;

	ret = rpu_disable();

	if (ret) {
		shell_print(shel, "Wi-Fi OFF failed...");
		return SHELL_FAIL;
	}

	shell_print(shel, "Wi-Fi OFF done");
	wifi_on_flag = 0;
	return SHELL_OK;
}

static int cmd_memmap(const struct shell *shel, size_t argc, char **argv)
{
	print_memmap(shel);
	return SHELL_OK;
}

enum memtest_status {
	MEMTEST_STATUS_PASS = 0,
	MEMTEST_STATUS_FAIL = 1,
};

/* In Wezen PKTRAM is DATARAM as per document */

#define PKTRAM_START 0x200000
#define PKTRAM_END   0x2FFFFF

static int cmd_straddled_memtest(const struct shell *shel, size_t argc, char **argv)
{
	enum memtest_status status = MEMTEST_STATUS_FAIL;
	unsigned int addr = 0;
	unsigned int val = 0;
	unsigned int i = 0;
	unsigned int init_val = 0;
	unsigned int num_times;

	if (wifi_on_flag == 0) {
		shell_print(shel, "Err!! Please run wifi_on first");
		return -1;
	}

	if (argc != 2) {
		shell_print(shel, "incorrect arguments!!");
		shell_print(shel, "$ wifiutils straddled_memtest <num_iterations> ");
		return -1;
	}

	num_times = strtoul(argv[1], NULL, 0);

	for (i = 0; i < num_times; i++) {
		printk("%s: Memory test iteration %d\n", __func__, i + 1);

		for (addr = PKTRAM_START; addr < PKTRAM_END; addr += 4) {
			/* status = hal_rpu_mem_write(fmac_dev_ctx->hal_dev_ctx,
			 * addr, &init_val, sizeof(init_val));
			 */

			status = rpu_write(addr, &init_val, sizeof(init_val));
			if (status != MEMTEST_STATUS_PASS) {
				printk("%s: Memory write failed at address 0x%x\n", __func__, addr);
				goto out;
			}
		}

		for (addr = PKTRAM_START; addr < PKTRAM_END; addr += 4) {
			/* status = hal_rpu_mem_write(fmac_dev_ctx->hal_dev_ctx,
			 * addr, &addr, sizeof(addr));
			 */
			status = rpu_write(addr, &addr, sizeof(addr));
			if (status != MEMTEST_STATUS_PASS) {
				printk("%s: Memory write failed at address 0x%x\n", __func__, addr);
				goto out;
			}

			/* status = hal_rpu_mem_read(fmac_dev_ctx->hal_dev_ctx,
			 * &val, addr, sizeof(val));
			 */
			status = rpu_read(addr, &val, sizeof(val));
			if (status != MEMTEST_STATUS_PASS) {
				printk("%s: Memory read failed at address 0x%x\n", __func__, addr);
				goto out;
			}

			if (val != addr) {
				printk("%s: Memory read value (0x%X) mismatch at address 0x%X\n",
				       __func__, val, addr);
				status = MEMTEST_STATUS_FAIL;
				goto out;
			}
		}
	}

	status = MEMTEST_STATUS_PASS;

out:
	return status;
}

static void cmd_help(const struct shell *shel, size_t argc, char **argv)
{
	shell_print(shel, "Supported commands....  ");
	shell_print(shel, "=========================  ");
	shell_print(shel, "uart:~$ wifiutils read_wrd    <address> ");
	shell_print(shel, "         ex: $ wifiutils read_wrd 0x200000");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils write_wrd   <address> <data>");
	shell_print(shel, "         ex: $ wifiutils write_wrd 0x200000 0xabcd1234");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils read_blk    <address> <num_words>");
	shell_print(shel, "         ex: $ wifiutils read_blk 0x200000 64");
	shell_print(shel, "         Note - num_words can be a maximum of 2000");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils write_blk   <address> <start_pattern> "
			   "<pattern_increment> <num_words>");
	shell_print(shel, "         ex: $ wifiutils write_blk 0x200000 0xaaaa5555 0 64");
	shell_print(
		shel,
		"         This writes pattern 0xaaaa5555 to 64 locations starting from 0x200000");
	shell_print(shel, "         ex: $ wifiutils write_blk 0x200000 0x0 1 64");
	shell_print(shel, "         This writes pattern 0x0, 0x1,0x2,0x3....etc to 64 locations "
			   "starting from 0x200000");
	shell_print(shel, "         Note - num_words can be a maximum of 2000");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils memtest   <address> <start_pattern> "
			   "<pattern_increment> <num_words>");
	shell_print(shel, "         ex: $ wifiutils memtest 0x200000 0xaaaa5555 0 64");
	shell_print(
		shel,
		"         This writes pattern 0xaaaa5555 to 64 locations starting from 0x200000,");
	shell_print(shel, "         reads them back and validates them");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils straddled_memtest  <num_iterations>");
	shell_print(shel, "uart:~$ wifiutils wifi_on  ");
#if CONFIG_NRF70_ON_QSPI
	shell_print(shel, "         - Configures all gpio pins ");
	shell_print(shel, "         - Writes 1 to BUCKEN (P0.12), waits for 2ms and then "
			   "writes 1 to IOVDD Control (P0.31) ");
	shell_print(shel, "         - Initializes qspi interface and wakes up RPU");
	shell_print(shel, "         - Enables all gated RPU clocks");
#else
	shell_print(shel, "         - Configures all gpio pins ");
	shell_print(shel, "         - Writes 1 to BUCKEN (P1.01), waits for 2ms and then writes "
			   " 1 to IOVDD Control (P1.00) ");
	shell_print(shel, "         - Initializes qspi interface and wakes up RPU");
	shell_print(shel, "         - Enables all gated RPU clocks");
#endif
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils wifi_off ");
#if CONFIG_NRF70_ON_QSPI
	shell_print(shel, "         This writes 0 to IOVDD Control (P0.31) and then writes "
			   "0 to BUCKEN Control (P0.12)");
#else
	shell_print(shel, "         This writes 0 to IOVDD Control (P1.00) and then writes "
			   "0 to BUCKEN Control (P1.01)");
#endif
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils sleep_stats ");
	shell_print(shel,
		    "        This continuously does the RPU sleep/wake cycle and displays stats ");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils gpio_config ");
#if CONFIG_NRF70_ON_QSPI
	shell_print(shel, "         Configures BUCKEN(P0.12) as o/p, IOVDD control (P0.31) as "
			   "output and HOST_IRQ (P0.23) as input");
	shell_print(shel, "         and interruptible with a ISR hooked to it");
#else
	shell_print(shel, "         Configures BUCKEN(P1.01) as o/p, IOVDD control (P1.00) as "
			   "output and HOST_IRQ (P1.09) as input");
	shell_print(shel, "         and interruptible with a ISR hooked to it");
#endif
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils qspi_init ");
	shell_print(shel, "         Initializes QSPI driver functions ");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils pwron ");
	shell_print(shel, "         Sets BUCKEN=1, delay, IOVDD cntrl=1 ");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils rpuwake ");
	shell_print(shel, "         Wakeup RPU: Write 0x1 to WRSR2 register");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils rpuclks_on ");
	shell_print(shel, "         Enables all gated RPU clocks. Only SysBUS and PKTRAM "
			   "will work w/o this setting enabled");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils wrsr2 <val> ");
	shell_print(shel, "         writes <val> (0/1) to WRSR2 reg - takes LSByte of <val>");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils rdsr1 ");
	shell_print(shel, "         Reads RDSR1 Register");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils rdsr2 ");
	shell_print(shel, "         Reads RDSR2 Register");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils trgirq ");
	shell_print(shel, "         Generates IRQ interrupt to host");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils clrirq ");
	shell_print(shel, "         Clears host IRQ generated interrupt");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils xtal_clkout ");
	shell_print(shel, "         Loops XTAL clock out for calibration reference");
	shell_print(shel, "  ");
	shell_print(shel,
		    "uart:~$ wifiutils config  <qspi/spi Freq> <mem_block_num> <read_latency>");
	shell_print(shel, "         QSPI/SPI clock freq in MHz : 4/8/16 etc");
	shell_print(shel, "         block num as per memmap (starting from 0) : 0-10");
	shell_print(shel, "         QSPI/SPIM read latency for the selected block : 0-255");
	shell_print(
		shel,
		"       NOTE: need to do a wifi_off and wifi_on for these changes to take effect");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils ver ");
	shell_print(shel, "         Display SW version and other details of the hex file ");
	shell_print(shel, "  ");
	shell_print(shel, "uart:~$ wifiutils help ");
	shell_print(shel, "         Lists all commands with usage example(s) ");
	shell_print(shel, "  ");
}

static int cmd_ver(const struct shell *shel, size_t argc, char **argv)
{
	shell_print(shel, "wifiutils Version: %s", SW_VER);
#if CONFIG_NRF70_ON_QSPI
	shell_print(shel, "Build for QSPI interface on nRF7002 board");
#else
	shell_print(shel,
		    "Build for SPIM interface on nRF7002EK+nRF5340DK connected via arduino header");
#endif
	return SHELL_OK;
}

static int cmd_trgirq(const struct shell *shel, size_t argc, char **argv)
{
	int i, ret;
	struct gpio_callback irq_callback_data;

	ret = rpu_irq_config(&irq_callback_data, dummy_irq_handler);
	if (ret) {
		return SHELL_FAIL;
	}

	static const uint32_t irq_regs[][2] = {
		{0x400, 0x20000}, {0x494, 0x80000000}, {0x484, 0x7fff7bee}};

	for (i = 0; i < ARRAY_SIZE(irq_regs); i++) {
		/* if (rpu_write(irq_regs[i][0], (const uint32_t *) irq_regs[i][1], 4)) { */
		rpu_write(irq_regs[i][0], (const uint32_t *)irq_regs[i][1], 4);
	}

	return SHELL_OK;
}

static int cmd_clrirq(const struct shell *shel, size_t argc, char **argv)
{
	shell_print(shel, "de-asserting IRQ to HOST");

	return shell_ret(rpu_write(0x488, (uint32_t *)0x80000000, 4));
}

static int cmd_xtal_clkout(const struct shell *shel, size_t argc, char **argv)
{
	uint32_t val;

	if (wifi_on_flag == 0) {
		shell_print(shel, "Err!! Please run wifi_on first");
		return -1;
	}

	shell_print(shel, "Looping XTAL clock out for reference");

	val = 0x10;
	rpu_write(0x2DC8, &val, 4);
	shell_print(shel, "1");

	val = 0x8F000045;
	rpu_write(0x080000, &val, 4);
	shell_print(shel, "2");

	val = 0x104D4F00;
	rpu_write(0x080004, &val, 4);
	shell_print(shel, "3");

	val = 0x00FA4000;
	rpu_write(0x080008, &val, 4);
	shell_print(shel, "4");

	val = 0x0;
	rpu_write(0x6024, &val, 4);
	shell_print(shel, "5");

	val = 0x20000000;
	rpu_write(0x6020, &val, 4);
	shell_print(shel, "6");

	return SHELL_OK;
}

/* Creating subcommands (level 1 command) array for command "demo". */
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_wifiutils,
	SHELL_CMD(write_blk, NULL,
		  "Writes a block of words to Sheliak host memory via QSPI interface",
		  cmd_write_blk),
	SHELL_CMD(read_blk, NULL,
		  "Reads a block of words from Sheliak host memory via QSPI interface",
		  cmd_read_blk),
	SHELL_CMD(write_wrd, NULL, "Writes a word to Sheliak host memory via QSPI interface",
		  cmd_write_wrd),
	SHELL_CMD(read_wrd, NULL, "Reads a word from Sheliak host memory via QSPI interface",
		  cmd_read_wrd),
	SHELL_CMD(wifi_on, NULL, "BUCKEN-IOVDD power ON", cmd_wifi_on),
	SHELL_CMD(wifi_off, NULL, "BUCKEN-IOVDD power OFF", cmd_wifi_off),
	SHELL_CMD(sleep_stats, NULL, "Tests Sleep/Wakeup cycles", cmd_sleep_stats),
	SHELL_CMD(gpio_config, NULL, "Configure all GPIOs", cmd_gpio_config),
	SHELL_CMD(qspi_init, NULL, "Initialize QSPI driver functions", cmd_qspi_init),
	SHELL_CMD(pwron, NULL, "BUCKEN=1, delay, IOVDD=1", cmd_pwron),
	SHELL_CMD(rpuwake, NULL, "Wakeup RPU: Write 0x1 to WRSR2 reg", cmd_rpuwake),
	SHELL_CMD(rpuclks_on, NULL, "Enable all RPU gated clocks", cmd_rpuclks_on),
	SHELL_CMD(wrsr2, NULL, "Write to WRSR2 register", cmd_wrsr2),
	SHELL_CMD(rdsr1, NULL, "Read RDSR1 register", cmd_rdsr1),
	SHELL_CMD(rdsr2, NULL, "Read RDSR2 register", cmd_rdsr2),
	SHELL_CMD(trgirq, NULL, "Generates IRQ interrupt to HOST", cmd_trgirq),
	SHELL_CMD(clrirq, NULL, "Clears generated Host IRQ interrupt", cmd_clrirq),
	SHELL_CMD(xtal_clkout, NULL, "Gives of XTAL Clock as reference", cmd_xtal_clkout),
	SHELL_CMD(memmap, NULL, "Gives the full memory map of the Sheliak chip", cmd_memmap),
	SHELL_CMD(memtest, NULL,
		  "Writes, reads back and validates "
		  "specified memory on Sheliak chip",
		  cmd_memtest),
	SHELL_CMD(straddled_memtest, NULL, "validates specified memory on Sheliak chip",
		  cmd_straddled_memtest),
	SHELL_CMD(ver, NULL, "Display SW version of the hex file", cmd_ver),
	SHELL_CMD(help, NULL, "Help with all supported commmands", cmd_help), SHELL_SUBCMD_SET_END);

/* Creating root (level 0) command "wifiutils" */
SHELL_CMD_REGISTER(wifiutils, &sub_wifiutils, "wifiutils commands", NULL);
