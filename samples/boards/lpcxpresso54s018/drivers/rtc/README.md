# LPC54S018 RTC Sample

## Overview

This sample demonstrates the use of the Real-Time Clock (RTC) peripheral on the LPC54S018.
The RTC provides accurate timekeeping that continues to run even in low-power modes.

## Features Demonstrated

- Setting and reading RTC time
- Configuring RTC alarms
- Displaying formatted time
- Using RTC interrupts to trigger actions

## Requirements

- LPCXpresso54S018M board
- Zephyr RTOS with LPC54S018 support
- USB cable for console output
- Optional: Battery backup for RTC (if available on board)

## Building and Running

### From Linux/WSL2:
```bash
# Navigate to project root
cd /mnt/e/ZephyrDevelop/lpc54s018

# Build the sample
./build.sh -s samples/drivers/rtc

# Flash the binary to the board
```

### From Windows:
```cmd
# Navigate to project root
cd E:\ZephyrDevelop\lpc54s018

# Build the sample
build-windows.cmd -s samples/drivers/rtc
```

## Expected Output

When the sample runs, you should see:

```
*** Booting Zephyr OS build v4.2.0 ***
[00:00:00.000,000] <inf> rtc_sample: LPC54S018 RTC Sample
[00:00:00.000,000] <inf> rtc_sample: RTC frequency: 1 Hz
[00:00:00.000,000] <inf> rtc_sample: Max relative alarm: 4294967295 ticks (4294967295 seconds)
[00:00:00.000,000] <inf> rtc_sample: RTC initialized to 2024-01-01 00:00:00
[00:00:00.000,000] <inf> rtc_sample: Current RTC time: 2024-01-01 00:00:00
[00:00:00.000,000] <inf> rtc_sample: Alarm set for: 2024-01-01 00:00:10
[00:00:05.000,000] <inf> rtc_sample: Current time: 2024-01-01 00:00:05
[00:00:10.000,000] <inf> rtc_sample: RTC Alarm! Current time: 00:00:10
[00:00:10.000,000] <inf> rtc_sample: Current time: 2024-01-01 00:00:10
```

The LED will toggle every time the alarm fires (every 10 seconds).

## Technical Details

### RTC Configuration

The LPC54S018 RTC features:
- 1Hz clock source (32.768 kHz crystal divided down)
- Calendar functionality
- Alarm capability
- Battery backup support (hardware dependent)

### Key Functions Used

1. **counter_start()** - Start the RTC (if not already running)
2. **counter_get_value()** - Read current RTC counter value
3. **counter_set_value()** - Set RTC time (if supported)
4. **counter_set_channel_alarm()** - Configure RTC alarm
5. **counter_is_counting_up()** - Check if RTC is running

### Time Conversion

The sample uses standard C library functions for time manipulation:
- `gmtime_r()` - Convert time_t to struct tm
- `mktime()` - Convert struct tm to time_t
- `snprintf()` - Format time for display

### Device Tree Configuration

Enable the RTC in your board's device tree:
```dts
&rtc {
    status = "okay";
};
```

## Customization

### Change Alarm Interval

Modify `ALARM_INTERVAL_SEC` in the source code:
```c
#define ALARM_INTERVAL_SEC 60  /* Alarm every minute */
```

### Set Different Initial Time

Modify the `init_time` structure:
```c
struct tm init_time = {
    .tm_year = 2024 - 1900,  /* Year 2024 */
    .tm_mon = 5,             /* June (0-11) */
    .tm_mday = 15,           /* 15th */
    .tm_hour = 12,           /* Noon */
    .tm_min = 0,
    .tm_sec = 0
};
```

### Add Calendar Features

Extend the sample to:
- Display day of week
- Calculate elapsed days
- Implement stopwatch functionality
- Add multiple alarms

## Power Management

The RTC is designed to run in low-power modes:
- Continues counting during sleep
- Can wake the system from deep sleep
- Minimal power consumption (typically < 1µA)

## Troubleshooting

1. **RTC not counting**: Check if 32.768 kHz crystal is present and enabled
2. **Time resets on power cycle**: No battery backup present
3. **Alarm not firing**: Verify interrupt configuration in device tree
4. **Wrong time displayed**: Check time zone assumptions (sample uses UTC)

## Battery Backup

If your board has RTC battery backup:
- The RTC will maintain time during power loss
- No reinitialization needed on power-up
- Battery typically lasts several years

## References

- [Zephyr Counter API Documentation](https://docs.zephyrproject.org/latest/reference/drivers/counter.html)
- NXP LPC54S018 Reference Manual - Chapter: RTC
- [POSIX Time Functions](https://pubs.opengroup.org/onlinepubs/9699919799/functions/time.html)