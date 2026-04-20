/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/kernel.h>
#if defined(CONFIG_CPU_LOAD)
#include <zephyr/debug/cpu_load.h>
#endif

/* Test Configuration */
#define MSPI_CONTROLLER_NODE	DT_BUS(DT_ALIAS(dev0))
#define MSPI_TARGET_DEVICE_NODE DT_ALIAS(dev0)

#define MSPI_CLOCK_FREQ		DT_PROP(MSPI_TARGET_DEVICE_NODE, mspi_max_frequency)
#define TOTAL_TRANSFER_SIZE CONFIG_SAMPLE_MSPI_THROUGHPUT_TOTAL_TRANSFER_SIZE

static inline uint8_t io_mode_data_lines(enum mspi_io_mode mode)
{
	switch (mode) {
	case MSPI_IO_MODE_DUAL:
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_DUAL_1_2_2:
		return 2;
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_QUAD_1_4_4:
		return 4;
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_OCTAL_1_1_8:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		return 8;
	case MSPI_IO_MODE_HEX:
	case MSPI_IO_MODE_HEX_8_8_16:
	case MSPI_IO_MODE_HEX_8_16_16:
		return 16;
	default:
		return 1;
	}
}

/* ideal bits/s = clk * data_lines, ideal bytes/s = clk * data_lines / 8 */
#define IDEAL_BYTES_PER_SEC \
	((uint64_t)MSPI_CLOCK_FREQ * io_mode_data_lines(dev_cfg.io_mode) / 8ULL)

#define IDEAL_THROUGHPUT_MBYTE	((double)IDEAL_BYTES_PER_SEC / (double)MB(1))

#define MAX_BUFFER_SIZE 2304
#define MAX_PACKETS 16

/* Buffer sizes to test (in bytes) */
static const uint32_t buffer_sizes[] = {
	4, 16, 64, 256, 1024, 1500, MAX_BUFFER_SIZE
};

/* Packet counts to test */
static const uint32_t packet_counts[] = {
	1, 4, MAX_PACKETS
};

/* Device references */
static const struct device *mspi_controller = DEVICE_DT_GET(MSPI_CONTROLLER_NODE);

/* Device ID from the target peripheral device node */
static const struct mspi_dev_id controller_id = MSPI_DEVICE_ID_DT(MSPI_TARGET_DEVICE_NODE);

/* Device configuration from devicetree */
static const struct mspi_dev_cfg dev_cfg = MSPI_DEVICE_CONFIG_DT(MSPI_TARGET_DEVICE_NODE);

static uint8_t tx_buffer[MAX_BUFFER_SIZE] __aligned(4);
static struct mspi_xfer_packet tx_packets[MAX_PACKETS];

/* Performance measurement variables */
static uint32_t transfer_start_cycles;
static uint32_t transfer_end_cycles;

struct test_result {
	uint16_t buffer_size;
	uint8_t packet_count;
	uint32_t transfer_cycles;	/* Store time in CPU cycles for high precision */
	double throughput_mbps;	/* Throughput in MB/s*/
#if defined(CONFIG_CPU_LOAD)
	double cpu_load_per;	  /* CPU load (%) */
#endif
	uint16_t efficiency;	/* (Actual xfer / ideal) * 100 (%) */
};

static struct test_result test_results[ARRAY_SIZE(buffer_sizes) * ARRAY_SIZE(packet_counts)];
static uint32_t test_result_index;
#if defined(CONFIG_CPU_LOAD)
static int cpu_load_per_mille;
#endif

/* Forward declarations */
static void setup_test_buffers(void);
static int run_single_test(uint32_t buffer_size, uint32_t packet_count);
static void calculate_and_store_results(uint32_t buffer_size,
					uint32_t packet_count,
					uint32_t num_transfers,
					uint32_t total_buffers,
					uint32_t transfer_cycles);
static void print_test_results(void);
static void print_summary_report(void);

/* Setup test buffers with pattern */
static void setup_test_buffers(void)
{
	for (uint32_t i = 0; i < sizeof(tx_buffer); i++) {
		tx_buffer[i] = (uint8_t)(i);
	}
}

