/*
 * Medical Infusion Pump with Fault Tolerance
 * 
 * This application demonstrates fault tolerance in a safety-critical medical
 * infusion pump. It monitors flow rate, detects air bubbles, handles occlusions,
 * and ensures patient safety through multiple redundant safety checks.
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(infusion_pump, LOG_LEVEL_INF);

/* Pump configuration */
#define TARGET_FLOW_RATE_ML_HR    100    /* Target: 100 mL/hour */
#define FLOW_TOLERANCE_PERCENT    10     /* ±10% tolerance */
#define PRESSURE_MAX_MMHG         300    /* Maximum line pressure */
#define VOLUME_TO_INFUSE_ML       500    /* Total dose volume */
#define FLOW_MONITOR_MS           200    /* Monitor every 200ms */
#define SAFETY_CHECK_MS           500    /* Safety checks every 500ms */
#define ALARM_CHECK_MS            100    /* Alarm monitoring */

/* Pump state */
struct pump_state {
	bool running;
	bool alarm_active;
	uint32_t current_flow_ml_hr;
	uint32_t pressure_mmhg;
	uint32_t volume_infused_ml;
	uint32_t air_bubbles_detected;
	uint32_t occlusion_events;
	uint32_t flow_errors;
	uint32_t uptime_seconds;
	uint32_t safety_shutdowns;
};

static struct pump_state pump = {
	.running = true,
	.alarm_active = false,
	.current_flow_ml_hr = 0,
	.pressure_mmhg = 0,
	.volume_infused_ml = 0,
	.air_bubbles_detected = 0,
	.occlusion_events = 0,
	.flow_errors = 0,
	.uptime_seconds = 0,
	.safety_shutdowns = 0
};

/* Thread stacks */
K_THREAD_STACK_DEFINE(flow_control_stack, 2048);
K_THREAD_STACK_DEFINE(safety_monitor_stack, 2048);
K_THREAD_STACK_DEFINE(alarm_handler_stack, 1024);

static struct k_thread flow_control_data;
static struct k_thread safety_monitor_data;
static struct k_thread alarm_handler_data;

K_MUTEX_DEFINE(pump_mutex);

/**
 * @brief Recovery handler for flow rate errors
 */
