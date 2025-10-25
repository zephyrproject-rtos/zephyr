# Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

"""
Vendor default argument profiles for `west monitor`.

These profiles allow vendors to predefine monitor defaults such as reset
sequences, baud rate, and line mapping without modifying the monitor source.

Pure data only — no code, no imports. Each vendor entry maps argparse
destination names to preferred default values. Applied only if user didn't
explicitly override via CLI flags.

-------------------------------------------------------------------------------
AVAILABLE KEYS
-------------------------------------------------------------------------------

Connection:
  baud                  (int)   - Baud rate (default 115200)
  rtscts                (bool)  - Enable hardware flow control (RTS/CTS)

Reset Behavior:
  reset                 (str)   - One of: 'none', 'run', 'boot', 'dtr-pulse', 'rts-pulse'
  reset_on_connect      (bool)  - Perform reset after initial connection
  reset_on_reconnect    (bool)  - Perform reset after auto-reconnects
  reset_delay_ms        (int)   - Delay between line toggles (milliseconds)
  reset_hold_ms         (int)   - Hold time for asserted lines (milliseconds)
  line_map              (str)   - Line mapping: 'dtr-boot,rts-reset' or 'rts-boot,dtr-reset'
  invert_reset          (bool)  - Invert polarity of reset line
  invert_boot           (bool)  - Invert polarity of boot line
  idle_lines            (str)   - Idle mode: 'released', 'boot-hold', 'reset-hold'

Read Mode:
  read_mode             (str)   - 'auto' (LF/CR + idle), 'line' (LF/CR only), 'raw' (stream)
  idle_flush_ms         (int)   - Idle timeout for auto mode (milliseconds)

Display:
  timestamp             (bool)  - Prepend timestamps to output
  no_color              (bool)  - Disable colored output

Addr2line:
  decode                (str)   - 'off', 'inline', 'side'
-------------------------------------------------------------------------------
"""

PROFILES = {
    # Espressif ESP32 family
    "espressif": {
        "reset": "run",
        "reset_on_connect": True,
        "line_map": "rts-boot,dtr-reset",  # ESP32 uses inverted mapping
        "baud": 115200,
    },
}
