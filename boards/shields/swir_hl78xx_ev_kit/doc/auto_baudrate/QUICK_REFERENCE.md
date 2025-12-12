# Copyright (c) 2025 Netfeasa Ltd.
# SPDX-License-Identifier: Apache-2.0

# HL78xx Auto Baud Rate - Quick Reference

## Enable Feature
```ini
# In prj.conf
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y  # Your target rate
```

## Common Configurations

### Preset 1: Smart Default (Recommended)
```ini
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="921600,9600,57600,38400,115200"
CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=4
CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=3
CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y
CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y
```
✅ **Use when:** General purpose, handles both known and unknown baud rates efficiently
⏱️ **Boot time impact:** ~50-100ms (if already at target), ~4-20s (if detection needed)
💡 **Why smart:** Only runs full detection if initial communication fails

### Preset 2: Ultra-Fast Boot (Known Rate)
```ini
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="921600"
CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=1
CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=1
CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y
```
✅ **Use when:** Known initial baud rate, minimal boot time required
⏱️ **Boot time impact:** ~50-100ms
⚠️ **Warning:** May fail if modem is at different rate

### Preset 3: High Speed
```ini
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="921600,460800,230400,115200"
CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=3
CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=3
CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y
CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y
```
✅ **Use when:** High throughput required
⏱️ **Boot time impact:** ~50ms (best), ~3-12s (worst)
⚠️ **Note:** Ensure UART hardware supports high speeds

### Preset 4: Robust Production
```ini
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="115200,57600,38400,19200,9600,921600"
CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=8
CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=5
CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y
CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y
```
✅ **Use when:** Field deployment, maximum reliability needed
⏱️ **Boot time impact:** ~50ms (best), ~40s (worst with retries)
💡 **Features:** Diagnostic fallback, comprehensive rate coverage


### Preset 5: Testing/Development (Non-Persistent)
```ini
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=n  # Don't save with AT&W
CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y
```
✅ **Use when:** Testing different baud rates temporarily
💡 **Note:** Modem reverts to previous rate after power cycle

## Supported Baud Rates
| Rate    | Kconfig Option                      | Use Case              |
|---------|-------------------------------------|-----------------------|
| 9600    | `CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_9600`    | Legacy, low-power     |
| 19200   | `CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_19200`   | Legacy                |
| 38400   | `CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_38400`   | Standard              |
| 57600   | `CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_57600`   | Standard              |
| 115200  | `CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600`  | Default (recommended) |
| 230400  | `CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_230400`  | High speed            |
| 460800  | `CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_460800`  | High speed            |
| 921600  | `CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600`  | Very high speed       |
| 3000000 | `CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_3000000` | Maximum (HL7812 only) |

## State Flow

```
Power ON
    ↓
[START_WITH_TARGET=y?] → Try target first → [AUTOBAUD_AT_BOOT=y?] → Immediate Detection
    ↓                                        ↓
Wait KSUP → [ONLY_IF_COMMS_FAIL=y?] → Try Target Rate First
    ↓                                        ↓
Post-Restart → Communication Success? → Skip Detection (Fast!)
    ↓                     ↓
    |              Communication Fails
    |                     ↓
    +----------→ Detect Baud Rate (tries each rate in list)
                          ↓
                  Rate Detected? → Switch to Target (AT+IPR)
                     ↓                      ↓
                   Retry          [CHANGE_PERSISTENT=y?]
                     ↓                      ↓
              Max Retries? → Yes → AT&W (Save to NVRAM)
                     ↓                      ↓
            Diagnostic State        Initialize Modem
                     ↓                      ↓
            [DIAGNOSING_FIRST=y?]    Normal Operation
                     ↓
              Try Auto-Baud Again
```

## Log Messages

### Success - Smart Default (No Detection Needed)
```
[hl78xx] Checking if modem is ready at target baud rate 921600...
[hl78xx] Modem ready at target rate, skipping detection
[hl78xx] Baud rate switch not needed
```

### Detection Triggered (Communication Failed)
```
[hl78xx] Modem not responding at target rate 921600, starting detection...
[hl78xx] Attempting recovery from diagnostic state...
[hl78xx] Trying AT+KSREP? and Failing
[hl78xx] Trying baud rate: 921600
[hl78xx] Trying baud rate: 9600
[hl78xx] Modem responded at 9600 baud
[hl78xx] Switching baud rate from 921600 to 9600
[hl78xx] Saving persistent baud rate with AT&W...
[hl78xx] Successfully switched to baud rate 9600
```

### Non-Persistent Mode
```
[hl78xx] Modem responded at 9600 baud
[hl78xx] Switching baud rate from 9600 to 921600
[hl78xx] Non-persistent mode: skipping AT&W
[hl78xx] Baud rate changed for this session only
```

### Failure
```
[hl78xx] Starting baud rate detection...
[hl78xx] Trying baud rate: 921600
[hl78xx] Trying baud rate: 9600
[hl78xx] Failed to detect modem baud rate
[hl78xx] Retrying baud rate detection (attempt 2/3)
[hl78xx] Max retries reached, consider increasing timeout or adding more rates
```

## Troubleshooting

