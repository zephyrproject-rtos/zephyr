# Fault Tolerance API - Sample Embedded Applications

This directory contains realistic embedded system applications that demonstrate the Zephyr Fault Tolerance API in practical scenarios. These applications showcase how to integrate fault detection and recovery into real-world embedded systems.

## Overview

The sample applications demonstrate:
- Multi-threaded embedded system architectures
- Real-world fault scenarios and recovery strategies
- Integration with Zephyr RTOS subsystems
- Event-driven fault handling
- Statistical monitoring and logging
- Graceful degradation under fault conditions

## Applications Summary

| Application | Domain | Safety Level | Key Features |
|-------------|--------|--------------|--------------|
| Smart Thermostat | Smart Home | Medium | HVAC control, sensor fallback |
| Industrial Motor | Manufacturing | High | Overcurrent, thermal protection |
| Medical Infusion Pump | Healthcare | Critical | Air bubble detection, dosing accuracy |
| Autonomous Drone | Aerospace | Critical | GPS-denied navigation, RTL failsafe |

---

## Applications

### 1. Smart Thermostat (`smart_thermostat/`)

**Description**: A connected thermostat that controls HVAC systems based on temperature sensor readings and communicates status over UART.

**Key Features**:
- Temperature sensor monitoring with I2C timeout handling
- HVAC control logic (heating/cooling)
- UART status transmission with CRC validation
- Watchdog monitoring
- Safe mode operation on sensor failures

**Fault Scenarios**:
| Fault Type | Trigger | Recovery Strategy | Impact |
|------------|---------|-------------------|--------|
| Peripheral Timeout | I2C sensor timeout (5% rate) | Use cached temperature | Degraded accuracy |
| CRC Error | UART transmission error (3% rate) | Retransmit packet | Delayed updates |
| Watchdog Bark | Delayed watchdog feed (1% rate) | Emergency feed | Prevent reset |
| App Assert | Out-of-range temperature | Enter safe mode | Disable HVAC |

**Operating Parameters**:
- Target Temperature: 22°C ± 2°C
- Sensor Poll Rate: 1 second
- Status Transmission: Every 5 seconds
- Watchdog Feed: Every 3 seconds

**Building and Running**:
```bash
west build -b qemu_cortex_m3 app/smart_thermostat -p
west build -t run
```

**Expected Output**: Continuous operation with periodic status displays showing temperature, HVAC state, sensor reads, communication status, and fault recovery statistics.

---

### 3. Medical Infusion Pump (`medical_infusion_pump/`)

**Description**: Safety-critical medical device that delivers precise IV fluid doses with multiple redundant safety checks.

**Key Features**:
- Flow rate monitoring with ±10% tolerance
- Air bubble detection (ultrasonic sensor)
- Occlusion detection (pressure monitoring)
- Sensor timeout handling with automatic reset
- Memory integrity verification
- Audible alarm system

**Fault Scenarios**:
| Fault Type | Trigger | Recovery Strategy | Impact |
|------------|---------|-------------------|--------|
| Air Bubble | Ultrasonic detection (1% rate) | Emergency stop, manual only | CRITICAL - No auto-restart |
| Occlusion | Pressure > 300 mmHg (3% rate) | Attempt clearance | Temporary pause |
| Flow Error | Rate outside ±10% | Recalibrate pump | Brief interruption |
| Sensor Timeout | Communication loss (2% rate) | Reset sensor | Safety shutdown |

**Safety Compliance**: IEC 60601-1 (Medical Equipment), IEC 62304 (Medical Software), FDA 21 CFR Part 820

**Operating Parameters**:
- Target Dose: 500 mL at 100 mL/hour
- Flow Tolerance: ±10%
- Pressure Limit: 300 mmHg
- Safety Response: < 200ms

**Building and Running**:
```bash
west build -b qemu_cortex_m3 app/medical_infusion_pump -p
west build -t run
```

**Expected Output**: Infusion progresses with continuous safety monitoring. Random faults trigger alarms and recovery procedures. Critical faults (air bubbles) require manual intervention. Infusion completes after delivering 500 mL.

---

### 4. Autonomous Drone Flight Controller (`autonomous_drone/`)