/* Run a single test case */
static int run_single_test(uint32_t buffer_size,
			   uint32_t packet_count)
{
	int rc;
	uint32_t total_buffers = TOTAL_TRANSFER_SIZE / buffer_size;
	uint32_t remaining_bytes = TOTAL_TRANSFER_SIZE % buffer_size;

	if (remaining_bytes > 0) {
		total_buffers++; /* Need one more buffer for remaining bytes */
	}

	/* Calculate how many transfers we need to make */
	uint32_t num_transfers = (total_buffers + packet_count - 1) / packet_count;

	if (packet_count > MAX_PACKETS) {
		printk("Packet count too high: %u (max %u)\n", packet_count, MAX_PACKETS);
		return -EINVAL;
	}

	/* Setup packets */
	for (uint32_t i = 0; i < MAX_PACKETS; i++) {
		tx_packets[i].dir = MSPI_TX;
		tx_packets[i].cmd = 0x42;
		tx_packets[i].address = 0x12;
		tx_packets[i].data_buf = tx_buffer;
		/* tx_packets[i].num_bytes will be set inside the timing loop */
	}

	/* Setup tx and rx transfer descriptions */
	struct mspi_xfer tx_xfer = {
		.xfer_mode = MSPI_DMA,
		/* tx_xfer.packets will be set inside the timing loop */
		/* tx_xfer.num_packet will be set inside the timing loop */
		.timeout = 10000,
		.cmd_length = dev_cfg.cmd_length,
		.addr_length = dev_cfg.addr_length,
		.async = false,
		.tx_dummy = 0,
	};

	printk("Total buffers = %u, Remaining bytes = %u, number of transfers = %u\n",
		total_buffers, remaining_bytes, num_transfers);

	/* Start performance measurement AFTER all packet setup is complete */
	transfer_start_cycles = k_cycle_get_32();
#if defined(CONFIG_CPU_LOAD)
	(void)cpu_load_get(true); /* Reset CPU load measurement */
#endif

	for (uint32_t transfer_idx = 0; transfer_idx < num_transfers; transfer_idx++) {
		uint32_t start_buffer = transfer_idx * packet_count;
		uint32_t end_buffer = MIN(start_buffer + packet_count, total_buffers);
		uint32_t current_buffers = end_buffer - start_buffer;

		/* Only update the num_bytes field - everything else is pre-configured */
		for (uint32_t i = 0; i < current_buffers; i++) {
			uint32_t buffer_idx = start_buffer + i;
			uint32_t current_buffer_size = (buffer_idx == total_buffers - 1 &&
							remaining_bytes > 0) ?
							remaining_bytes : buffer_size;

			tx_packets[i].num_bytes = current_buffer_size;
		}
		tx_xfer.packets = tx_packets;
		tx_xfer.num_packet = current_buffers;

		rc = mspi_transceive(mspi_controller, &controller_id, &tx_xfer);
		if (rc != 0) {
			printk("Failed to start transfer chunk %u: %d\n", transfer_idx, rc);
			return rc;
		}
	}
#if defined(CONFIG_CPU_LOAD)
	cpu_load_per_mille = cpu_load_get(true); /* Get load in per mille, reset for next test */
#endif
	transfer_end_cycles = k_cycle_get_32();

	calculate_and_store_results(buffer_size, packet_count, num_transfers, total_buffers,
				    (uint32_t)(transfer_end_cycles - transfer_start_cycles));

	printk("Test completed: buf=%u, desc=%u, cycles=%u\n",
		buffer_size, packet_count,
		(unsigned int)(transfer_end_cycles - transfer_start_cycles));

	return 0;
}

static void calculate_and_store_results(uint32_t buffer_size,
					uint32_t packet_count,
					uint32_t num_transfers,
					uint32_t total_buffers,
					uint32_t transfer_cycles)
{
	if (test_result_index >= ARRAY_SIZE(test_results)) {
		printk("Test results array full\n");
		return;
	}

	struct test_result *result = &test_results[test_result_index];

	result->buffer_size = (uint16_t)buffer_size;
	result->packet_count = (uint8_t)packet_count;
	result->transfer_cycles = transfer_cycles;

	/* Calculate throughput in MB/s */
	uint64_t timer_freq = (uint64_t)sys_clock_hw_cycles_per_sec();
	double transfer_time_sec = (double)(transfer_cycles) / (double)timer_freq;

	result->throughput_mbps  = (double)(TOTAL_TRANSFER_SIZE +
		(num_transfers * (dev_cfg.addr_length + dev_cfg.cmd_length))) /
		MB(1) / transfer_time_sec;

	double efficiency = (double)(result->throughput_mbps)/IDEAL_THROUGHPUT_MBYTE;
	double estimated_latency_us = 1000000.0 * (transfer_time_sec * (1.0-efficiency)
				/ (double)total_buffers);

	printk("Total transfer time = %lf s\n", transfer_time_sec);
	printk("Throughput = %lf mb/s\n", result->throughput_mbps);
	printk("Efficiency = %lf %%\n", efficiency * 100);
	printk("Estimated latency us = %lf us\n", estimated_latency_us);

#if defined(CONFIG_CPU_LOAD)
	/* CPU load (value in per mille) */
	result->cpu_load_per = (double)cpu_load_per_mille / 10.0;
	printk("CPU Load: %.2f%%\n", result->cpu_load_per);
#endif
	test_result_index++;
}

