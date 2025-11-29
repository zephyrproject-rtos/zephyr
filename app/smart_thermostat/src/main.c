/*
 * Smart Thermostat Application with Fault Tolerance
 * 
 * This application demonstrates fault tolerance in a realistic smart thermostat
 * that monitors temperature sensors, controls HVAC systems, and communicates
 * over UART. It handles sensor failures, communication errors, and system faults.
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(smart_thermostat, LOG_LEVEL_INF);

/* Application configuration */
#define TEMP_SENSOR_POLL_MS      1000
#define UART_TX_INTERVAL_MS      5000
#define WATCHDOG_FEED_INTERVAL   3000
#define SENSOR_TIMEOUT_MS        500
#define TARGET_TEMP_C            22
#define TEMP_TOLERANCE_C         2

/* Simulated sensor data */
static int16_t current_temp = 20;
static bool hvac_heating = false;
static bool hvac_cooling = false;
static uint32_t sensor_read_count = 0;
static uint32_t uart_tx_count = 0;
static uint32_t fault_count = 0;

/* Flags to prevent repeated critical fault injections */
static bool memory_corruption_occurred = false;
static bool stack_overflow_occurred = false;

/* Thread stacks */
K_THREAD_STACK_DEFINE(sensor_thread_stack, 2048);
K_THREAD_STACK_DEFINE(comm_thread_stack, 2048);
K_THREAD_STACK_DEFINE(watchdog_thread_stack, 1024);

static struct k_thread sensor_thread_data;
static struct k_thread comm_thread_data;
static struct k_thread watchdog_thread_data;

/* Context structures for fault reporting */
struct sensor_fault_context {
	const char *sensor_name;
	uint32_t timeout_ms;
	uint32_t read_count;
};

struct comm_fault_context {
	const char *protocol;
	uint32_t expected_crc;
	uint32_t received_crc;
	uint32_t packet_id;
};

/**
 * @brief Recovery handler for peripheral timeout (sensor I2C timeout)
 */