static ft_recovery_result_t flow_error_recovery(const ft_event_t *event)
{
	LOG_ERR("=== FLOW RATE ERROR RECOVERY ===");
	LOG_ERR("Flow rate outside acceptable range");
	
	k_mutex_lock(&pump_mutex, K_FOREVER);
	pump.running = false;
	pump.alarm_active = true;
	pump.flow_errors++;
	k_mutex_unlock(&pump_mutex);

	LOG_WRN("PUMP STOPPED - Flow rate error");
	LOG_INF("Recalibrating flow sensor");
	
	/* In real system: recalibrate pump motor and flow sensor */
	k_msleep(100);
	
	k_mutex_lock(&pump_mutex, K_FOREVER);
	pump.running = true;
	pump.alarm_active = false;
	k_mutex_unlock(&pump_mutex);
	
	LOG_INF("Pump resumed after recalibration");
	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Recovery handler for air bubble detection
 */
static ft_recovery_result_t air_bubble_recovery(const ft_event_t *event)
{
	LOG_ERR("=== AIR BUBBLE DETECTED ===");
	
	k_mutex_lock(&pump_mutex, K_FOREVER);
	pump.running = false;
	pump.alarm_active = true;
	pump.air_bubbles_detected++;
	pump.safety_shutdowns++;
	k_mutex_unlock(&pump_mutex);

	LOG_ERR("CRITICAL SAFETY EVENT: Air in line detected!");
	LOG_ERR("PUMP EMERGENCY STOP - Manual intervention required");
	LOG_INF("Audible alarm activated");
	LOG_INF("Nurse call signal sent");

	/* Air in IV line is CRITICAL - do not auto-restart */
	return FT_RECOVERY_FAILED;
}

/**
 * @brief Recovery handler for occlusion (blockage)
 */
static ft_recovery_result_t occlusion_recovery(const ft_event_t *event)
{
	LOG_WRN("=== OCCLUSION DETECTED ===");
	
	k_mutex_lock(&pump_mutex, K_FOREVER);
	pump.running = false;
	pump.alarm_active = true;
	pump.occlusion_events++;
	k_mutex_unlock(&pump_mutex);

	LOG_WRN("Line pressure exceeded limit");
	LOG_INF("Checking for kinks or blockages");
	
	/* Attempt to clear occlusion */
	LOG_INF("Attempting gentle pressure pulse");
	k_msleep(500);
	
	/* Simulated success (in reality would check if pressure dropped) */
	uint32_t test_pressure = 100 + (sys_rand32_get() % 50);
	
	if (test_pressure < 150) {
		k_mutex_lock(&pump_mutex, K_FOREVER);
		pump.running = true;
		pump.alarm_active = false;
		pump.pressure_mmhg = test_pressure;
		k_mutex_unlock(&pump_mutex);
		
		LOG_INF("Occlusion cleared - resuming infusion");
		return FT_RECOVERY_SUCCESS;
	} else {
		LOG_ERR("Occlusion persists - manual intervention required");
		return FT_RECOVERY_FAILED;
	}
}

/**
 * @brief Recovery handler for sensor timeout
 */
static ft_recovery_result_t sensor_timeout_recovery(const ft_event_t *event)
{
	LOG_ERR("=== SENSOR COMMUNICATION TIMEOUT ===");
	
	k_mutex_lock(&pump_mutex, K_FOREVER);
	pump.running = false;
	pump.alarm_active = true;
	pump.safety_shutdowns++;
	k_mutex_unlock(&pump_mutex);

	LOG_ERR("Flow sensor communication lost");
	LOG_ERR("SAFETY SHUTDOWN: Cannot verify flow rate");
	LOG_INF("Attempting sensor reset");
	
	/* Try to reset sensor */
	k_msleep(200);
	
	/* Simulate sensor recovery */
	if ((sys_rand32_get() % 100) < 70) {
		k_mutex_lock(&pump_mutex, K_FOREVER);
		pump.running = true;
		pump.alarm_active = false;
		k_mutex_unlock(&pump_mutex);
		
		LOG_INF("Sensor communication restored");
		return FT_RECOVERY_SUCCESS;
	} else {
		LOG_ERR("Sensor unresponsive - device inoperable");
		return FT_RECOVERY_REBOOT_REQUIRED;
	}
}

/**
 * @brief Recovery handler for memory corruption
 */
static ft_recovery_result_t memory_corruption_recovery(const ft_event_t *event)
{
	LOG_ERR("=== MEMORY CORRUPTION DETECTED ===");
	
	k_mutex_lock(&pump_mutex, K_FOREVER);
	pump.running = false;
	pump.alarm_active = true;
	pump.safety_shutdowns++;
	k_mutex_unlock(&pump_mutex);

	LOG_ERR("CRITICAL: Dosage calculation memory corrupted");
	LOG_ERR("EMERGENCY SHUTDOWN for patient safety");
	LOG_ERR("System requires restart and verification");

	return FT_RECOVERY_REBOOT_REQUIRED;
}

/**
 * @brief Flow control thread - manages infusion rate
 */
static void flow_control_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Flow control thread started");
	uint32_t cycle_count = 0;

	while (1) {
		if (pump.running) {
			cycle_count++;
			
			/* Simulate flow rate with variation */
			int32_t variation = (sys_rand32_get() % 30) - 15;
			int32_t flow = TARGET_FLOW_RATE_ML_HR + variation;
			
			k_mutex_lock(&pump_mutex, K_FOREVER);
			pump.current_flow_ml_hr = flow;
			
			/* Update volume infused (rough approximation) */
			pump.volume_infused_ml += (flow * FLOW_MONITOR_MS) / (1000 * 3600);
			k_mutex_unlock(&pump_mutex);

			/* Check if flow rate is out of tolerance */
			uint32_t min_flow = TARGET_FLOW_RATE_ML_HR * (100 - FLOW_TOLERANCE_PERCENT) / 100;
			uint32_t max_flow = TARGET_FLOW_RATE_ML_HR * (100 + FLOW_TOLERANCE_PERCENT) / 100;
			
			if (flow < min_flow || flow > max_flow) {
				LOG_WRN("Flow rate out of range: %d mL/hr (target: %d ±%d%%)",
				        flow, TARGET_FLOW_RATE_ML_HR, FLOW_TOLERANCE_PERCENT);
				
				struct flow_fault_context {
					uint32_t current_flow;
					uint32_t target_flow;
					uint32_t min_flow;
					uint32_t max_flow;
				} ctx = {
					.current_flow = flow,
					.target_flow = TARGET_FLOW_RATE_ML_HR,
					.min_flow = min_flow,
					.max_flow = max_flow
				};

				ft_event_t event = {
					.kind = FT_PERIPH_TIMEOUT,  /* Using as control error */
					.severity = FT_SEV_ERROR,
					.domain = FT_DOMAIN_HARDWARE,
					.code = 0x6501,
					.timestamp = k_uptime_get(),
					.thread_id = k_current_get(),
					.context = &ctx,
					.context_size = sizeof(ctx)
				};

				ft_report_fault(&event);
			}

			/* Check if infusion complete */
			if (pump.volume_infused_ml >= VOLUME_TO_INFUSE_ML) {
				k_mutex_lock(&pump_mutex, K_FOREVER);
				pump.running = false;
				k_mutex_unlock(&pump_mutex);
				
				LOG_INF("=== INFUSION COMPLETE ===");
				LOG_INF("Volume infused: %u mL", pump.volume_infused_ml);
			}
		}

		k_msleep(FLOW_MONITOR_MS);
	}
}