| Symptom | Likely Cause | Solution |
|---------|--------------|----------|
| "Failed to detect modem baud rate" | Detection list doesn't include actual rate | Add more rates to `AUTOBAUD_DETECTION_BAUDRATES` |
| Times out on each rate | Modem not ready/responding | Increase `AUTOBAUD_TIMEOUT` (in **seconds**) or startup delay |
| Hangs on AT&W command | Sending AT&W before UART reconfig | **Bug fixed**: UART now reconfigured before AT&W |
| Detects but switch fails | AT+IPR not supported | Check modem firmware version |
| Communication lost after switch | UART hardware limitation | Try lower baud rate |
| Fails after disabling auto-baud | Modem at target rate, UART at default | Update device tree `current-speed` to match target rate |
| Multiple retries, then success | Timing/noise issues | Increase retry count and timeout |
| Fast boot not working | Wrong config combination | Enable both `START_WITH_TARGET_BAUDRATE=y` AND `ONLY_IF_COMMS_FAIL=y` |
| Detection runs every boot | `ONLY_IF_COMMS_FAIL` disabled | Enable it for smart detection |
| Rate doesn't persist | `CHANGE_PERSISTENT` disabled | Enable it: `CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y` |
| Boot hangs waiting for KSUP | Normal KSUP wait too long | Consider `AUTOBAUD_AT_BOOT=y` (advanced users only!) |

## Configuration Decision Tree

```
Start Here
    │
    ├─→ Do you know the modem's current baud rate?
    │       │
    │       ├─→ YES, it's at my target rate already
    │       │       └─→ Use Preset 2 ( Ultra-Fast Boot (Known Rate))
    │       │           Boot: ~50-100ms
    │       │
    │       └─→ NO, or it varies
    │               └─→ Continue below
    │
    ├─→ Is boot time critical (<200ms)?
    │       │
    │       ├─→ YES → Use Preset 5 (Skip KSUP)
    │       │          WARNING: Advanced users only!
    │       │
    │       └─→ NO → Continue below
    │
    ├─→ Is this for production/field deployment?
    │       │
    │       ├─→ YES → Use Preset 4 (Robust Production)
    │       │          Comprehensive coverage, diagnostic fallback
    │       │
    │       └─→ NO → Continue below
    │
    ├─→ Do you need high-speed communication (>115200)?
    │       │
    │       ├─→ YES → Use Preset 3 (High Speed)
    │       │          Set target to 921600 or 460800
    │       │
    │       └─→ NO → Use Preset 1 (Smart Default)
    │                  Best balance of speed and reliability
    │
    └─→ Special Cases:
            ├─→ Testing different rates → Preset 6 (Non-Persistent)
            └─→ Unknown environment → Preset 1 or 4
```

## AT Commands Reference

```bash
# Query current baud rate
AT+IPR?
# Response: +IPR: 115200

# Set baud rate to 115200
AT+IPR=115200
# Response: OK
# Note: New rate takes effect after ~2 seconds

# Save configuration to NVRAM (required for persistence)
AT&W
# Response: OK
# Note: Without this, baud rate reverts on power cycle
```

## API Functions (Internal)

```c
/* Try communication at specific baud rate */
int hl78xx_try_baudrate(struct hl78xx_data *data, uint32_t baudrate);

/* Detect current baud rate from list */
int hl78xx_detect_current_baudrate(struct hl78xx_data *data);

/* Switch to target baud rate */
int hl78xx_switch_baudrate(struct hl78xx_data *data, uint32_t target_baudrate);
```

## Shell Commands (Future)
```bash
# Manually trigger detection
modem autobaud detect

# Show current rate
modem autobaud status

# Set new rate
modem autobaud set 115200
```
*(Not yet implemented)*

## Performance Tips

1. **Enable smart defaults:** Set `ONLY_IF_COMMS_FAIL=y` and `START_WITH_TARGET_BAUDRATE=y` for optimal boot times
2. **Order matters:** Put most likely rate first in detection list
3. **Adjust timeout wisely:** Shorter timeouts (1-2 seconds) for known environments, longer (5-8 seconds) for robust detection
4. **Limit detection list:** Only include rates you actually need to minimize detection time
5. **Consider AUTOBAUD_AT_BOOT:** For ultra-fast boot (<200ms) but understand the risks (modem might not be ready)
6. **Enable persistence:** With `CHANGE_PERSISTENT=y`, rate is saved and detection runs only once
7. **Diagnostic fallback:** Enable `AUTOBAUD_DIAGNOSING_FIRST=y` to recover from baud rate mismatches automatically

## Compatibility

- ✅ HL7812 (all firmware versions)
- ✅ HL7800 (all firmware versions)
- ✅ Works with Zephyr 4.4 and newer
- ✅ Compatible with all supported boards
- ✅ Smart flags reduce typical boot time from 20s to 50-100ms

## When NOT to Use

- ❌ Baud rate is fixed and known AND device tree matches (adds boot overhead)
  * But consider using with `ONLY_IF_COMMS_FAIL=y` for safety (minimal overhead)
- ❌ Ultra-fast boot required (<50ms total) - even smart defaults take ~50-100ms
- ❌ Memory-constrained systems (<2KB code space available)
- ❌ Using modem's hardware auto-baud feature (not applicable to HL78xx)

## Build Example

```bash
# Create new build with smart default auto-baud
west build -b pinnacle_100_dvk -p -- \
  -DCONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y \
  -DCONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y \
  -DCONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y \
  -DCONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y \
  -DCONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=3

# Flash to board
west flash

```

## Support

For issues or questions:
1. Check `AUTO_BAUDRATE_README.md` for detailed documentation
2. Review `IMPLEMENTATION_SUMMARY.md` for technical details
3. Enable verbose logging: `CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG=y`
4. Check Zephyr modem driver documentation
5. Review troubleshooting table above for common issues

---
**Last Updated:** 2025-12-06
