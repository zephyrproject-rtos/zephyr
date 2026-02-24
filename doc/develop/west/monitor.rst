.. _west-monitor:

Serial Monitor: ``west monitor``
################################

.. tip:: Run ``west monitor -h`` for a quick overview.

The ``west monitor`` command provides a Zephyr-optimized serial monitor with
automatic reconnection, reset control, and layered YAML-based configuration.

.. only:: html

   .. contents::
      :local:

Basics
******

The simplest usage auto-detects the first available USB/ACM serial port::

  west monitor

Specify port and baud rate explicitly::

  west monitor -p /dev/ttyUSB0 -b 921600

A typical development workflow::

  west build -p -b esp32c3_devkitc samples/hello_world
  west flash
  west monitor

Interactive Controls
********************

The following keyboard shortcuts are available during monitoring:

- :kbd:`Ctrl-C` or :kbd:`Ctrl-]` -- Exit monitor
- :kbd:`Ctrl-R` -- Trigger reset (when ``--reset`` is configured)

The exit character and reset key can be customized with ``--exit-char`` and
``--reset-key`` respectively.

Auto-Reconnect
**************

The monitor automatically reconnects when the device:

- Is unplugged and replugged
- Resets and re-enumerates USB
- Temporarily disconnects during firmware updates

No manual restart of the monitor is required.

.. _west-monitor-reset:

Reset Control
*************

The monitor provides generic reset control using DTR/RTS serial lines::

  west monitor --reset run --reset-on-connect

Supported reset modes:

- ``none`` -- No reset (default)
- ``run`` -- Normal reset (assert/deassert reset line)
- ``boot`` -- Enter bootloader mode (hold boot, pulse reset)
- ``dtr-pulse`` -- Pulse DTR line
- ``rts-pulse`` -- Pulse RTS line

Line Mapping
============

The ``--line-map`` option controls which physical serial lines (DTR/RTS)
correspond to which logical functions (boot/reset). The default mapping is
``dtr-boot,rts-reset``.

Valid tokens are: ``dtr-boot``, ``dtr-reset``, ``rts-boot``, ``rts-reset``.

Line Polarity
=============

Many boards invert the reset and boot lines through transistors. The
``--invert-reset`` and ``--invert-boot`` flags (enabled by default) account
for this inversion.

Use ``--no-invert-reset`` or ``--no-invert-boot`` for boards with
non-inverted lines.

Idle Line State
===============

The ``--idle-lines`` option controls the state of DTR/RTS when the monitor
is idle:

- ``released`` -- Both lines deasserted (default)
- ``boot-hold`` -- Boot line held asserted
- ``reset-hold`` -- Reset line held asserted

Read Modes
**********

The ``--read-mode`` option controls how serial data is buffered before
display:

- ``auto`` (default) -- Lines are flushed on newline characters. Partial
  lines (such as shell prompts like ``uart:~$``) are flushed after an idle
  timeout controlled by ``--idle-flush-ms`` (default: 60 ms).
- ``line`` -- Only flushes on newline characters. Partial lines are never
  shown until a newline arrives.
- ``raw`` -- Unbuffered. Data is displayed immediately as received.

.. _west-monitor-config:

Configuration Files
*******************

The monitor supports layered YAML configuration loaded from multiple sources
with the following priority (lowest to highest):

1. Code defaults (built into the monitor)
2. Board ``monitor.yaml`` (from ``BOARD_DIR``)
3. Project ``.monitor.yaml`` or ``monitor.yaml`` (from
   ``APPLICATION_SOURCE_DIR`` or current directory)
4. Explicit ``--config`` file
5. CLI arguments

Values from YAML files are only applied when the user has not explicitly
overridden them via CLI flags.

Board Configuration
===================

Board maintainers can place a ``monitor.yaml`` in the board directory. The
monitor discovers it automatically via ``BOARD_DIR`` from the build's
CMakeCache.

Example :file:`boards/vendor/my_board/monitor.yaml`:

.. code-block:: yaml

   serial:
     baud: 115200

   reset:
     mode: run
     on_connect: true
     line_map: rts-boot,dtr-reset

Project Configuration
=====================

Users can place ``.monitor.yaml`` or ``monitor.yaml`` in the application
source directory or the current working directory:

.. code-block:: yaml

   serial:
     baud: 921600

   display:
     timestamp: true

Explicit Configuration
======================

A specific config file can be loaded with ``--config``::

  west monitor --config path/to/monitor.yaml

YAML Schema
============

.. code-block:: yaml

   serial:
     baud: 115200           # Baud rate
     rtscts: false          # Hardware flow control
     read_mode: auto        # auto | line | raw
     idle_flush_ms: 60      # Idle flush timeout (auto mode)

   reset:
     mode: none             # none | run | boot | dtr-pulse | rts-pulse
     on_connect: false      # Reset after initial connection
     on_reconnect: false    # Reset after auto-reconnect
     delay_ms: 25           # Delay between line toggles
     hold_ms: 100           # Hold time for asserted lines
     line_map: dtr-boot,rts-reset
     invert_reset: true     # Invert reset line polarity
     invert_boot: true      # Invert boot line polarity
     idle_lines: released   # released | boot-hold | reset-hold

   display:
     timestamp: false       # Prepend HH:MM:SS.mmm timestamps

Output Options
**************

Timestamps
==========

Prepend ``HH:MM:SS.mmm`` timestamps to each line::

  west monitor --timestamp

Logging
=======

Save all monitor output to a file::

  west monitor --logfile output.log

.. _west-monitor-options:

Command-Line Options
********************

.. list-table::
   :widths: 20 10 40
   :header-rows: 1

   * - Option
     - Default
     - Description
   * - ``-p``, ``--port``
     - auto-detect
     - Serial port path
   * - ``-b``, ``--baud``
     - ``115200``
     - Baud rate
   * - ``--rtscts`` / ``--no-rtscts``
     - off
     - Hardware flow control
   * - ``--read-mode``
     - ``auto``
     - ``auto``, ``line``, or ``raw``
   * - ``--idle-flush-ms``
     - ``60``
     - Idle flush timeout in ms (auto mode)
   * - ``--reset``
     - ``none``
     - ``none``, ``run``, ``boot``, ``dtr-pulse``, ``rts-pulse``
   * - ``--reset-on-connect`` / ``--no-reset-on-connect``
     - off
     - Reset after initial connection
   * - ``--reset-on-reconnect`` / ``--no-reset-on-reconnect``
     - off
     - Reset after auto-reconnect
   * - ``--reset-delay-ms``
     - ``25``
     - Delay between line toggles (ms)
   * - ``--reset-hold-ms``
     - ``100``
     - Hold time for asserted lines (ms)
   * - ``--line-map``
     - ``dtr-boot,rts-reset``
     - DTR/RTS line mapping
   * - ``--invert-reset`` / ``--no-invert-reset``
     - inverted
     - Reset line polarity
   * - ``--invert-boot`` / ``--no-invert-boot``
     - inverted
     - Boot line polarity
   * - ``--idle-lines``
     - ``released``
     - ``released``, ``boot-hold``, ``reset-hold``
   * - ``--reset-key``
     - ``r``
     - Ctrl-<key> to trigger reset
   * - ``-t``, ``--timestamp`` / ``--no-timestamp``
     - off
     - Prepend timestamps to output
   * - ``-l``, ``--logfile``
     - none
     - Log output to file
   * - ``--config``
     - none
     - Path to monitor.yaml config file
   * - ``--exit-char``
     - ``]``
     - Ctrl-<char> to exit

Dependencies
************

- pyserial (``pip install pyserial``)
- PyYAML (``pip install pyyaml``)

Cross-Platform Support
**********************

The monitor supports Linux, macOS, and Windows with proper terminal handling
for each platform.
