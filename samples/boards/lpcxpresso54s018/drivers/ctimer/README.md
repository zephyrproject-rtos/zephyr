# LPC54S018 CTIMER Sample

## Overview

This sample demonstrates the use of the CTIMER (Counter/Timer) peripheral on the LPC54S018.
The sample configures CTIMER0 to generate periodic interrupts every second, which are used to:

- Toggle an LED
- Display elapsed time on the console
- Demonstrate timer value reading

## Requirements

- LPCXpresso54S018M board
- Zephyr RTOS with LPC54S018 support
- USB cable for console output

## Building and Running

### From Linux/WSL2:
```bash
# Navigate to project root
cd /mnt/e/ZephyrDevelop/lpc54s018

# Build the sample
./build.sh -s samples/drivers/ctimer

# Flash the binary to the board
```

### From Windows:
```cmd
# Navigate to project root
cd E:\ZephyrDevelop\lpc54s018

# Build the sample
build-windows.cmd -s samples/drivers/ctimer
```

## Expected Output

When the sample runs, you should see:

1. The red LED (or configured LED) toggling every second
2. Console output showing:
   ```
   *** Booting Zephyr OS build v4.2.0 ***
   [00:00:00.000,000] <inf> ctimer_sample: LPC54S018 CTIMER Sample
   [00:00:00.000,000] <inf> ctimer_sample: Timer frequency: 96000000 Hz
   [00:00:00.000,000] <inf> ctimer_sample: Timer configured for 1 second period
   [00:00:00.000,000] <inf> ctimer_sample: LED will toggle every second...
   [00:00:01.000,000] <inf> ctimer_sample: Timer fired! Elapsed time: 1 seconds
   [00:00:02.000,000] <inf> ctimer_sample: Timer fired! Elapsed time: 2 seconds
   ...
   [00:00:10.000,000] <inf> ctimer_sample: Current timer value: 960000000 us
   ```

## Technical Details

### CTIMER Configuration

The sample uses CTIMER0 with the following configuration:
- Clock source: APB clock (96MHz)
- Mode: Timer mode
- Prescaler: 0 (no prescaling)
- Interrupt: Every 1 second

### Key Functions Used

1. **counter_start()** - Start the timer
2. **counter_get_frequency()** - Get timer frequency
3. **counter_us_to_ticks()** - Convert microseconds to timer ticks
4. **counter_set_channel_alarm()** - Set up periodic interrupts
5. **counter_get_value()** - Read current timer value

### Device Tree Configuration

The CTIMER is configured in the device tree with:
```dts
&ctimer0 {
    status = "okay";
};
```

## Customization

### Change Timer Period

Modify `TIMER_PERIOD_SEC` in the source code:
```c
#define TIMER_PERIOD_SEC 2  /* 2 second period */
```

### Use Different CTIMER

Change the timer node:
```c
#define TIMER_NODE DT_NODELABEL(ctimer1)  /* Use CTIMER1 */
```

Enable the corresponding timer in the device tree overlay.

### Add More Functionality

The callback function can be extended to:
- Measure precise time intervals
- Generate PWM signals
- Create delays
- Implement a real-time clock

## Troubleshooting

1. **Timer not starting**: Ensure CTIMER0 is enabled in device tree
2. **LED not toggling**: Verify LED GPIO configuration
3. **No console output**: Check UART connection and baud rate (115200)

## References

- [Zephyr Counter API Documentation](https://docs.zephyrproject.org/latest/reference/drivers/counter.html)
- NXP LPC54S018 Reference Manual - Chapter: CTIMER