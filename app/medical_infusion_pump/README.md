# Medical Infusion Pump Application

## Overview

Safety-critical medical infusion pump controller that demonstrates fault tolerance in a device where failures can directly impact patient safety. This application shows how the FT API enables safe operation through multiple redundant safety checks and immediate fault response.

## Clinical Scenario

Target: Deliver 500 mL of IV fluid at 100 mL/hour over 5 hours.

## Safety Features

### Critical Safety Mechanisms
1. **Air Bubble Detection**: Immediate emergency stop (no auto-restart)
2. **Occlusion Detection**: High pressure triggers alarm, attempts clearance
3. **Flow Rate Monitoring**: Ensures accurate dosing within ±10%
4. **Sensor Redundancy**: Detects and recovers from sensor failures
5. **Memory Integrity**: Protects dosage calculation integrity

### Safety Response Times
- Air bubble detection: < 200ms to full stop
- Occlusion detection: < 500ms
- Flow error correction: < 1 second
- Sensor timeout: Immediate safety shutdown

## Fault Scenarios

| Fault | Trigger | Severity | Recovery | Patient Risk |
|-------|---------|----------|----------|--------------|
| Air Bubble | Ultrasonic sensor (1% rate) | CRITICAL | Manual only | HIGH |
| Occlusion | Pressure > 300 mmHg (3% rate) | WARNING | Auto-clear | MEDIUM |
| Flow Error | Rate outside ±10% | ERROR | Recalibrate | MEDIUM |
| Sensor Timeout | Communication loss (2% rate) | CRITICAL | Reset/Reboot | HIGH |
| Memory Corruption | Integrity check fail | CRITICAL | Reboot | CRITICAL |

## Safety Compliance

Demonstrates principles from:
- **IEC 60601-1**: Medical electrical equipment safety
- **IEC 62304**: Medical device software lifecycle
- **FDA 21 CFR Part 820**: Quality system regulation

## Building and Running

```bash
west build -b qemu_cortex_m3 app/medical_infusion_pump -p
west build -t run
```

## Expected Behavior

1. Pump starts infusing at 100 mL/hr
2. Safety monitors continuously check pressure, flow, air bubbles
3. Random safety events trigger appropriate responses
4. Critical events (air bubbles) require manual intervention
5. Non-critical events auto-recover after corrective action
6. Infusion completes after delivering 500 mL

## Real-World Application

This design pattern is required for:
- IV infusion pumps
- Syringe pumps
- Patient-controlled analgesia (PCA) devices
- Dialysis machines
- Chemotherapy infusion systems
