/*
 * Industrial Motor Controller with Fault Tolerance
 * 
 * This application demonstrates fault tolerance in an industrial motor control
 * system. It monitors motor current, temperature, and position while handling
 * overcurrent conditions, sensor failures, and communication faults.
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(motor_controller, LOG_LEVEL_INF);

/* Motor control parameters */
#define MOTOR_CURRENT_LIMIT_MA   5000
#define MOTOR_TEMP_LIMIT_C       80
#define MOTOR_SPEED_MAX_RPM      3000
#define POSITION_UPDATE_MS       100
#define CURRENT_MONITOR_MS       50
#define TEMP_MONITOR_MS          500
#define MODBUS_POLL_MS           1000

/* System state */
struct motor_state {
	bool enabled;
	int32_t current_ma;          /* Motor current in milliamps */
	int32_t temperature_c;       /* Motor temperature in Celsius */
	int32_t speed_rpm;           /* Motor speed in RPM */
	int32_t position_deg;        /* Motor position in degrees */
	uint32_t total_runtime_s;    /* Total runtime in seconds */
	uint32_t fault_trips;        /* Number of fault trips */
};

static struct motor_state motor = {
	.enabled = true,
	.current_ma = 0,
	.temperature_c = 25,
	.speed_rpm = 0,
	.position_deg = 0,
	.total_runtime_s = 0,
	.fault_trips = 0
};

/* Thread stacks */
K_THREAD_STACK_DEFINE(current_monitor_stack, 2048);
K_THREAD_STACK_DEFINE(temp_monitor_stack, 2048);
K_THREAD_STACK_DEFINE(position_control_stack, 2048);
K_THREAD_STACK_DEFINE(modbus_comm_stack, 2048);

static struct k_thread current_monitor_data;
static struct k_thread temp_monitor_data;
static struct k_thread position_control_data;
static struct k_thread modbus_comm_data;

/* Mutex for motor state access */
K_MUTEX_DEFINE(motor_mutex);

/**
 * @brief Recovery handler for overcurrent condition
 */