static void print_test_results(void)
{
	printk("\n\n=== TEST RESULTS ===\n");

#if defined(CONFIG_CPU_LOAD)
	printk("%-8s %-8s %-12s %-12s %-12s %-12s\n",
	       "BufSize", "PktCnt", "Time", "Throughput", "Efficiency", "CPU Load");
	printk("%-8s %-8s %-12s %-12s %-12s %-12s\n",
	       "(bytes)", "", "(us)", "(MB/s)", "(%)", "(%)");
#else
	printk("%-8s %-8s %-12s %-12s %-12s\n",
	       "BufSize", "PktCnt", "Time", "Throughput", "Efficiency");
	printk("%-8s %-8s %-12s %-12s %-12s\n",
	       "(bytes)", "", "(us)", "(MB/s)", "(%)");
#endif
	printk("-----------------------------------------------------------------\n");

	for (uint32_t i = 0; i < test_result_index; i++) {
		struct test_result *result = &test_results[i];

#if defined(CONFIG_CPU_LOAD)
		printk("%-8u %-8u %-12u %-12.3f %-12.2f %-12.2f\n",
		result->buffer_size, result->packet_count,
		result->transfer_cycles, (double)result->throughput_mbps,
		(double)100.0*(double)result->throughput_mbps/IDEAL_THROUGHPUT_MBYTE,
		(double)result->cpu_load_per);
#else
		printk("%-8u %-8u %-12u %-12.3f %-12.2f\n",
		result->buffer_size, result->packet_count,
		result->transfer_cycles, (double)result->throughput_mbps,
		(double)100.0*(double)result->throughput_mbps/IDEAL_THROUGHPUT_MBYTE);
#endif
	}
}

/* Print summary report with scaling analysis */
static void print_summary_report(void)
{
	printk("\n\n=== SUMMARY REPORT ===\n");

	/* Find best performance configurations */
	double best_tx_throughput = 0.0;
	uint32_t best_tx_buf_size = 0;
	uint32_t best_tx_desc_count = 0;

	for (uint32_t i = 0; i < test_result_index; i++) {
		struct test_result *result = &test_results[i];

		if ((double)result->throughput_mbps > best_tx_throughput) {
			best_tx_throughput = (double)result->throughput_mbps;
			best_tx_buf_size = result->buffer_size;
			best_tx_desc_count = result->packet_count;
		}
	}

	printk("Best TX Performance: %.2f MB/s (buffer size=%u, packet count=%u)\n",
			best_tx_throughput, best_tx_buf_size, best_tx_desc_count);
}

int main(void)
{
	int rc;

	printk("=== MSPI Throughput Test ===\n");
#ifdef CONFIG_MSPI_DMA
	printk("%d Byte transfer with using DMA\n", TOTAL_TRANSFER_SIZE);
#else
	printk("%d Byte transfer with CPU-driven polling\n", TOTAL_TRANSFER_SIZE);
#endif
	printk("MSPI Clock: %u MHz\n", MSPI_CLOCK_FREQ / 1000000);

	/* Check device readiness */
	if (!device_is_ready(mspi_controller)) {
		printk("MSPI controller device not ready\n");
		return -ENODEV;
	}

	/* Setup test environment */
	setup_test_buffers();

	rc = mspi_dev_config(mspi_controller, &controller_id, MSPI_DEVICE_CONFIG_ALL, &dev_cfg);
	if (rc != 0) {
		printk("Failed to configure MSPI device: %d\n", rc);
		return rc;
	}

	printk("Starting throughput tests...\n");
	printk("Ideal throughput with a frequency of %d is %.3f MB/s\n", MSPI_CLOCK_FREQ,
	       (double) IDEAL_THROUGHPUT_MBYTE);

	printk("Total test cases: %zu\n", ARRAY_SIZE(test_results));

	/* Delay needed for cycle measurement as other background processes must finish */
	k_msleep(500);

	/* Run all test permutations */
	for (uint32_t buf_idx = 0; buf_idx < ARRAY_SIZE(buffer_sizes); buf_idx++) {
		for (uint32_t desc_idx = 0; desc_idx < ARRAY_SIZE(packet_counts); desc_idx++) {

			uint32_t buffer_size = buffer_sizes[buf_idx];
			uint32_t packet_count = packet_counts[desc_idx];

			printk("\nRunning test %u/%zu: buf=%u, desc=%u\n",
					test_result_index + 1,
					ARRAY_SIZE(test_results),
					buffer_size, packet_count);

			rc = run_single_test(buffer_size, packet_count);
			if (rc != 0) {
				printk("Test failed: %d\n", rc);
				/* Continue with other tests */
			}

			/* Small delay between tests */
			k_msleep(300);
		}
	}

	/* Release controller channel */
	rc = mspi_get_channel_status(mspi_controller, 0);
	if (rc != 0) {
		printk("mspi_get_channel_status() controller failed: %d\n", rc);
	}

	/* Print results */
	print_test_results();
	print_summary_report();

	printk("=== MSPI Throughput Test Complete ===\n");

	return 0;
}