/**
 * @brief Safety monitor thread - checks for hazardous conditions
 */
static void safety_monitor_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Safety monitoring thread started");

	while (1) {
		if (pump.running) {
			/* Simulate pressure reading */
			int32_t pressure_delta = (sys_rand32_get() % 40) - 20;
			k_mutex_lock(&pump_mutex, K_FOREVER);
			pump.pressure_mmhg = 100 + pressure_delta;
			k_mutex_unlock(&pump_mutex);

			/* Check for occlusion (high pressure, 3% chance) */
			if (pump.pressure_mmhg > PRESSURE_MAX_MMHG || 
			    (sys_rand32_get() % 100) < 3) {
				LOG_ERR("High pressure detected: %u mmHg (limit: %u)",
				        pump.pressure_mmhg, PRESSURE_MAX_MMHG);
				
				struct occlusion_context {
					uint32_t pressure_mmhg;
					uint32_t limit_mmhg;
				} ctx = {
					.pressure_mmhg = pump.pressure_mmhg,
					.limit_mmhg = PRESSURE_MAX_MMHG
				};

				ft_event_t event = {
					.kind = FT_APP_ASSERT,  /* Using as pressure fault */
					.severity = FT_SEV_WARNING,
					.domain = FT_DOMAIN_HARDWARE,
					.code = 0x9501,
					.timestamp = k_uptime_get(),
					.thread_id = k_current_get(),
					.context = &ctx,
					.context_size = sizeof(ctx)
				};

				ft_report_fault(&event);
			}

			/* Check for air bubbles (1% chance - very rare) */
			if ((sys_rand32_get() % 100) < 1) {
				LOG_ERR("AIR BUBBLE SENSOR TRIGGERED!");
				
				ft_event_t event = {
					.kind = FT_COMM_CRC_ERROR,  /* Using as air bubble */
					.severity = FT_SEV_CRITICAL,
					.domain = FT_DOMAIN_HARDWARE,
					.code = 0x7501,
					.timestamp = k_uptime_get(),
					.thread_id = k_current_get(),
					.context = NULL,
					.context_size = 0
				};

				ft_report_fault(&event);
			}

			/* Check for sensor timeout (2% chance) */
			if ((sys_rand32_get() % 100) < 2) {
				LOG_ERR("Flow sensor timeout!");
				
				ft_event_t event = {
					.kind = FT_PERIPH_TIMEOUT,
					.severity = FT_SEV_CRITICAL,
					.domain = FT_DOMAIN_HARDWARE,
					.code = 0x6502,
					.timestamp = k_uptime_get(),
					.thread_id = k_current_get(),
					.context = NULL,
					.context_size = 0
				};

				ft_report_fault(&event);
			}
		}

		k_msleep(SAFETY_CHECK_MS);
	}
}

