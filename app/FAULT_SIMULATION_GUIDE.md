# Simulating Diverse Fault Types in Sample Applications

## Overview

The sample applications now demonstrate all 9 fault types supported by the Fault Tolerance API through realistic simulation scenarios. Each fault type is injected probabilistically during runtime to demonstrate real-world fault handling.

## Fault Type Simulation Breakdown

### 1. **FT_STACK_OVERFLOW**
**What it simulates**: Thread stack overflow when a thread exceeds its allocated stack space.

**Where simulated**:
- Smart Thermostat: *Not yet implemented*
- Motor Controller: Temperature monitoring thread (0.1% chance)
- Medical Pump: *Not yet implemented*
- Drone: *Not yet implemented*

**Simulation approach**:
```c
struct stack_context {
    const char *thread;
    uint32_t stack_size;
    uint32_t stack_used;
} ctx = {
    .thread = "temp_mon",
    .stack_size = 2048,
    .stack_used = 2100  // Overflow!
};
```

**Real-world equivalent**: Recursive function calls, large local arrays, excessive nested function calls.

---

### 2. **FT_HARDFAULT**
**What it simulates**: CPU exception from invalid memory access, divide by zero, or illegal instruction.

**Where simulated**:
- Smart Thermostat: *Not yet implemented*
- Motor Controller: Current monitoring thread (0.3% chance)
- Medical Pump: *Not yet implemented*
- Drone: Sensor fusion thread (0.5% chance)

**Simulation approach**:
```c
struct hardfault_context {
    uint32_t pc;          // Program counter
    uint32_t lr;          // Link register
    uint32_t fault_addr;  // Faulting address
} ctx = {
    .pc = 0x08001234,
    .lr = 0x08005678,
    .fault_addr = 0xDEADBEEF  // Invalid address
};
```

**Real-world equivalent**: NULL pointer dereference, accessing unmapped memory, stack corruption.

---

### 3. **FT_WATCHDOG_BARK**
**What it simulates**: System running slower than expected, watchdog timer warning before reset.

**Where simulated**:
- Smart Thermostat: Watchdog thread (1% chance)
- Motor Controller: Position control thread (0.5% chance)
- Medical Pump: Safety monitoring (periodic check)
- Drone: *Uses as IMU failure instead*

**Simulation approach**:
```c
struct watchdog_context {
    uint32_t bark_timeout_ms;
    uint32_t missed_feeds;
    const char *thread;
} ctx = {
    .bark_timeout_ms = 5000,
    .missed_feeds = 1,
    .thread = "watchdog_thread"
};
```

**Real-world equivalent**: System overload, infinite loop, thread starvation, priority inversion.

---

### 4. **FT_DEADLOCK_DETECTED**
**What it simulates**: Circular dependency between threads waiting for resources.

**Where simulated**:
- Smart Thermostat: Watchdog thread (0.5% chance)
- Motor Controller: *Not yet implemented*
- Medical Pump: *Not yet implemented*
- Drone: *Not yet implemented*

**Simulation approach**:
```c
struct deadlock_context {
    const char *thread1;
    const char *thread2;
    const char *resource;
} ctx = {
    .thread1 = "sensor_thread",
    .thread2 = "comm_thread",
    .resource = "i2c_bus"
};
```

**Real-world equivalent**: Thread A locks mutex1, waits for mutex2; Thread B locks mutex2, waits for mutex1.

---

### 5. **FT_MEM_CORRUPTION**
**What it simulates**: Data corruption detected via checksums or integrity checks.

**Where simulated**:
- Smart Thermostat: Sensor thread (0.2% chance)
- Motor Controller: *Using as critical memory fault*
- Medical Pump: *Using for dosage calculation integrity*
- Drone: *Using for flight control memory*

**Simulation approach**:
```c
struct mem_corruption_context {
    uint32_t expected_checksum;
    uint32_t actual_checksum;
    const char *location;
} ctx = {
    .expected_checksum = 0xDEADBEEF,
    .actual_checksum = 0xDEAD0000,  // Corruption!
    .location = "temperature_buffer"
};
```

**Real-world equivalent**: Cosmic rays flipping bits, DMA corruption, bad RAM, buffer overruns.

---

### 6. **FT_PERIPH_TIMEOUT**
**What it simulates**: Peripheral device not responding (I2C, SPI, UART timeouts).

**Where simulated**:
- Smart Thermostat: I2C temperature sensor (5% chance)
- Motor Controller: Encoder communication (2% chance)
- Medical Pump: Flow sensor I2C (varies)
- Drone: GPS timeout (2% chance)

**Simulation approach**:
```c
struct sensor_fault_context {
    const char *sensor_name;
    uint32_t timeout_ms;
    uint32_t read_count;
} ctx = {
    .sensor_name = "BME280_Temperature",
    .timeout_ms = 500,
    .read_count = sensor_read_count
};
```

**Real-world equivalent**: Disconnected sensor, electromagnetic interference, faulty wiring, broken peripheral.

---

### 7. **FT_COMM_CRC_ERROR**
**What it simulates**: Communication packet corruption detected via CRC/checksum.

**Where simulated**:
- Smart Thermostat: UART status transmission (3% chance)
- Motor Controller: Modbus RTU packets (2% chance)
- Medical Pump: *Using for air bubble detection (repurposed)*
- Drone: Radio telemetry packets (3% chance)

**Simulation approach**:
```c
struct comm_fault_context {
    const char *protocol;
    uint32_t expected_crc;
    uint32_t received_crc;
    uint32_t packet_id;
} ctx = {
    .protocol = "UART",
    .expected_crc = 0x12345678,
    .received_crc = 0x12345600,  // CRC mismatch!
    .packet_id = uart_tx_count
};
```