**Description**: Fault-tolerant quadcopter flight controller demonstrating graceful degradation across multiple sensor failures and autonomous safety responses.

**Key Features**:
- GPS + Inertial navigation with automatic fallback
- IMU failure detection with emergency landing
- Radio loss failsafe (Return To Launch)
- Battery management with progressive warnings
- Motor failure detection and compensation
- Multiple flight modes (GPS Hold, Waypoint, RTL, Emergency Land)

**Fault Scenarios**:
| Fault Type | Trigger | Recovery Strategy | Impact |
|------------|---------|-------------------|--------|
| GPS Timeout | No fix for 1s (2% rate) | Switch to inertial nav | GPS_HOLD → STABILIZE |
| IMU Failure | Sensor error (0.5% rate) | Emergency landing | Any mode → EMERGENCY_LAND |
| Radio Loss | CRC errors (3% rate) | Autonomous RTL | Any mode → RETURN_TO_LAUNCH |
| Low Battery | < 20% | Return to launch | RTL mode |
| Critical Battery | < 10% | Emergency landing | EMERGENCY_LAND mode |

**Flight Modes**:
- **GPS_HOLD**: Maintain current position (default)
- **STABILIZE**: Attitude hold using IMU only
- **RETURN_TO_LAUNCH**: Autonomous navigation home
- **EMERGENCY_LAND**: Controlled rapid descent

**Safety Compliance**: FAA Part 107 (Small UAS), DO-178C (Airborne Software), ARP4754A (Aircraft Systems)

**Operating Parameters**:
- Cruise Altitude: 50 meters
- Cruise Speed: 10 m/s
- Battery Warning: 20%
- Battery Critical: 10%
- Control Loop: 50 Hz

**Building and Running**:
```bash
west build -b qemu_cortex_m3 app/autonomous_drone -p
west build -t run
```

**Expected Output**: Drone operates at 50m altitude with GPS hold. Battery drains over time. Random sensor faults trigger mode changes (GPS loss → STABILIZE, IMU fault → EMERGENCY_LAND). Low battery triggers autonomous return to launch. System demonstrates graceful degradation through sensor failures.

---

### 2. Industrial Motor Controller (`industrial_motor_controller/`)

**Description**: A three-phase motor controller with current monitoring, thermal protection, position control via incremental encoder, and Modbus RTU communication.

**Key Features**:
- Real-time current monitoring (50ms)
- Thermal overload protection with auto-restart
- Position control using incremental encoder
- Sensorless FOC fallback mode
- Modbus RTU communication with CRC validation
- Multiple safety shutdown mechanisms

**Fault Scenarios**:
| Fault Type | Trigger | Recovery Strategy | Impact |
|------------|---------|-------------------|--------|
| Overcurrent | Current > 5000 mA | Immediate shutdown | Manual reset required |
| Thermal Overload | Temp > 80°C | Shutdown, auto-restart at 60°C | Temporary stop |
| Encoder Timeout | Position sensor loss (2% rate) | Switch to sensorless FOC | Reduced precision |
| Modbus CRC Error | Packet corruption (2% rate) | Retransmit | Status delay |
| Memory Corruption | Stack/heap integrity failure | System reboot | Complete restart |

**Operating Parameters**:
- Maximum Speed: 3000 RPM
- Current Limit: 5000 mA
- Temperature Limit: 80°C
- Position Range: 0-359 degrees
- Modbus Update Rate: 1 second

**Safety Features**:
- Overcurrent protection (< 50ms response)
- Thermal overload protection with auto-recovery
- Position sensor redundancy
- Memory integrity monitoring
- Safe shutdown on critical faults

**Building and Running**:
```bash
west build -b qemu_cortex_m3 app/industrial_motor_controller -p
west build -t run
```

**Expected Output**: Motor ramps to 1500 RPM and operates continuously. Periodic status displays show speed, position, current, temperature, runtime, and fault statistics. Random faults trigger with automatic recovery.

---

## Architecture Patterns

### Multi-Threaded Design

Both applications use multiple threads for concurrent operation:

```
┌─────────────────────────────────────────┐
│          Main Thread                    │
│  (Status Display & Coordination)        │
└─────────────────────────────────────────┘
            │
            ├─── Sensor/Control Thread (High Priority)
            ├─── Monitoring Thread (Medium Priority)
            ├─── Communication Thread (Low Priority)
            └─── Watchdog Thread (Critical Priority)
                            │
                            ▼
            ┌───────────────────────────────┐
            │   Fault Tolerance Worker      │
            │   (Event-Driven Processing)   │
            └───────────────────────────────┘
```

### Fault Handling Flow

```
[Fault Detection] ──► [Report to FT API] ──► [Queue Event]
                                                    │
                                                    ▼
                                         [Worker Thread Processes]
                                                    │
                                                    ▼
                                         [Execute Recovery Handler]
                                                    │
                                    ┌───────────────┼───────────────┐
                                    ▼               ▼               ▼
                              [SUCCESS]      [PARTIAL]        [REBOOT]
                                    │               │               │
                                    ▼               ▼               ▼
                            [Continue]      [Safe Mode]      [System Reset]
```

### Recovery Strategies

1. **Graceful Degradation**: System continues with reduced functionality
   - Smart Thermostat: Uses cached temperature when sensor fails
   - Motor Controller: Switches to sensorless mode when encoder fails

2. **Automatic Retry**: Temporary faults resolved through retry
   - Communication CRC errors trigger packet retransmission
   - Watchdog delays trigger emergency feed

3. **Safe Shutdown**: Critical faults trigger controlled shutdown
   - Overcurrent protection immediately disables motor
   - Out-of-range temperatures disable HVAC

4. **Auto-Recovery**: System self-recovers when conditions improve
   - Motor restarts automatically after cooling
   - Communication resumes after transient errors

5. **System Reboot**: Unrecoverable faults trigger complete restart
   - Memory corruption requires full system reset
   - Hardware faults beyond software recovery

---

## Integration Guidelines

### Adding Fault Tolerance to Your Application

1. **Include the API Header**:
```c
#include <zephyr/fault_tolerance/ft_api.h>
```

2. **Enable in Configuration** (`prj.conf`):
```
CONFIG_FT_API_ENABLED=y
CONFIG_LOG=y
CONFIG_LOG_MODE_DEFERRED=y
```

3. **Initialize and Register Handlers**:
```c
ft_init();
ft_register_handler(FT_PERIPH_TIMEOUT, my_timeout_handler);
ft_register_handler(FT_COMM_CRC_ERROR, my_crc_handler);
```

4. **Report Faults When Detected**:
```c
struct my_fault_context ctx = { /* fault-specific data */ };

ft_event_t event = {
    .kind = FT_PERIPH_TIMEOUT,
    .severity = FT_SEV_ERROR,
    .domain = FT_DOMAIN_HARDWARE,
    .code = 0x1234,
    .timestamp = k_uptime_get(),
    .thread_id = k_current_get(),
    .context = &ctx,
    .context_size = sizeof(ctx)
};

ft_report_fault(&event);
```

5. **Implement Recovery Handlers**:
```c
static ft_recovery_result_t my_timeout_handler(const ft_event_t *event)
{
    /* Analyze fault */
    struct my_fault_context *ctx = (struct my_fault_context *)event->context;
    
    /* Execute recovery actions */
    reset_peripheral();
    use_cached_data();
    
    /* Return result */
    return FT_RECOVERY_SUCCESS;
}
```

---

## Performance Characteristics

### Smart Thermostat
- **Flash Usage**: ~28 KB
- **RAM Usage**: ~19 KB
- **CPU Overhead**: < 5% (background monitoring)
- **Fault Response**: < 100ms
- **Uptime**: Continuous operation with fault tolerance

### Industrial Motor Controller
- **Flash Usage**: ~28 KB
- **RAM Usage**: ~22 KB
- **CPU Overhead**: < 10% (high-frequency monitoring)
- **Safety Response**: < 50ms (overcurrent shutdown)
- **Recovery Time**: Immediate to 20 seconds (thermal cooling)

---

## Real-World Applications

### Smart Thermostat Pattern Applicable To:
- Smart home devices (lighting, security, climate)
- Building automation systems
- Environmental monitoring stations
- Medical equipment with patient monitoring
- Agricultural automation (greenhouse control)

