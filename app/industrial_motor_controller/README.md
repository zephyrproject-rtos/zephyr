# Industrial Motor Controller Application

## Overview

This application demonstrates fault tolerance in an industrial motor control system. It simulates a three-phase motor controller with current monitoring, thermal protection, position control via incremental encoder, and Modbus RTU communication.

## System Architecture

### Control Threads

1. **Current Monitor Thread** (50ms period, Priority 5)
   - Monitors motor current draw
   - Detects overcurrent conditions
   - Triggers immediate safety shutdown

2. **Temperature Monitor Thread** (500ms period, Priority 6)
   - Monitors motor winding temperature
   - Implements thermal overload protection
   - Manages auto-restart after cooling

3. **Position Control Thread** (100ms period, Priority 7)
   - Reads incremental encoder
   - Updates motor position
   - Handles encoder communication failures

4. **Modbus Communication Thread** (1s period, Priority 8)
   - Transmits motor status via Modbus RTU
   - Handles CRC errors with retransmission
   - Provides remote monitoring interface

### Safety Features

#### Overcurrent Protection
- **Limit**: 5000 mA
- **Response**: Immediate motor shutdown
- **Recovery**: Manual reset required
- **Hardware**: Would trigger E-stop in production

#### Thermal Overload Protection
- **Limit**: 80°C
- **Response**: Automatic shutdown
- **Recovery**: Auto-restart when temp < 60°C
- **Cooling Model**: Passive cooling simulation

#### Position Sensor Redundancy
- **Primary**: Incremental encoder (2% failure rate simulated)
- **Fallback**: Sensorless FOC using back-EMF estimation
- **Degradation**: Reduced precision but continued operation

#### Communication Resilience
- **Protocol**: Modbus RTU
- **Error Handling**: CRC validation with retransmission
- **Failure Impact**: Status updates delayed but system continues

## Fault Scenarios

### Critical Faults (Motor Shutdown)
1. **Overcurrent**: Current > 5000 mA
   - Cause: Mechanical jam, short circuit, overload
   - Action: Immediate shutdown, manual intervention required

2. **Thermal Overload**: Temperature > 80°C
   - Cause: Sustained high current, poor ventilation
   - Action: Shutdown until cooled to < 60°C

3. **Memory Corruption**: Stack/heap integrity check failure
   - Cause: Hardware fault, cosmic ray, power glitch
   - Action: System reboot required for safety

### Non-Critical Faults (Degraded Operation)
1. **Encoder Timeout**: Position sensor communication lost
   - Cause: Cable damage, electromagnetic interference
   - Action: Switch to sensorless control mode

2. **Modbus CRC Error**: Communication packet corruption
   - Cause: Electrical noise, cable issues
   - Action: Packet retransmission, operation continues

## Operating Parameters

| Parameter | Value | Unit |
|-----------|-------|------|
| Max Speed | 3000 | RPM |
| Current Limit | 5000 | mA |
| Temperature Limit | 80 | °C |
| Position Range | 0-359 | degrees |
| Modbus Update Rate | 1 | second |

## Building and Running

```bash
# Build for QEMU Cortex-M3
west build -b qemu_cortex_m3 app/industrial_motor_controller -p

# Run simulation
west build -t run
```

## Expected Behavior

1. **Startup**: Motor ramps up to 1500 RPM
2. **Normal Operation**: Continuous monitoring of all parameters
3. **Fault Injection**: Random faults occur during operation
4. **Recovery**: Automatic or manual recovery based on severity
5. **Status Display**: Comprehensive status every 8 seconds

## Real-World Applications

This design is applicable to:
- Industrial conveyor systems
- CNC machine tool spindles
- Robotic arm joint controllers
- Automated manufacturing equipment
- Elevator/lift motor control
- HVAC fan controllers

## Safety Compliance

This implementation demonstrates principles from:
- IEC 61508 (Functional Safety)
- ISO 13849 (Machinery Safety)
- UL 508C (Power Conversion Equipment)

## Performance Characteristics

- **Response Time**: < 50ms for overcurrent shutdown
- **Thermal Protection**: Continuous monitoring, auto-recovery
- **Communication**: Resilient to 2% packet error rate
- **Position Accuracy**: Maintains control during sensor faults
- **Uptime**: Maximized through graceful degradation