static ft_recovery_result_t sensor_timeout_recovery(const ft_event_t *event)
{
	struct sensor_fault_context *ctx = 
		(struct sensor_fault_context *)event->context;

	LOG_ERR("=== SENSOR TIMEOUT RECOVERY ===");
	LOG_ERR("Sensor: %s timed out after %u ms", 
	        ctx->sensor_name, ctx->timeout_ms);
	LOG_ERR("Total sensor reads: %u", ctx->read_count);

	/* Recovery: Reset I2C bus and use last known good value */
	LOG_WRN("Resetting I2C bus and using last known temperature");
	LOG_INF("HVAC operating in safe mode with cached data");

	fault_count++;
	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for communication CRC errors
 */
static ft_recovery_result_t comm_crc_recovery(const ft_event_t *event)
{
	struct comm_fault_context *ctx = 
		(struct comm_fault_context *)event->context;

	LOG_WRN("=== COMM CRC ERROR RECOVERY ===");
	LOG_WRN("Protocol: %s, Packet ID: %u", ctx->protocol, ctx->packet_id);
	LOG_WRN("Expected CRC: 0x%08x, Received: 0x%08x",
	        ctx->expected_crc, ctx->received_crc);

	/* Recovery: Request retransmission */
	LOG_INF("Requesting packet %u retransmission", ctx->packet_id);

	fault_count++;
	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for watchdog bark (system overload)
 */
static ft_recovery_result_t watchdog_recovery(const ft_event_t *event)
{
	LOG_ERR("=== WATCHDOG BARK RECOVERY ===");
	LOG_ERR("System is not responding, feeding watchdog");
	
	/* In real system: identify and restart stuck thread */
	LOG_WRN("Emergency watchdog feed to prevent reset");

	fault_count++;
	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for application assertions
 */
static ft_recovery_result_t assertion_recovery(const ft_event_t *event)
{
	LOG_ERR("=== ASSERTION FAILURE RECOVERY ===");
	LOG_ERR("Application assertion failed, entering safe mode");
	
	/* Recovery: Disable HVAC and enter safe monitoring mode */
	hvac_heating = false;
	hvac_cooling = false;
	LOG_WRN("HVAC disabled, system in safe mode");

	fault_count++;
	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for deadlock detection
 */
static ft_recovery_result_t deadlock_recovery(const ft_event_t *event)
{
	LOG_ERR("=== DEADLOCK DETECTED ===");
	LOG_ERR("Thread deadlock detected - killing victim thread");
	
	/* In real system: kill deadlocked thread and restart */
	LOG_WRN("Restarting sensor thread to break deadlock");

	fault_count++;
	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for memory corruption
 */
static ft_recovery_result_t memory_corruption_recovery(const ft_event_t *event)
{
	LOG_ERR("=== MEMORY CORRUPTION DETECTED ===");
	LOG_ERR("Critical data structure corrupted");
	
	/* Disable HVAC for safety */
	hvac_heating = false;
	hvac_cooling = false;
	LOG_WRN("Entering safe mode - no further fault injections");
	memory_corruption_occurred = true;

	fault_count++;
	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for stack overflow
 */
static ft_recovery_result_t stack_overflow_recovery(const ft_event_t *event)
{
	LOG_ERR("=== STACK OVERFLOW DETECTED ===");
	LOG_ERR("Thread stack overflow - system unstable");
	
	hvac_heating = false;
	hvac_cooling = false;
	LOG_WRN("Entering safe mode - no further fault injections");
	stack_overflow_occurred = true;

	fault_count++;
	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Simulate temperature sensor read with occasional timeouts
 */
static int read_temperature_sensor(int16_t *temp)
{
	sensor_read_count++;

	/* Simulate occasional sensor timeout (5% failure rate) */
	if ((sys_rand32_get() % 100) < 5) {
		LOG_ERR("Sensor I2C timeout!");
		
		struct sensor_fault_context ctx = {
			.sensor_name = "BME280_Temperature",
			.timeout_ms = SENSOR_TIMEOUT_MS,
			.read_count = sensor_read_count
		};

		ft_event_t event = {
			.kind = FT_PERIPH_TIMEOUT,
			.severity = FT_SEV_ERROR,
			.domain = FT_DOMAIN_HARDWARE,
			.code = 0x6001,
			.timestamp = k_uptime_get(),
			.thread_id = k_current_get(),
			.context = &ctx,
			.context_size = sizeof(ctx)
		};

		ft_report_fault(&event);
		return -ETIMEDOUT;
	}

	/* Simulate temperature drift */
	int16_t drift = (sys_rand32_get() % 5) - 2;
	current_temp += drift;
	
	/* Keep temperature in realistic range */
	if (current_temp < 15) current_temp = 15;
	if (current_temp > 30) current_temp = 30;

	*temp = current_temp;
	return 0;
}

/**
 * @brief Control HVAC based on temperature
 */
static void control_hvac(int16_t temp)
{
	if (temp < TARGET_TEMP_C - TEMP_TOLERANCE_C) {
		/* Too cold - turn on heating */
		if (!hvac_heating) {
			hvac_heating = true;
			hvac_cooling = false;
			LOG_INF("HVAC: Heating ON (temp: %d°C)", temp);
		}
	} else if (temp > TARGET_TEMP_C + TEMP_TOLERANCE_C) {
		/* Too hot - turn on cooling */
		if (!hvac_cooling) {
			hvac_cooling = true;
			hvac_heating = false;
			LOG_INF("HVAC: Cooling ON (temp: %d°C)", temp);
		}
	} else {
		/* In acceptable range - turn off HVAC */
		if (hvac_heating || hvac_cooling) {
			hvac_heating = false;
			hvac_cooling = false;
			LOG_INF("HVAC: OFF (temp: %d°C in range)", temp);
		}
	}
}

/**
 * @brief Validate temperature reading
 */
static bool validate_temperature(int16_t temp)
{
	/* Check for physically impossible values */
	if (temp < -40 || temp > 80) {
		LOG_ERR("Temperature %d°C is out of valid range!", temp);
		
		struct assert_context {
			const char *file;
			uint32_t line;
			const char *condition;
			const char *message;
		} ctx = {
			.file = "smart_thermostat.c",
			.line = 200,
			.condition = "temp >= -40 && temp <= 80",
			.message = "Temperature reading out of physical range"
		};

		ft_event_t event = {
			.kind = FT_APP_ASSERT,
			.severity = FT_SEV_ERROR,
			.domain = FT_DOMAIN_APPLICATION,
			.code = 0x9001,
			.timestamp = k_uptime_get(),
			.thread_id = k_current_get(),
			.context = &ctx,
			.context_size = sizeof(ctx)
		};

		ft_report_fault(&event);
		return false;
	}
	return true;
}

/**
 * @brief Sensor monitoring thread
 */
static void sensor_thread_entry(void *p1, void *p2, void *p3)
{
	int16_t temp;
	int ret;

	LOG_INF("Sensor monitoring thread started");
	uint32_t iteration = 0;

	while (1) {
		iteration++;
		
		/* Simulate memory corruption detection (0.2% chance) */
		if (!memory_corruption_occurred && (sys_rand32_get() % 1000) < 2) {
			LOG_ERR("Memory corruption detected in temperature buffer!");
			
			struct mem_corruption_context {
				uint32_t expected_checksum;
				uint32_t actual_checksum;
				const char *location;
			} ctx = {
				.expected_checksum = 0xDEADBEEF,
				.actual_checksum = 0xDEAD0000,
				.location = "temperature_buffer"
			};

			ft_event_t event = {
				.kind = FT_MEM_CORRUPTION,
				.severity = FT_SEV_CRITICAL,
				.domain = FT_DOMAIN_SYSTEM,
				.code = 0x5001,
				.timestamp = k_uptime_get(),
				.thread_id = k_current_get(),
				.context = &ctx,
				.context_size = sizeof(ctx)
			};

			ft_report_fault(&event);
		}
		
		ret = read_temperature_sensor(&temp);
		
		if (ret == 0) {
			/* Successful read - validate and control HVAC */
			if (validate_temperature(temp)) {
				control_hvac(temp);
				LOG_DBG("Temperature: %d°C (target: %d°C)", 
				        temp, TARGET_TEMP_C);
			}
		} else {
			LOG_WRN("Using last known temperature: %d°C", current_temp);
			/* Continue with last known value */
		}

		k_msleep(TEMP_SENSOR_POLL_MS);
	}
}

/**
 * @brief Simulate UART transmission with CRC errors
 */
static int transmit_status_uart(void)
{
	uart_tx_count++;

	/* Simulate occasional CRC error (3% failure rate) */
	if ((sys_rand32_get() % 100) < 3) {
		LOG_ERR("UART CRC mismatch!");
		
		struct comm_fault_context ctx = {
			.protocol = "UART",
			.expected_crc = 0x12345678,
			.received_crc = 0x12345600,
			.packet_id = uart_tx_count
		};

		ft_event_t event = {
			.kind = FT_COMM_CRC_ERROR,
			.severity = FT_SEV_WARNING,
			.domain = FT_DOMAIN_COMMUNICATION,
			.code = 0x7001,
			.timestamp = k_uptime_get(),
			.thread_id = k_current_get(),
			.context = &ctx,
			.context_size = sizeof(ctx)
		};

		ft_report_fault(&event);
		return -EIO;
	}

	LOG_DBG("Status transmitted: Temp=%d°C, Heating=%d, Cooling=%d",
	        current_temp, hvac_heating, hvac_cooling);
	return 0;
}

/**
 * @brief Communication thread
 */
static void comm_thread_entry(void *p1, void *p2, void *p3)
{
	LOG_INF("Communication thread started");

	while (1) {
		/* Transmit status over UART */
		if (transmit_status_uart() != 0) {
			LOG_WRN("Retrying transmission...");
			k_msleep(100);
			transmit_status_uart(); /* Retry */
		}

		k_msleep(UART_TX_INTERVAL_MS);
	}
}

/**
 * @brief Watchdog feeding thread
 */
static void watchdog_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t feed_count = 0;

	LOG_INF("Watchdog thread started");

	while (1) {
		feed_count++;
		
		/* Simulate deadlock detection (0.5% chance) - skip if in safe mode */
		if (!memory_corruption_occurred && !stack_overflow_occurred && (sys_rand32_get() % 1000) < 5) {
			LOG_ERR("Circular dependency detected between threads!");
			
			struct deadlock_context {
				const char *thread1;
				const char *thread2;
				const char *resource;
			} ctx = {
				.thread1 = "sensor_thread",
				.thread2 = "comm_thread",
				.resource = "i2c_bus"
			};

			ft_event_t event = {
				.kind = FT_DEADLOCK_DETECTED,
				.severity = FT_SEV_ERROR,
				.domain = FT_DOMAIN_SYSTEM,
				.code = 0x4001,
				.timestamp = k_uptime_get(),
				.thread_id = k_current_get(),
				.context = &ctx,
				.context_size = sizeof(ctx)
			};

			ft_report_fault(&event);
		}
		
		/* Simulate missed watchdog feed (1% chance) */
		if ((sys_rand32_get() % 100) < 1) {
			LOG_WRN("Watchdog feed delayed!");
			
			struct watchdog_context {
				uint32_t bark_timeout_ms;
				uint32_t missed_feeds;
				const char *thread;
			} ctx = {
				.bark_timeout_ms = 5000,
				.missed_feeds = 1,
				.thread = "watchdog_thread"
			};

			ft_event_t event = {
				.kind = FT_WATCHDOG_BARK,
				.severity = FT_SEV_ERROR,
				.domain = FT_DOMAIN_SYSTEM,
				.code = 0x3001,
				.timestamp = k_uptime_get(),
				.thread_id = k_current_get(),
				.context = &ctx,
				.context_size = sizeof(ctx)
			};

			ft_report_fault(&event);
			k_msleep(500); /* Simulate delay */
		}

		LOG_DBG("Watchdog fed (count: %u)", feed_count);
		k_msleep(WATCHDOG_FEED_INTERVAL);
	}
}

/**
 * @brief Display system status
 */
static void display_status(void)
{
	ft_statistics_t stats;

	LOG_INF("========================================");
	LOG_INF("SMART THERMOSTAT STATUS");
	LOG_INF("========================================");
	LOG_INF("Current Temperature: %d°C", current_temp);
	LOG_INF("Target Temperature: %d°C", TARGET_TEMP_C);
	LOG_INF("HVAC Heating: %s", hvac_heating ? "ON" : "OFF");
	LOG_INF("HVAC Cooling: %s", hvac_cooling ? "ON" : "OFF");
	LOG_INF("Sensor Reads: %u", sensor_read_count);
	LOG_INF("UART Transmissions: %u", uart_tx_count);
	LOG_INF("Total Faults Handled: %u", fault_count);

	if (ft_get_statistics(&stats) == 0) {
		LOG_INF("----------------------------------------");
		LOG_INF("FAULT TOLERANCE STATISTICS");
		LOG_INF("----------------------------------------");
		LOG_INF("Total Faults: %u", stats.total_faults);
		LOG_INF("Successful Recoveries: %u", stats.recoveries_successful);
		LOG_INF("Failed Recoveries: %u", stats.recoveries_failed);
		LOG_INF("Peripheral Timeouts: %u", stats.fault_counts[FT_PERIPH_TIMEOUT]);
		LOG_INF("CRC Errors: %u", stats.fault_counts[FT_COMM_CRC_ERROR]);
		LOG_INF("Watchdog Barks: %u", stats.fault_counts[FT_WATCHDOG_BARK]);
		LOG_INF("Assertions: %u", stats.fault_counts[FT_APP_ASSERT]);
	}
	LOG_INF("========================================");
}

/**
 * @brief Main application
 */
int main(void)
{
	LOG_INF("========================================");
	LOG_INF("Smart Thermostat with Fault Tolerance");
	LOG_INF("========================================");

	/* Initialize fault tolerance */
	ft_init();
	ft_register_handler(FT_PERIPH_TIMEOUT, sensor_timeout_recovery);
	ft_register_handler(FT_COMM_CRC_ERROR, comm_crc_recovery);
	ft_register_handler(FT_WATCHDOG_BARK, watchdog_recovery);
	ft_register_handler(FT_APP_ASSERT, assertion_recovery);
	ft_register_handler(FT_DEADLOCK_DETECTED, deadlock_recovery);
	ft_register_handler(FT_MEM_CORRUPTION, memory_corruption_recovery);
	ft_register_handler(FT_STACK_OVERFLOW, stack_overflow_recovery);

	LOG_INF("Fault tolerance handlers registered");
	LOG_INF("Target temperature: %d°C", TARGET_TEMP_C);

	/* Create application threads */
	k_thread_create(&sensor_thread_data, sensor_thread_stack,
	                K_THREAD_STACK_SIZEOF(sensor_thread_stack),
	                sensor_thread_entry, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
	k_thread_name_set(&sensor_thread_data, "sensor");

	k_thread_create(&comm_thread_data, comm_thread_stack,
	                K_THREAD_STACK_SIZEOF(comm_thread_stack),
	                comm_thread_entry, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_thread_name_set(&comm_thread_data, "comm");

	k_thread_create(&watchdog_thread_data, watchdog_thread_stack,
	                K_THREAD_STACK_SIZEOF(watchdog_thread_stack),
	                watchdog_thread_entry, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(6), 0, K_NO_WAIT);
	k_thread_name_set(&watchdog_thread_data, "watchdog");

	LOG_INF("All threads started - system operational");

	/* Main loop - display status periodically */
	while (1) {
		k_sleep(K_SECONDS(10));
		display_status();
	}

	return 0;
}
