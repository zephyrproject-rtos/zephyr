/*
 * Autonomous Drone Flight Controller with Fault Tolerance
 * 
 * This application demonstrates fault tolerance in a quadcopter drone flight
 * controller. It handles GPS failures, IMU sensor faults, communication loss,
 * battery issues, and motor failures while maintaining safe flight.
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(drone_controller, LOG_LEVEL_INF);

/* Flight parameters */
#define CRUISE_ALTITUDE_M         50      /* 50 meters */
#define CRUISE_SPEED_MS           10      /* 10 m/s */
#define BATTERY_MIN_PERCENT       20      /* Land at 20% */
#define BATTERY_CRITICAL_PERCENT  10      /* Emergency at 10% */
#define GPS_TIMEOUT_MS            1000
#define IMU_SAMPLE_RATE_MS        10
#define TELEMETRY_RATE_MS         500
#define FLIGHT_CONTROL_MS         20      /* 50 Hz control loop */

/* Flight modes */
typedef enum {
	MODE_MANUAL,
	MODE_STABILIZE,
	MODE_GPS_HOLD,
	MODE_WAYPOINT,
	MODE_RTL,           /* Return to launch */
	MODE_EMERGENCY_LAND
} flight_mode_t;

/* Drone state */
struct drone_state {
	flight_mode_t mode;
	bool motors_armed;
	bool gps_valid;
	bool imu_valid;
	bool radio_link;
	
	int32_t altitude_cm;
	int32_t speed_cms;
	int32_t battery_percent;
	int32_t gps_satellites;
	
	uint32_t flight_time_s;
	uint32_t gps_timeouts;
	uint32_t imu_errors;
	uint32_t motor_failures;
	uint32_t radio_losses;
	uint32_t emergency_lands;
};

static struct drone_state drone = {
	.mode = MODE_GPS_HOLD,
	.motors_armed = true,
	.gps_valid = true,
	.imu_valid = true,
	.radio_link = true,
	.altitude_cm = 5000,  /* 50 meters */
	.speed_cms = 1000,    /* 10 m/s */
	.battery_percent = 100,
	.gps_satellites = 12,
	.flight_time_s = 0,
	.gps_timeouts = 0,
	.imu_errors = 0,
	.motor_failures = 0,
	.radio_losses = 0,
	.emergency_lands = 0
};

/* Thread stacks */
K_THREAD_STACK_DEFINE(flight_control_stack, 2048);
K_THREAD_STACK_DEFINE(sensor_fusion_stack, 2048);
K_THREAD_STACK_DEFINE(telemetry_stack, 1024);
K_THREAD_STACK_DEFINE(battery_monitor_stack, 1024);

static struct k_thread flight_control_data;
static struct k_thread sensor_fusion_data;
static struct k_thread telemetry_data;
static struct k_thread battery_monitor_data;

K_MUTEX_DEFINE(drone_mutex);

static const char *mode_to_string(flight_mode_t mode)
{
	switch (mode) {
	case MODE_MANUAL: return "MANUAL";
	case MODE_STABILIZE: return "STABILIZE";
	case MODE_GPS_HOLD: return "GPS_HOLD";
	case MODE_WAYPOINT: return "WAYPOINT";
	case MODE_RTL: return "RETURN_TO_LAUNCH";
	case MODE_EMERGENCY_LAND: return "EMERGENCY_LAND";
	default: return "UNKNOWN";
	}
}

/**
 * @brief Recovery handler for GPS timeout
 */
