# Smart Thermostat Application

## Overview

This application demonstrates the fault tolerance API in a realistic smart thermostat scenario. The system monitors temperature sensors, controls HVAC systems, and communicates status over UART, handling various faults that occur during operation.

## Application Architecture

### Threads
- **Sensor Thread**: Polls temperature sensor every 1 second, controls HVAC
- **Communication Thread**: Transmits status over UART every 5 seconds
- **Watchdog Thread**: Feeds system watchdog every 3 seconds
- **Main Thread**: Displays system status every 10 seconds

### Fault Types Demonstrated

1. **Peripheral Timeout** (FT_PERIPH_TIMEOUT)
   - Simulates I2C sensor timeouts (5% failure rate)
   - Recovery: Uses last known temperature value
   - Impact: HVAC continues operating with cached data

2. **Communication CRC Error** (FT_COMM_CRC_ERROR)
   - Simulates UART transmission errors (3% failure rate)
   - Recovery: Requests packet retransmission
   - Impact: Status update delayed by retry

3. **Watchdog Bark** (FT_WATCHDOG_BARK)
   - Simulates delayed watchdog feeding (1% failure rate)
   - Recovery: Emergency feed to prevent system reset
   - Impact: System continues without restart

4. **Application Assertion** (FT_APP_ASSERT)
   - Detects out-of-range temperature readings
   - Recovery: Enters safe mode, disables HVAC
   - Impact: System remains operational but conservative

## HVAC Control Logic

- **Target Temperature**: 22°C
- **Tolerance**: ±2°C
- **Heating**: Activates when temp < 20°C
- **Cooling**: Activates when temp > 24°C
- **Off**: When 20°C ≤ temp ≤ 24°C

## Building and Running

```bash
# Build for QEMU Cortex-M3
west build -b qemu_cortex_m3 app/smart_thermostat -p

# Run simulation
west build -t run
```

## Expected Output

The application runs continuously, displaying:
- Real-time temperature readings
- HVAC state changes
- Fault events and recoveries
- Periodic status summaries

## Key Features

- **Multi-threaded Architecture**: Demonstrates concurrent fault handling
- **Realistic Fault Scenarios**: Based on common embedded system issues
- **Graceful Degradation**: System continues operating despite faults
- **Statistical Monitoring**: Tracks all faults and recovery outcomes
- **Safe Mode Operation**: Falls back to safe state on critical errors

## Real-World Application

This design pattern is applicable to:
- Smart home devices (thermostats, lighting, security)
- Industrial process control
- Automotive climate control
- Medical equipment with redundancy requirements
- Any system requiring high reliability with degraded operation capability
