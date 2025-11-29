# Autonomous Drone Flight Controller

## Overview

Fault-tolerant flight controller for autonomous quadcopter demonstrating graceful degradation in the face of sensor failures, communication loss, and system faults. Shows how critical flight systems can maintain safe operation even with multiple simultaneous failures.

## Flight Modes

1. **GPS_HOLD**: Maintain current GPS position (default)
2. **WAYPOINT**: Navigate to programmed waypoints
3. **RETURN_TO_LAUNCH (RTL)**: Autonomous return home
4. **STABILIZE**: Attitude hold using IMU only (GPS-denied)
5. **EMERGENCY_LAND**: Controlled emergency descent
6. **MANUAL**: Direct pilot control

## Fault Scenarios and Responses

### Sensor Failures

| Fault | Detection | Response | Flight Mode Change |
|-------|-----------|----------|-------------------|
| GPS Timeout | No fix for 1s (2% rate) | Switch to inertial nav | GPS_HOLD → STABILIZE |
| IMU Failure | Sensor error (0.5% rate) | Emergency landing | Any → EMERGENCY_LAND |
| Radio Loss | CRC errors (3% rate) | Autonomous RTL | Any → RTL |
| Motor Failure | Current/RPM fault | Emergency land | Any → EMERGENCY_LAND |

### Battery Management

| Battery Level | Action | Priority |
|---------------|--------|----------|
| < 20% | Return to launch | WARNING |
| < 10% | Emergency landing | CRITICAL |
| 0% | Controlled crash | EMERGENCY |

### Failsafe Hierarchy

1. **Level 1 (Graceful)**: GPS loss → Switch to stabilize mode
2. **Level 2 (RTL)**: Radio loss → Autonomous return home
3. **Level 3 (Emergency)**: IMU/Motor failure → Emergency landing
4. **Level 4 (Critical)**: Memory corruption → Immediate landing + reboot

## Safety Features

### Multi-Redundant Systems
- Dual navigation: GPS + Inertial (IMU + barometer)
- Failsafe modes: RTL on radio loss
- Battery monitoring with progressive warnings
- Motor failure detection and compensation

### Emergency Procedures
- **RTL (Return to Launch)**: Activated on radio loss or low battery
- **Emergency Land**: Rapid but controlled descent (1 m/s)
- **Controlled Crash**: Last resort if all systems failing

## Control Loop Architecture

```
Sensor Fusion (100 Hz) → Flight Controller (50 Hz) → Motor Control
         ↓                        ↓                         ↓
    GPS + IMU              PID Controllers           ESC Commands
         ↓                        ↓                         ↓
  Position Est.           Mode Logic              Motor Outputs
```

## Building and Running

```bash
west build -b qemu_cortex_m3 app/autonomous_drone -p
west build -t run
```

## Expected Behavior

1. Drone starts at 50m altitude in GPS_HOLD mode
2. Battery drains 1% every 5 seconds
3. Random faults inject during flight:
   - GPS timeouts (2%)
   - Radio CRC errors (3%)
   - IMU faults (0.5%)
4. System responds with appropriate mode changes
5. Low battery triggers RTL
6. Critical battery triggers emergency landing

## Real-World Applications

This architecture is used in:
- Consumer drones (DJI, Parrot, etc.)
- Commercial delivery drones (Amazon Prime Air, Zipline)
- Agricultural survey drones
- Search and rescue UAVs
- Military reconnaissance drones
- Package delivery systems

## Regulatory Compliance

Demonstrates principles from:
- **FAA Part 107**: Small UAS regulations
- **DO-178C**: Software for airborne systems
- **ARP4754A**: Development of civil aircraft systems
