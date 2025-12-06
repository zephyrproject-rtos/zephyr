# Copyright (c) 2025 Netfeasa Ltd.
# SPDX-License-Identifier: Apache-2.0

# HL78xx Auto Baud Rate Switching

## Overview

This feature enables automatic baud rate detection and switching for the Sierra Wireless HL78xx modem driver. The driver can automatically detect the modem's current baud rate and switch to a configured target baud rate using the AT+IPR command.

## Features

- **Auto-detection**: Automatically detects the modem's current baud rate by trying a list of common baud rates
- **Dynamic switching**: Changes the modem's baud rate to the configured target using AT+IPR command
- **Configurable**: Supports multiple baud rates from 9600 to 921600 bps
- **Retry mechanism**: Configurable retry count for robust detection
- **State machine integration**: Seamlessly integrated into the modem initialization sequence

## Configuration

Enable the feature in your project's `prj.conf`:

```ini
# Enable auto baud rate detection and switching
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y

# Set target baud rate (default is 115200)
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y

# Configure detection baud rates (comma-separated list)
CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="115200,9600,57600,38400,19200"

# Set timeout for each detection attempt (in seconds)
CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=4

# Set number of retries
CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=3

# Save baud rate change to modem NVRAM (default: y, recommended)
CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y

# Try target baud rate first before detection list (default: y, faster)
CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y

# Only perform auto-baud if initial communication fails (default: y, efficient)
CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y

# Perform auto-baud immediately at boot, skip KSUP wait (default: n)
CONFIG_MODEM_HL78XX_AUTOBAUD_AT_BOOT=n
```

## Supported Baud Rates

The following baud rates are supported:

- 9600 bps
- 19200 bps
- 38400 bps
- 57600 bps
- 115200 bps (default)
- 230400 bps
- 460800 bps
- 921600 bps
- 3000000 bps (HL7812 only)

## How It Works

### 1. Detection Phase (MODEM_HL78XX_STATE_SET_BAUDRATE)

When the modem enters the `SET_BAUDRATE` state after power-on:

1. The driver tries each baud rate from the detection list
2. For each rate, it:
   - Configures the UART to that baud rate
   - Sends an "AT" command
   - Waits for "OK" response
3. Once a response is received, the current baud rate is identified

### 2. Switching Phase

If the detected baud rate differs from the target:

1. Send `AT+IPR=<target_baudrate>` command at current baud rate
2. Wait for "OK" response
3. Wait 2.5 seconds for modem to apply the change (~2s per AT command guide)
4. Reconfigure the host UART to the new baud rate
5. Wait 50ms for UART stabilization
6. Verify communication by sending "AT" test command
7. Send `AT&W` to save configuration to NVRAM (persists across power cycles)

**Note:** After baud rate switching completes, the state machine runs the post-restart script to wait for KUSP URC message before proceeding to initialization.

### 3. State Transitions

```
AWAIT_POWER_ON --> SET_BAUDRATE --> RUN_INIT_SCRIPT
                        |
                        +--> [On Failure] --> RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT
```

## Use Cases

### Use Case 1: Unknown Modem Baud Rate

If you're unsure what baud rate the modem is currently using:

```ini
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="115200,9600,57600,38400,19200"
```

The driver will automatically detect and switch to your preferred rate.

### Use Case 2: High-Speed Communication

To maximize throughput, configure a high baud rate:

```ini
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
```

### Use Case 3: Low-Power Applications

For low-power applications where you want to minimize UART activity:

```ini
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_9600=y
```

### Use Case 4: Fast Boot with Known Baud Rate

If you know the modem is already at your target baud rate and want fastest boot:

```ini
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y
CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=2  # Short timeout
```

**Result:** Boot time ~50-100ms if modem is already at target rate!

### Use Case 5: Production/Field Deployment

For maximum reliability in unknown environments:

```ini
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="115200,57600,38400,19200,9600"
CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=8  # Longer timeout
CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=5
CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y
CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y
```

### Use Case 6: Temporary Baud Rate Change (Non-Persistent)

For testing or temporary rate changes that revert on modem reboot:

```ini
CONFIG_MODEM_HL78XX_AUTO_BAUDRATE=y
CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_921600=y
CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=n  # Don't save with AT&W
```

**Note:** Modem will revert to previous baud rate after power cycle or AT+CFUN reset.

## AT Commands Used

### Query Current Baud Rate
```
AT+IPR?
```
Response: `+IPR: <rate>` or `+IPR: 0` (auto-baud)

### Set Fixed Baud Rate
```
AT+IPR=<rate>
```
Where `<rate>` can be: 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 3000000 (HL7812 only)

**Important:** The new baud rate takes effect after ~2 seconds. Use `AT&W` to save the configuration to non-volatile memory, otherwise it will revert to previous setting on power cycle or phone functionality changed.

## Troubleshooting

### Issue: Detection Fails

**Symptoms**: Modem enters diagnostic state after multiple retries

**Solutions**:
1. Verify your detection baud rate list includes the modem's current rate
2. Increase timeout: `CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=10` (10 seconds)
3. Check UART wiring and signal quality
4. Verify modem is powered and responsive


### Issue: Switching Fails

**Symptoms**: Detection succeeds but switching to target rate fails

**Solutions**:
1. Ensure target baud rate is supported by both modem and UART hardware
2. Check for UART clock limitations
3. Try a lower baud rate first
4. Verify modem firmware supports AT+IPR command

### Issue: Communication Lost After Switch

**Symptoms**: Modem stops responding after baud rate change, hangs on AT&W command

**Root Cause**: Trying to send AT&W before reconfiguring host UART causes mismatch (host at old rate, modem at new rate)
**Solutions**:
1. **Critical:** Reconfigure host UART BEFORE sending AT&W command
2. Wait 2.5 seconds after AT+IPR for modem to apply change
3. Wait for KUSP URC message after baud rate switch
4. Check for UART buffer overflow
5. Ensure proper UART flow control if using high baud rates

### Issue: Communication Fails After Disabling Auto-Baud

**Symptoms**: Modem doesn't respond after disabling `CONFIG_MODEM_HL78XX_AUTO_BAUDRATE`

**Root Cause**: Once auto-baud was enabled and used, the modem's baud rate was changed and saved to NVRAM with `AT&W`. When you disable auto-baud, the driver no longer detects/switches, but the modem is still at the target baud rate (e.g., 921600). If your device tree UART is configured for 115200, communication will fail.

**Solutions**:
1. **Option A:** Keep auto-baud enabled (recommended)
2. **Option B:** Update device tree UART `current-speed` property to match the target baud rate:
   ```dts
   &uart_modem {
       current-speed = <921600>; /* Match your target rate */
   };
   ```
3. **Option C:** Manually reset modem baud rate using AT commands at the saved rate before disabling auto-baud

**Note:** There is no Kconfig option to change UART baud rate - it must be configured in the device tree.

## Implementation Details

### Key Functions

```c
/* Try communication at specific baud rate */
static int hl78xx_try_baudrate(struct hl78xx_data *data, uint32_t baudrate);

/* Detect current modem baud rate */
static int hl78xx_detect_current_baudrate(struct hl78xx_data *data);

/* Switch to target baud rate */
static int hl78xx_switch_baudrate(struct hl78xx_data *data, uint32_t target_baudrate);
```

### State Handler

```c
/* State entry handler */
static int hl78xx_on_set_baudrate_state_enter(struct hl78xx_data *data);

/* State event handler */
static void hl78xx_set_baudrate_event_handler(struct hl78xx_data *data, enum hl78xx_event evt);
```

### Data Structure

```c
struct uart_status {
#ifdef CONFIG_MODEM_HL78XX_AUTO_BAUDRATE
    uint32_t current_baudrate;
    uint32_t target_baudrate;
    uint8_t baudrate_detection_retry;
#endif
};
```

## Advanced Configuration Options

### CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT

**Default:** `y` (recommended)

Controls whether the baud rate change is saved to modem's NVRAM using AT&W command.

- **Enabled (y):** Baud rate persists across power cycles and reboots
- **Disabled (n):** Baud rate reverts to previous setting on modem reset