static ft_recovery_result_t overcurrent_recovery(const ft_event_t *event)
{
	LOG_ERR("=== OVERCURRENT PROTECTION TRIGGERED ===");
	
	/* Immediately disable motor */
	k_mutex_lock(&motor_mutex, K_FOREVER);
	motor.enabled = false;
	motor.fault_trips++;
	k_mutex_unlock(&motor_mutex);

	LOG_ERR("Motor current: %d mA (limit: %d mA)", 
	        motor.current_ma, MOTOR_CURRENT_LIMIT_MA);
	LOG_WRN("Motor DISABLED for safety");
	LOG_INF("Manual reset required to re-enable motor");

	/* In real system: trigger hardware e-stop */
	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for thermal overload
 */
static ft_recovery_result_t thermal_recovery(const ft_event_t *event)
{
	LOG_ERR("=== THERMAL OVERLOAD PROTECTION ===");
	
	k_mutex_lock(&motor_mutex, K_FOREVER);
	motor.enabled = false;
	motor.fault_trips++;
	k_mutex_unlock(&motor_mutex);

	LOG_ERR("Motor temperature: %d°C (limit: %d°C)", 
	        motor.temperature_c, MOTOR_TEMP_LIMIT_C);
	LOG_WRN("Motor DISABLED - cooling required");
	LOG_INF("Will auto-restart when temperature < 60°C");

	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for sensor timeout
 */
static ft_recovery_result_t sensor_timeout_recovery(const ft_event_t *event)
{
	LOG_WRN("=== SENSOR TIMEOUT RECOVERY ===");
	LOG_WRN("Position encoder communication lost");
	
	/* Recovery: Switch to sensorless control mode */
	LOG_INF("Switching to sensorless FOC mode");
	LOG_INF("Position estimation using back-EMF");

	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for Modbus communication errors
 */
static ft_recovery_result_t modbus_recovery(const ft_event_t *event)
{
	LOG_WRN("=== MODBUS COMMUNICATION ERROR ===");
	LOG_WRN("CRC error in Modbus RTU packet");
	
	/* Recovery: Request retransmission */
	LOG_INF("Requesting packet retransmission");

	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for memory corruption
 */
static ft_recovery_result_t memory_corruption_recovery(const ft_event_t *event)
{
	LOG_ERR("=== MEMORY CORRUPTION DETECTED ===");
	
	/* Critical safety response */
	k_mutex_lock(&motor_mutex, K_FOREVER);
	motor.enabled = false;
	motor.fault_trips++;
	k_mutex_unlock(&motor_mutex);

	LOG_ERR("Motor DISABLED due to memory corruption");
	LOG_ERR("System requires restart for safety");

	return FT_RECOVERY_REBOOT_REQUIRED;
}

/**
 * @brief Recovery handler for hard fault
 */
static ft_recovery_result_t hardfault_recovery(const ft_event_t *event)
{
	LOG_ERR("=== HARD FAULT DETECTED ===");
	
	k_mutex_lock(&motor_mutex, K_FOREVER);
	motor.enabled = false;
	motor.fault_trips++;
	k_mutex_unlock(&motor_mutex);

	LOG_ERR("CPU exception - motor disabled for safety");
	LOG_ERR("Attempting to continue with reduced functionality");

	return FT_RECOVERY_FAILED;
}

/**
 * @brief Recovery handler for stack overflow
 */
static ft_recovery_result_t stack_overflow_recovery(const ft_event_t *event)
{
	LOG_ERR("=== STACK OVERFLOW DETECTED ===");
	
	k_mutex_lock(&motor_mutex, K_FOREVER);
	motor.enabled = false;
	motor.fault_trips++;
	k_mutex_unlock(&motor_mutex);

	LOG_ERR("Thread stack overflow detected");
	LOG_ERR("System unstable - reboot required");

	return FT_RECOVERY_REBOOT_REQUIRED;
}

/**
 * @brief Recovery handler for watchdog bark
 */
static ft_recovery_result_t watchdog_recovery(const ft_event_t *event)
{
	LOG_WRN("=== WATCHDOG BARK DETECTED ===");
	LOG_WRN("Control loop running slow - system overloaded");
	
	/* Recovery: feed watchdog and reduce load */
	LOG_INF("Emergency watchdog feed");
	LOG_INF("Reducing sensor polling rate");

	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Monitor motor current and detect overcurrent
 */
static void current_monitor_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Current monitoring thread started");

	while (1) {
		if (motor.enabled) {
			/* Simulate current draw based on speed */
			int32_t base_current = (motor.speed_rpm * 1000) / MOTOR_SPEED_MAX_RPM;
			int32_t variation = (sys_rand32_get() % 500) - 250;
			
			k_mutex_lock(&motor_mutex, K_FOREVER);
			motor.current_ma = base_current + variation;
			k_mutex_unlock(&motor_mutex);

			/* Check for overcurrent */
			if (motor.current_ma > MOTOR_CURRENT_LIMIT_MA) {
				LOG_ERR("OVERCURRENT DETECTED: %d mA", motor.current_ma);
				
				struct overcurrent_context {
					int32_t current_ma;
					int32_t limit_ma;
					int32_t speed_rpm;
				} ctx = {
					.current_ma = motor.current_ma,
					.limit_ma = MOTOR_CURRENT_LIMIT_MA,
					.speed_rpm = motor.speed_rpm
				};

				ft_event_t event = {
					.kind = FT_PERIPH_TIMEOUT,  /* Using as hardware fault */
					.severity = FT_SEV_CRITICAL,
					.domain = FT_DOMAIN_HARDWARE,
					.code = 0x6100,
					.timestamp = k_uptime_get(),
					.thread_id = k_current_get(),
					.context = &ctx,
					.context_size = sizeof(ctx)
				};

				ft_report_fault(&event);
			}
		} else {
			k_mutex_lock(&motor_mutex, K_FOREVER);
			motor.current_ma = 0;
			k_mutex_unlock(&motor_mutex);
		}

		k_msleep(CURRENT_MONITOR_MS);
	}
}

/**
 * @brief Monitor motor temperature
 */
static void temp_monitor_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Temperature monitoring thread started");

	while (1) {
		if (motor.enabled) {
			/* Temperature rises with current */
			int32_t heat_delta = motor.current_ma / 200;
			
			k_mutex_lock(&motor_mutex, K_FOREVER);
			motor.temperature_c += heat_delta / 10;
			motor.temperature_c -= 1;  /* Cooling */
			
			if (motor.temperature_c < 25) {
				motor.temperature_c = 25;
			}
			k_mutex_unlock(&motor_mutex);

			/* Check for thermal overload */
			if (motor.temperature_c > MOTOR_TEMP_LIMIT_C) {
				LOG_ERR("THERMAL OVERLOAD: %d°C", motor.temperature_c);
				
				struct thermal_context {
					int32_t temp_c;
					int32_t limit_c;
					int32_t current_ma;
				} ctx = {
					.temp_c = motor.temperature_c,
					.limit_c = MOTOR_TEMP_LIMIT_C,
					.current_ma = motor.current_ma
				};

				ft_event_t event = {
					.kind = FT_POWER_BROWNOUT,  /* Using as thermal event */
					.severity = FT_SEV_CRITICAL,
					.domain = FT_DOMAIN_HARDWARE,
					.code = 0x8100,
					.timestamp = k_uptime_get(),
					.thread_id = k_current_get(),
					.context = &ctx,
					.context_size = sizeof(ctx)
				};

				ft_report_fault(&event);
			}
			
			/* Simulate stack overflow detection (0.1% chance - very rare) */
			if ((sys_rand32_get() % 1000) < 1) {
				LOG_ERR("Stack overflow detected in temp monitor!");
				
				struct stack_context {
					const char *thread;
					uint32_t stack_size;
					uint32_t stack_used;
				} ctx = {
					.thread = "temp_mon",
					.stack_size = 2048,
					.stack_used = 2100
				};

				ft_event_t event = {
					.kind = FT_STACK_OVERFLOW,
					.severity = FT_SEV_CRITICAL,
					.domain = FT_DOMAIN_SYSTEM,
					.code = 0x1001,
					.timestamp = k_uptime_get(),
					.thread_id = k_current_get(),
					.context = &ctx,
					.context_size = sizeof(ctx)
				};

				ft_report_fault(&event);
			}
		} else {
			/* Cool down when disabled */
			k_mutex_lock(&motor_mutex, K_FOREVER);
			motor.temperature_c -= 2;
			if (motor.temperature_c < 25) {
				motor.temperature_c = 25;
			}
			k_mutex_unlock(&motor_mutex);

			/* Auto-restart after cooling */
			if (motor.temperature_c < 60 && motor.fault_trips > 0) {
				LOG_INF("Motor cooled to %d°C - ready for restart", 
				        motor.temperature_c);
				k_mutex_lock(&motor_mutex, K_FOREVER);
				motor.enabled = true;
				k_mutex_unlock(&motor_mutex);
			}
		}

		k_msleep(TEMP_MONITOR_MS);
	}
}

/**
 * @brief Position control thread
 */
static void position_control_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Position control thread started");
	uint32_t encoder_errors = 0;
	uint32_t control_cycles = 0;

	while (1) {
		control_cycles++;
		
		/* Simulate watchdog bark (system overload, 0.5% chance) */
		if ((sys_rand32_get() % 1000) < 5) {
			LOG_WRN("Control loop running slow - watchdog bark!");
			
			struct watchdog_context {
				uint32_t expected_interval_ms;
				uint32_t actual_interval_ms;
				const char *thread;
			} ctx = {
				.expected_interval_ms = POSITION_UPDATE_MS,
				.actual_interval_ms = POSITION_UPDATE_MS * 3,
				.thread = "pos_ctrl"
			};

			ft_event_t event = {
				.kind = FT_WATCHDOG_BARK,
				.severity = FT_SEV_WARNING,
				.domain = FT_DOMAIN_SYSTEM,
				.code = 0x3001,
				.timestamp = k_uptime_get(),
				.thread_id = k_current_get(),
				.context = &ctx,
				.context_size = sizeof(ctx)
			};

			ft_report_fault(&event);
		}
		
		if (motor.enabled) {
			/* Simulate encoder read with occasional timeout */
			if ((sys_rand32_get() % 100) < 2) {
				encoder_errors++;
				LOG_ERR("Encoder timeout (count: %u)", encoder_errors);
				
				struct sensor_context {
					const char *sensor_name;
					uint32_t timeout_ms;
					uint32_t error_count;
				} ctx = {
					.sensor_name = "Incremental_Encoder",
					.timeout_ms = 100,
					.error_count = encoder_errors
				};

				ft_event_t event = {
					.kind = FT_PERIPH_TIMEOUT,
					.severity = FT_SEV_WARNING,
					.domain = FT_DOMAIN_HARDWARE,
					.code = 0x6200,
					.timestamp = k_uptime_get(),
					.thread_id = k_current_get(),
					.context = &ctx,
					.context_size = sizeof(ctx)
				};

				ft_report_fault(&event);
			} else {
				/* Update position based on speed */
				int32_t delta_deg = (motor.speed_rpm * 6 * POSITION_UPDATE_MS) / 1000;
				
				k_mutex_lock(&motor_mutex, K_FOREVER);
				motor.position_deg += delta_deg;
				motor.position_deg %= 360;
				k_mutex_unlock(&motor_mutex);
			}

			/* Ramp speed up to target */
			if (motor.speed_rpm < 1500) {
				motor.speed_rpm += 50;
			}
		} else {
			/* Coast to stop */
			if (motor.speed_rpm > 0) {
				motor.speed_rpm -= 100;
				if (motor.speed_rpm < 0) {
					motor.speed_rpm = 0;
				}
			}
		}

		k_msleep(POSITION_UPDATE_MS);
	}
}

/**
 * @brief Modbus RTU communication thread
 */
static void modbus_comm_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Modbus communication thread started");
	uint32_t packet_id = 0;

	while (1) {
		packet_id++;

		/* Simulate Modbus CRC error (2% failure rate) */
		if ((sys_rand32_get() % 100) < 2) {
			LOG_ERR("Modbus CRC error on packet %u", packet_id);
			
			struct modbus_context {
				const char *protocol;
				uint32_t expected_crc;
				uint32_t received_crc;
				uint32_t packet_id;
			} ctx = {
				.protocol = "Modbus_RTU",
				.expected_crc = 0xABCD,
				.received_crc = 0xAB00,
				.packet_id = packet_id
			};

			ft_event_t event = {
				.kind = FT_COMM_CRC_ERROR,
				.severity = FT_SEV_WARNING,
				.domain = FT_DOMAIN_COMMUNICATION,
				.code = 0x7100,
				.timestamp = k_uptime_get(),
				.thread_id = k_current_get(),
				.context = &ctx,
				.context_size = sizeof(ctx)
			};

			ft_report_fault(&event);
		} else {
			LOG_DBG("Modbus status sent: Speed=%d RPM, Pos=%d°, I=%d mA",
			        motor.speed_rpm, motor.position_deg, motor.current_ma);
		}

		k_msleep(MODBUS_POLL_MS);
	}
}

/**
 * @brief Display comprehensive motor status
 */
static void display_motor_status(void)
{
	ft_statistics_t stats;

	LOG_INF("========================================");
	LOG_INF("INDUSTRIAL MOTOR CONTROLLER STATUS");
	LOG_INF("========================================");
	
	k_mutex_lock(&motor_mutex, K_FOREVER);
	LOG_INF("Motor State: %s", motor.enabled ? "ENABLED" : "DISABLED");
	LOG_INF("Speed: %d RPM (max: %d RPM)", motor.speed_rpm, MOTOR_SPEED_MAX_RPM);
	LOG_INF("Position: %d degrees", motor.position_deg);
	LOG_INF("Current: %d mA (limit: %d mA)", motor.current_ma, MOTOR_CURRENT_LIMIT_MA);
	LOG_INF("Temperature: %d°C (limit: %d°C)", 
	        motor.temperature_c, MOTOR_TEMP_LIMIT_C);
	LOG_INF("Total Runtime: %u seconds", motor.total_runtime_s);
	LOG_INF("Fault Trips: %u", motor.fault_trips);
	k_mutex_unlock(&motor_mutex);

	if (ft_get_statistics(&stats) == 0) {
		LOG_INF("----------------------------------------");
		LOG_INF("FAULT TOLERANCE STATISTICS");
		LOG_INF("----------------------------------------");
		LOG_INF("Total Faults: %u", stats.total_faults);
		LOG_INF("Successful Recoveries: %u", stats.recoveries_successful);
		LOG_INF("Failed Recoveries: %u", stats.recoveries_failed);
		LOG_INF("Overcurrent Events: %u", 
		        stats.fault_counts[FT_PERIPH_TIMEOUT]);
		LOG_INF("Communication Errors: %u", 
		        stats.fault_counts[FT_COMM_CRC_ERROR]);
	}
	LOG_INF("========================================");
}

/**
 * @brief Main application
 */
int main(void)
{
	LOG_INF("========================================");
	LOG_INF("Industrial Motor Controller");
	LOG_INF("Fault Tolerant Embedded System");
	LOG_INF("========================================");

	/* Initialize fault tolerance */
	ft_init();
	ft_register_handler(FT_PERIPH_TIMEOUT, sensor_timeout_recovery);
	ft_register_handler(FT_POWER_BROWNOUT, thermal_recovery);
	ft_register_handler(FT_COMM_CRC_ERROR, modbus_recovery);
	ft_register_handler(FT_MEM_CORRUPTION, memory_corruption_recovery);
	ft_register_handler(FT_HARDFAULT, hardfault_recovery);
	ft_register_handler(FT_STACK_OVERFLOW, stack_overflow_recovery);
	ft_register_handler(FT_WATCHDOG_BARK, watchdog_recovery);

	LOG_INF("Safety systems initialized");
	LOG_INF("Current limit: %d mA", MOTOR_CURRENT_LIMIT_MA);
	LOG_INF("Temperature limit: %d°C", MOTOR_TEMP_LIMIT_C);

	/* Create monitoring and control threads */
	k_thread_create(&current_monitor_data, current_monitor_stack,
	                K_THREAD_STACK_SIZEOF(current_monitor_stack),
	                current_monitor_thread, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	k_thread_name_set(&current_monitor_data, "current_mon");

	k_thread_create(&temp_monitor_data, temp_monitor_stack,
	                K_THREAD_STACK_SIZEOF(temp_monitor_stack),
	                temp_monitor_thread, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(6), 0, K_NO_WAIT);
	k_thread_name_set(&temp_monitor_data, "temp_mon");

	k_thread_create(&position_control_data, position_control_stack,
	                K_THREAD_STACK_SIZEOF(position_control_stack),
	                position_control_thread, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
	k_thread_name_set(&position_control_data, "pos_ctrl");

	k_thread_create(&modbus_comm_data, modbus_comm_stack,
	                K_THREAD_STACK_SIZEOF(modbus_comm_stack),
	                modbus_comm_thread, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_thread_name_set(&modbus_comm_data, "modbus");

	LOG_INF("All subsystems started - motor operational");

	/* Main loop - status display and runtime tracking */
	while (1) {
		k_sleep(K_SECONDS(8));
		motor.total_runtime_s += 8;
		display_motor_status();
	}

	return 0;
}