static ft_recovery_result_t gps_timeout_recovery(const ft_event_t *event)
{
	LOG_WRN("=== GPS SIGNAL LOST ===");
	
	k_mutex_lock(&drone_mutex, K_FOREVER);
	drone.gps_valid = false;
	drone.gps_timeouts++;
	
	/* Switch to inertial navigation mode */
	if (drone.mode == MODE_GPS_HOLD || drone.mode == MODE_WAYPOINT) {
		drone.mode = MODE_STABILIZE;
		LOG_WRN("Switching from GPS mode to STABILIZE (dead reckoning)");
		LOG_INF("Using IMU + barometer for position estimation");
	}
	k_mutex_unlock(&drone_mutex);

	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for IMU sensor failure
 */
static ft_recovery_result_t imu_failure_recovery(const ft_event_t *event)
{
	LOG_ERR("=== IMU SENSOR FAILURE ===");
	
	k_mutex_lock(&drone_mutex, K_FOREVER);
	drone.imu_valid = false;
	drone.imu_errors++;
	drone.mode = MODE_EMERGENCY_LAND;
	drone.emergency_lands++;
	k_mutex_unlock(&drone_mutex);

	LOG_ERR("CRITICAL: Cannot maintain stable flight without IMU");
	LOG_ERR("Initiating EMERGENCY LANDING procedure");
	LOG_INF("Attempting controlled descent");

	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for radio communication loss
 */
static ft_recovery_result_t radio_loss_recovery(const ft_event_t *event)
{
	LOG_WRN("=== RADIO LINK LOST ===");
	
	k_mutex_lock(&drone_mutex, K_FOREVER);
	drone.radio_link = false;
	drone.radio_losses++;
	
	/* Activate failsafe: Return to launch */
	if (drone.mode != MODE_EMERGENCY_LAND) {
		drone.mode = MODE_RTL;
		LOG_WRN("Failsafe activated: RETURN TO LAUNCH");
		LOG_INF("Flying back to home position autonomously");
	}
	k_mutex_unlock(&drone_mutex);

	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for low battery
 */
static ft_recovery_result_t low_battery_recovery(const ft_event_t *event)
{
	LOG_WRN("=== LOW BATTERY WARNING ===");
	
	k_mutex_lock(&drone_mutex, K_FOREVER);
	
	if (drone.battery_percent <= BATTERY_CRITICAL_PERCENT) {
		LOG_ERR("CRITICAL BATTERY LEVEL: %d%%", drone.battery_percent);
		LOG_ERR("EMERGENCY LANDING NOW");
		drone.mode = MODE_EMERGENCY_LAND;
		drone.emergency_lands++;
	} else {
		LOG_WRN("Battery at %d%% - initiating return to launch", 
		        drone.battery_percent);
		if (drone.mode != MODE_EMERGENCY_LAND) {
			drone.mode = MODE_RTL;
		}
	}
	k_mutex_unlock(&drone_mutex);

	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for motor failure
 */
static ft_recovery_result_t motor_failure_recovery(const ft_event_t *event)
{
	LOG_ERR("=== MOTOR FAILURE DETECTED ===");
	
	k_mutex_lock(&drone_mutex, K_FOREVER);
	drone.motor_failures++;
	drone.mode = MODE_EMERGENCY_LAND;
	drone.emergency_lands++;
	k_mutex_unlock(&drone_mutex);

	LOG_ERR("Motor failure - limited control authority");
	LOG_ERR("Attempting emergency landing with remaining motors");
	LOG_INF("Compensating thrust distribution");

	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for memory corruption
 */
static ft_recovery_result_t memory_corruption_recovery(const ft_event_t *event)
{
	LOG_ERR("=== MEMORY CORRUPTION ===");
	
	k_mutex_lock(&drone_mutex, K_FOREVER);
	drone.mode = MODE_EMERGENCY_LAND;
	drone.motors_armed = false;
	drone.emergency_lands++;
	k_mutex_unlock(&drone_mutex);

	LOG_ERR("Flight control memory corrupted");
	LOG_ERR("IMMEDIATE LANDING REQUIRED");

	return FT_RECOVERY_REBOOT_REQUIRED;
}

/**
 * @brief Flight control thread - main control loop
 */
static void flight_control_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Flight control thread started (50 Hz)");

	while (1) {
		k_mutex_lock(&drone_mutex, K_FOREVER);
		flight_mode_t current_mode = drone.mode;
		bool armed = drone.motors_armed;
		k_mutex_unlock(&drone_mutex);

		if (armed) {
			/* Execute mode-specific control logic */
			switch (current_mode) {
			case MODE_GPS_HOLD:
				/* Hold current GPS position */
				break;
				
			case MODE_WAYPOINT:
				/* Navigate to waypoint */
				break;
				
			case MODE_RTL:
				/* Return to launch point */
				k_mutex_lock(&drone_mutex, K_FOREVER);
				/* Simulate descent */
				if (drone.altitude_cm > 0) {
					drone.altitude_cm -= 50;  /* 0.5m per cycle */
				}
				drone.speed_cms = 500;  /* 5 m/s return speed */
				k_mutex_unlock(&drone_mutex);
				break;
				
			case MODE_EMERGENCY_LAND:
				/* Emergency landing - descend quickly but safely */
				k_mutex_lock(&drone_mutex, K_FOREVER);
				if (drone.altitude_cm > 0) {
					drone.altitude_cm -= 100;  /* 1m per cycle */
					if (drone.altitude_cm < 0) {
						drone.altitude_cm = 0;
						drone.motors_armed = false;
						LOG_INF("=== LANDED ===");
					}
				}
				drone.speed_cms = 0;
				k_mutex_unlock(&drone_mutex);
				break;
				
			default:
				break;
			}
		}

		k_msleep(FLIGHT_CONTROL_MS);
	}
}

/**
 * @brief Sensor fusion thread - processes IMU and GPS
 */
static void sensor_fusion_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Sensor fusion thread started");

	while (1) {
		/* Check GPS timeout (2% chance) */
		if ((sys_rand32_get() % 100) < 2) {
			LOG_WRN("GPS timeout!");
			
			ft_event_t event = {
				.kind = FT_PERIPH_TIMEOUT,
				.severity = FT_SEV_WARNING,
				.domain = FT_DOMAIN_HARDWARE,
				.code = 0x6601,
				.timestamp = k_uptime_get(),
				.thread_id = k_current_get(),
				.context = NULL,
				.context_size = 0
			};

			ft_report_fault(&event);
		} else if (!drone.gps_valid && (sys_rand32_get() % 100) < 30) {
			/* GPS recovery */
			k_mutex_lock(&drone_mutex, K_FOREVER);
			drone.gps_valid = true;
			drone.gps_satellites = 8 + (sys_rand32_get() % 5);
			k_mutex_unlock(&drone_mutex);
			LOG_INF("GPS signal restored (%d satellites)", drone.gps_satellites);
		}

		/* Check IMU health (0.5% chance of failure) */
		if ((sys_rand32_get() % 1000) < 5) {
			LOG_ERR("IMU sensor fault detected!");
			
			ft_event_t event = {
				.kind = FT_HARDFAULT,  /* Using as IMU fault */
				.severity = FT_SEV_CRITICAL,
				.domain = FT_DOMAIN_HARDWARE,
				.code = 0x2601,
				.timestamp = k_uptime_get(),
				.thread_id = k_current_get(),
				.context = NULL,
				.context_size = 0
			};

			ft_report_fault(&event);
		}

		k_msleep(IMU_SAMPLE_RATE_MS);
	}
}

/**
 * @brief Telemetry thread - handles radio communication
 */
static void telemetry_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Telemetry thread started");

	while (1) {
		/* Simulate radio CRC errors (3% chance) */
		if ((sys_rand32_get() % 100) < 3) {
			LOG_WRN("Radio packet CRC error");
			
			ft_event_t event = {
				.kind = FT_COMM_CRC_ERROR,
				.severity = FT_SEV_WARNING,
				.domain = FT_DOMAIN_COMMUNICATION,
				.code = 0x7601,
				.timestamp = k_uptime_get(),
				.thread_id = k_current_get(),
				.context = NULL,
				.context_size = 0
			};

			ft_report_fault(&event);
		} else if (!drone.radio_link && (sys_rand32_get() % 100) < 40) {
			/* Radio recovery */
			k_mutex_lock(&drone_mutex, K_FOREVER);
			drone.radio_link = true;
			k_mutex_unlock(&drone_mutex);
			LOG_INF("Radio link restored");
		}

		k_msleep(TELEMETRY_RATE_MS);
	}
}

/**
 * @brief Battery monitor thread
 */
static void battery_monitor_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Battery monitoring thread started");

	while (1) {
		if (drone.motors_armed) {
			/* Simulate battery drain */
			k_mutex_lock(&drone_mutex, K_FOREVER);
			drone.battery_percent -= 1;
			if (drone.battery_percent < 0) {
				drone.battery_percent = 0;
			}
			k_mutex_unlock(&drone_mutex);

			/* Check battery levels */
			if (drone.battery_percent <= BATTERY_CRITICAL_PERCENT) {
				LOG_ERR("Critical battery: %d%%", drone.battery_percent);
				
				ft_event_t event = {
					.kind = FT_POWER_BROWNOUT,
					.severity = FT_SEV_CRITICAL,
					.domain = FT_DOMAIN_HARDWARE,
					.code = 0x8601,
					.timestamp = k_uptime_get(),
					.thread_id = k_current_get(),
					.context = NULL,
					.context_size = 0
				};

				ft_report_fault(&event);
			} else if (drone.battery_percent <= BATTERY_MIN_PERCENT) {
				LOG_WRN("Low battery: %d%%", drone.battery_percent);
				
				ft_event_t event = {
					.kind = FT_POWER_BROWNOUT,
					.severity = FT_SEV_WARNING,
					.domain = FT_DOMAIN_HARDWARE,
					.code = 0x8602,
					.timestamp = k_uptime_get(),
					.thread_id = k_current_get(),
					.context = NULL,
					.context_size = 0
				};

				ft_report_fault(&event);
			}
		}

		k_sleep(K_SECONDS(5));
	}
}

/**
 * @brief Display drone status
 */
static void display_drone_status(void)
{
	ft_statistics_t stats;

	LOG_INF("========================================");
	LOG_INF("DRONE FLIGHT CONTROLLER STATUS");
	LOG_INF("========================================");
	
	k_mutex_lock(&drone_mutex, K_FOREVER);
	LOG_INF("Flight Mode: %s", mode_to_string(drone.mode));
	LOG_INF("Motors: %s", drone.motors_armed ? "ARMED" : "DISARMED");
	LOG_INF("Altitude: %d.%02d m", drone.altitude_cm / 100, drone.altitude_cm % 100);
	LOG_INF("Speed: %d.%02d m/s", drone.speed_cms / 100, drone.speed_cms % 100);
	LOG_INF("Battery: %d%%", drone.battery_percent);
	LOG_INF("GPS: %s (%d satellites)", 
	        drone.gps_valid ? "VALID" : "INVALID", drone.gps_satellites);
	LOG_INF("IMU: %s", drone.imu_valid ? "OK" : "FAULT");
	LOG_INF("Radio: %s", drone.radio_link ? "CONNECTED" : "LOST");
	LOG_INF("Flight Time: %u seconds", drone.flight_time_s);
	LOG_INF("----------------------------------------");
	LOG_INF("FAULT EVENTS");
	LOG_INF("----------------------------------------");
	LOG_INF("GPS Timeouts: %u", drone.gps_timeouts);
	LOG_INF("IMU Errors: %u", drone.imu_errors);
	LOG_INF("Radio Losses: %u", drone.radio_losses);
	LOG_INF("Motor Failures: %u", drone.motor_failures);
	LOG_INF("Emergency Lands: %u", drone.emergency_lands);
	k_mutex_unlock(&drone_mutex);

	if (ft_get_statistics(&stats) == 0) {
		LOG_INF("----------------------------------------");
		LOG_INF("FAULT TOLERANCE STATISTICS");
		LOG_INF("----------------------------------------");
		LOG_INF("Total Faults: %u", stats.total_faults);
		LOG_INF("Successful Recoveries: %u", stats.recoveries_successful);
		LOG_INF("Failed Recoveries: %u", stats.recoveries_failed);
	}
	LOG_INF("========================================");
}

/**
 * @brief Main application
 */
int main(void)
{
	LOG_INF("========================================");
	LOG_INF("Autonomous Drone Flight Controller");
	LOG_INF("Fault-Tolerant Navigation System");
	LOG_INF("========================================");

	/* Initialize fault tolerance */
	ft_init();
	ft_register_handler(FT_PERIPH_TIMEOUT, gps_timeout_recovery);
	ft_register_handler(FT_HARDFAULT, imu_failure_recovery);
	ft_register_handler(FT_COMM_CRC_ERROR, radio_loss_recovery);
	ft_register_handler(FT_POWER_BROWNOUT, low_battery_recovery);
	ft_register_handler(FT_MEM_CORRUPTION, memory_corruption_recovery);

	LOG_INF("Flight safety systems initialized");
	LOG_INF("Takeoff altitude: %d m", CRUISE_ALTITUDE_M);
	LOG_INF("Cruise speed: %d m/s", CRUISE_SPEED_MS);

	/* Create flight control threads */
	k_thread_create(&flight_control_data, flight_control_stack,
	                K_THREAD_STACK_SIZEOF(flight_control_stack),
	                flight_control_thread, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	k_thread_name_set(&flight_control_data, "flight_ctrl");

	k_thread_create(&sensor_fusion_data, sensor_fusion_stack,
	                K_THREAD_STACK_SIZEOF(sensor_fusion_stack),
	                sensor_fusion_thread, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(6), 0, K_NO_WAIT);
	k_thread_name_set(&sensor_fusion_data, "sensors");

	k_thread_create(&telemetry_data, telemetry_stack,
	                K_THREAD_STACK_SIZEOF(telemetry_stack),
	                telemetry_thread, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
	k_thread_name_set(&telemetry_data, "telemetry");

	k_thread_create(&battery_monitor_data, battery_monitor_stack,
	                K_THREAD_STACK_SIZEOF(battery_monitor_stack),
	                battery_monitor_thread, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_thread_name_set(&battery_monitor_data, "battery");

	LOG_INF("Flight systems operational - ready for mission");

	/* Main loop - status display */
	while (1) {
		k_sleep(K_SECONDS(10));
		drone.flight_time_s += 10;
		display_drone_status();
	}

	return 0;
}