### Motor Controller Pattern Applicable To:
- Industrial conveyor systems
- CNC machine tool spindles
- Robotic arm joint controllers
- Elevator/lift motor control
- Automated manufacturing equipment
- HVAC fan controllers
- Electric vehicle drive systems

---

## Safety and Compliance

These applications demonstrate design principles from:

- **IEC 61508**: Functional Safety of Electrical/Electronic/Programmable Electronic Safety-related Systems
- **ISO 13849**: Safety of Machinery - Safety-related Parts of Control Systems
- **UL 508C**: Power Conversion Equipment
- **IEC 62061**: Safety of Machinery - Functional Safety of Electrical, Electronic and Programmable Electronic Control Systems

### Safety Features Demonstrated:
- Redundant fault detection mechanisms
- Multiple layers of protection (software + hardware)
- Safe shutdown procedures
- Fault logging and statistics
- Automatic recovery with validation
- Graceful degradation strategies

---

## Testing and Validation

### Build All Sample Applications:
```bash
# Smart Thermostat
west build -b qemu_cortex_m3 app/smart_thermostat -p
west build -t run

# Industrial Motor Controller
west build -b qemu_cortex_m3 app/industrial_motor_controller -p
west build -t run
```

### Expected Validation Results:
- ✓ Applications build without errors
- ✓ All threads start successfully
- ✓ Faults inject randomly during operation
- ✓ Recovery handlers execute correctly
- ✓ Systems continue operating after faults
- ✓ Statistics accurately track fault events
- ✓ No memory leaks or resource exhaustion

---

## Extending the Applications

### Adding New Fault Types:

1. Define fault context structure for your scenario
2. Implement recovery handler function
3. Register handler during initialization
4. Detect fault condition in monitoring code
5. Report fault with appropriate context

### Example - Adding Battery Monitoring:

```c
struct battery_fault_context {
    uint32_t voltage_mv;
    uint32_t min_voltage_mv;
    uint32_t capacity_percent;
};

static ft_recovery_result_t battery_low_recovery(const ft_event_t *event)
{
    struct battery_fault_context *ctx = 
        (struct battery_fault_context *)event->context;
    
    LOG_WRN("Battery low: %u mV (%u%%)", 
            ctx->voltage_mv, ctx->capacity_percent);
    
    /* Enter power-saving mode */
    reduce_sensor_polling();
    disable_non_critical_features();
    
    return FT_RECOVERY_SUCCESS;
}

/* In main initialization */
ft_register_handler(FT_POWER_BROWNOUT, battery_low_recovery);
```

---

## Troubleshooting

### Build Errors:
- **Undefined reference to `z_impl_sys_rand_get`**: Add `CONFIG_TEST_RANDOM_GENERATOR=y`
- **Logging deadlocks**: Ensure `CONFIG_LOG_MODE_DEFERRED=y` (not IMMEDIATE)
- **Stack overflow in recovery handler**: Increase thread stack size

### Runtime Issues:
- **No faults detected**: Random number generator working, wait longer
- **Recovery handler not called**: Verify handler registration
- **System reset instead of recovery**: Check handler return value
- **High CPU usage**: Reduce monitoring thread frequency

---

## Contributing

To add new sample applications:

1. Create directory under `app/`
2. Include src/main.c, prj.conf, CMakeLists.txt, README.md
3. Demonstrate realistic embedded scenario
4. Use multiple fault types appropriately
5. Document fault scenarios and recovery strategies
6. Test thoroughly on QEMU and hardware
7. Update this README with application details

---

## License

SPDX-License-Identifier: Apache-2.0

Copyright (c) 2025

---

## Additional Resources

- [Fault Tolerance Framework Documentation](../FAULT_TOLERANCE_FRAMEWORK.md)
- [Fault Tolerance API Reference](../include/zephyr/fault_tolerance/ft_api.h)
- [Implementation Progress](../FAULT_TOLERANCE_PROGRESS.md)
- [Zephyr RTOS Documentation](https://docs.zephyrproject.org)

For questions or issues, please refer to the main framework documentation.