/**
 * @brief Alarm handler thread
 */
static void alarm_handler_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("Alarm handler thread started");

	while (1) {
		k_mutex_lock(&pump_mutex, K_FOREVER);
		bool alarm = pump.alarm_active;
		k_mutex_unlock(&pump_mutex);

		if (alarm) {
			/* In real system: activate buzzer, LED, nurse call */
			LOG_WRN("*** ALARM ACTIVE ***");
		}

		k_msleep(ALARM_CHECK_MS);
	}
}

/**
 * @brief Display pump status
 */
static void display_pump_status(void)
{
	ft_statistics_t stats;

	LOG_INF("========================================");
	LOG_INF("MEDICAL INFUSION PUMP STATUS");
	LOG_INF("========================================");
	
	k_mutex_lock(&pump_mutex, K_FOREVER);
	LOG_INF("Pump State: %s", pump.running ? "INFUSING" : "STOPPED");
	LOG_INF("Alarm: %s", pump.alarm_active ? "ACTIVE" : "CLEAR");
	LOG_INF("Flow Rate: %u mL/hr (target: %u mL/hr)",
	        pump.current_flow_ml_hr, TARGET_FLOW_RATE_ML_HR);
	LOG_INF("Line Pressure: %u mmHg (max: %u mmHg)",
	        pump.pressure_mmhg, PRESSURE_MAX_MMHG);
	LOG_INF("Volume Infused: %u / %u mL",
	        pump.volume_infused_ml, VOLUME_TO_INFUSE_ML);
	LOG_INF("Uptime: %u seconds", pump.uptime_seconds);
	LOG_INF("----------------------------------------");
	LOG_INF("SAFETY EVENTS");
	LOG_INF("----------------------------------------");
	LOG_INF("Air Bubbles Detected: %u", pump.air_bubbles_detected);
	LOG_INF("Occlusion Events: %u", pump.occlusion_events);
	LOG_INF("Flow Errors: %u", pump.flow_errors);
	LOG_INF("Safety Shutdowns: %u", pump.safety_shutdowns);
	k_mutex_unlock(&pump_mutex);

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
	LOG_INF("Medical Infusion Pump Controller");
	LOG_INF("Safety-Critical Device with FT");
	LOG_INF("========================================");

	/* Initialize fault tolerance */
	ft_init();
	ft_register_handler(FT_PERIPH_TIMEOUT, sensor_timeout_recovery);
	ft_register_handler(FT_COMM_CRC_ERROR, air_bubble_recovery);
	ft_register_handler(FT_APP_ASSERT, occlusion_recovery);
	ft_register_handler(FT_MEM_CORRUPTION, memory_corruption_recovery);

	LOG_INF("Safety systems initialized");
	LOG_INF("Target dose: %u mL at %u mL/hr",
	        VOLUME_TO_INFUSE_ML, TARGET_FLOW_RATE_ML_HR);

	/* Create monitoring and control threads */
	k_thread_create(&flow_control_data, flow_control_stack,
	                K_THREAD_STACK_SIZEOF(flow_control_stack),
	                flow_control_thread, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	k_thread_name_set(&flow_control_data, "flow_ctrl");

	k_thread_create(&safety_monitor_data, safety_monitor_stack,
	                K_THREAD_STACK_SIZEOF(safety_monitor_stack),
	                safety_monitor_thread, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(4), 0, K_NO_WAIT);
	k_thread_name_set(&safety_monitor_data, "safety_mon");

	k_thread_create(&alarm_handler_data, alarm_handler_stack,
	                K_THREAD_STACK_SIZEOF(alarm_handler_stack),
	                alarm_handler_thread, NULL, NULL, NULL,
	                K_PRIO_PREEMPT(3), 0, K_NO_WAIT);
	k_thread_name_set(&alarm_handler_data, "alarm");

	LOG_INF("Infusion started - all safety systems active");

	/* Main loop - status display */
	while (1) {
		k_sleep(K_SECONDS(10));
		pump.uptime_seconds += 10;
		display_pump_status();
	}

	return 0;
}