**Real-world equivalent**: Electrical noise, poor signal quality, EMI, loose connections.

---

### 8. **FT_POWER_BROWNOUT**
**What it simulates**: Voltage drop or power supply issues.

**Where simulated**:
- Smart Thermostat: *Not directly used*
- Motor Controller: Thermal overload (repurposed for temperature)
- Medical Pump: *Not directly used*
- Drone: Low battery warnings (< 20%, < 10%)

**Simulation approach**:
```c
struct power_context {
    uint32_t voltage_mv;
    uint32_t min_voltage_mv;
    uint32_t battery_percent;
} ctx = {
    .voltage_mv = 2900,
    .min_voltage_mv = 3000,  // Below minimum!
    .battery_percent = 15
};
```

**Real-world equivalent**: Battery drain, inadequate power supply, voltage regulator failure.

---

### 9. **FT_APP_ASSERT**
**What it simulates**: Application-level assertion failures, validation checks.

**Where simulated**:
- Smart Thermostat: Out-of-range temperature readings
- Motor Controller: Occlusion detection (repurposed for pressure)
- Medical Pump: Flow rate out of tolerance
- Drone: *Not directly used*

**Simulation approach**:
```c
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
```

**Real-world equivalent**: Invalid sensor readings, sanity check failures, configuration errors.

---

## Fault Injection Rates

| Fault Type | Smart Thermostat | Motor Controller | Medical Pump | Drone |
|------------|------------------|------------------|--------------|-------|
| STACK_OVERFLOW | - | 0.1% | - | - |
| HARDFAULT | - | 0.3% | - | 0.5% |
| WATCHDOG_BARK | 1% | 0.5% | Periodic | - |
| DEADLOCK_DETECTED | 0.5% | - | - | - |
| MEM_CORRUPTION | 0.2% | Critical only | Critical only | Critical only |
| PERIPH_TIMEOUT | 5% | 2% | Varies | 2% |
| COMM_CRC_ERROR | 3% | 2% | Repurposed | 3% |
| POWER_BROWNOUT | - | Thermal | - | Battery |
| APP_ASSERT | On invalid data | Pressure | Flow rate | - |

## Increasing Fault Diversity

To see more diverse faults in a given application, you can:

### 1. **Increase Simulation Time**
Rare faults (< 1% probability) may take several minutes to appear:
```bash
timeout 300 west build -t run  # Run for 5 minutes
```

### 2. **Increase Fault Injection Rates**
Edit the probability checks in the source code:
```c
// Change from 0.5% to 5% for more frequent deadlocks
if ((sys_rand32_get() % 1000) < 5) {  // Was: < 5, Make: < 50
    // Trigger deadlock
}
```

### 3. **Filter Output for Specific Faults**
```bash
west build -t run | grep -E "(HARDFAULT|DEADLOCK|STACK_OVERFLOW)"
```

### 4. **Force Fault Injection**
For testing, temporarily force a specific fault:
```c
// Force memory corruption on iteration 100
if (iteration == 100) {
    // Trigger FT_MEM_CORRUPTION
}
```

## Real-World vs. Simulated Faults

| Fault Type | Real Trigger | Simulated Trigger |
|------------|-------------|-------------------|
| Stack Overflow | Recursive calls, large arrays | Check stack usage counter |
| Hard Fault | NULL deref, bad pointer | Random probability |
| Watchdog Bark | System overload | Delayed feed simulation |
| Deadlock | Mutex circular wait | Simulated dependency |
| Mem Corruption | Bit flips, DMA errors | Checksum mismatch |
| Periph Timeout | Device failure | Random timeout |
| CRC Error | Noise, interference | CRC calculation error |
| Power Brownout | Low voltage | Battery level check |
| App Assert | Invalid state | Range validation |

## Adding New Fault Scenarios

To add a new fault simulation:

1. **Choose appropriate location** (which thread/function)
2. **Define context structure** with relevant data
3. **Set probability** (1% = 1 in 100 iterations)
4. **Create fault event** with proper kind, severity, domain
5. **Report fault** using `ft_report_fault()`
6. **Implement recovery handler** if not already registered

Example:
```c
/* In some monitoring thread */
if ((sys_rand32_get() % 100) < 2) {  // 2% chance
    struct my_fault_ctx {
        // Fault-specific data
    } ctx = { /* ... */ };

    ft_event_t event = {
        .kind = FT_HARDFAULT,
        .severity = FT_SEV_CRITICAL,
        .domain = FT_DOMAIN_SYSTEM,
        .code = 0x2999,
        .timestamp = k_uptime_get(),
        .thread_id = k_current_get(),
        .context = &ctx,
        .context_size = sizeof(ctx)
    };

    ft_report_fault(&event);
}
```

## Testing Specific Fault Types

To verify a particular fault type works:

```bash
# Test for 2 minutes and look for specific fault
timeout 120 west build -t run 2>&1 | \
  grep -E "FAULT DETECTED.*DEADLOCK" -A 5

# Count how many times each fault occurs
timeout 60 west build -t run 2>&1 | \
  grep "FAULT DETECTED" | \
  cut -d'=' -f2 | cut -d',' -f1 | \
  sort | uniq -c
```

## Conclusion

The sample applications now demonstrate **comprehensive fault coverage** across all 9 supported fault types. The probabilistic injection approach creates realistic fault scenarios that mirror real-world embedded system failures, showcasing the API's ability to handle diverse failure modes gracefully.