**Recommendation:** Always enable in production. This allows detection of unexpected modem reboots and quick recovery since the modem remembers the configured rate.

### CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE

**Default:** `y` (recommended)

When enabled, the driver first attempts communication at the configured target baud rate before trying the detection list.

**Benefits:**
- Significantly faster boot when modem is already at target rate (~50ms vs several seconds)
- Reduces UART traffic and power consumption
- Works well with `AUTOBAUD_ONLY_IF_COMMS_FAIL`

**When to disable:** If you frequently switch between different baud rates and want comprehensive detection every boot.

### CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL

**Default:** `y` (recommended)

Only performs auto baud rate detection if initial UART communication with the modem fails.

**How it works:**
1. Driver attempts normal initialization at target baud rate
2. If modem responds correctly → skip auto-baud detection entirely
3. If communication fails → trigger full auto-baud detection

**Benefits:**
- Minimal boot time impact when modem is already configured
- Automatic recovery from baud rate mismatches
- Best of both worlds: fast boot + robust recovery

**When to disable:** If you want to force detection every boot regardless of communication success.

### CONFIG_MODEM_HL78XX_AUTOBAUD_AT_BOOT

**Default:** `n`

Performs auto baud rate detection immediately as soon as the modem is powered up, without waiting for the KSUP URC (startup ready indication).

**Benefits:**
- Fastest possible modem initialization
- Useful for time-critical applications

**Warnings:**
- Skips waiting for modem's normal boot sequence
- May attempt communication before modem is fully ready
- Can interfere with modem's internal startup process
- **Use only if you understand the modem's boot behavior**

**When to enable:** Ultra-fast boot requirements where you control the exact timing of modem power-up.

## Optimization Strategies

### Strategy 1: Known Environment (Fastest)
```ini
# Modem is already at target rate, just verify and move on
CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y
CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y
CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=1  # Very short
CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="115200"  # Just target rate
```
**Boot impact:** ~50-100ms

### Strategy 2: Unknown Environment (Balanced)
```ini
# Detect efficiently, fall back to comprehensive search if needed
CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y
CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y
CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=4
CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="115200,57600,9600"
```
**Boot impact:** ~50ms (best case) to ~12s (worst case)

### Strategy 3: Maximum Reliability (Production)
```ini
# Comprehensive detection with diagnostic fallback
CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE=y
CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL=y
CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT=y
CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT=8
CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT=5
CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES="115200,57600,38400,19200,9600"
```
**Boot impact:** ~50ms (best case) to ~40s (worst case with retries)

## Performance Considerations

### Detection Time

With default settings (5 baud rates @ 4 seconds timeout):
- Best case: ~50ms (target rate succeeds via `START_WITH_TARGET_BAUDRATE`)
- Worst case: ~20s (all rates tried)
- Average: ~10s
- With `ONLY_IF_COMMS_FAIL`: ~50ms in normal operation, falls back to detection only on failure

### Optimization Tips

1. **Order detection list by likelihood**: Put most common baud rates first
2. **Enable smart detection**: Use `START_WITH_TARGET_BAUDRATE=y` and `ONLY_IF_COMMS_FAIL=y`
3. **Reduce timeout**: If your system is fast and reliable, use shorter timeout (e.g., 2000ms)
4. **Limit detection list**: Only include rates you actually use
5. **Use persistent mode**: Enable `AUTOBAUD_CHANGE_PERSISTENT=y` to avoid re-detection after modem reboots

## Future Enhancements

Potential improvements for this feature:

1. **Persistent storage**: Save detected/configured baud rate to avoid re-detection
2. **Dynamic adjustment**: Adjust baud rate based on requirement or link quality
3. **Statistics**: Track detection attempts and success rates
4. **Shell commands**: Runtime baud rate query and adjustment

## References

- Sierra Wireless HL78xx AT Command Reference
- HL78xx Product Technical Specification
- Zephyr UART Driver API Documentation
- Zephyr Modem API Documentation

## License

Copyright (c) 2025 Netfeasa Ltd.
SPDX-License-Identifier: Apache-2.0
---
**Last Updated:** 2025-12-06