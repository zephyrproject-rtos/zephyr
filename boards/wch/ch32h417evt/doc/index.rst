.. zephyr:board:: ch32h417evt

Overview
********

The WCH_ CH32H417EVT is an evaluation board for the CH32H417 SoC.
The SoC contains two asymmetric RISC-V cores:

* QingKe V3F boot core, hart 0
* QingKe V5F application core, hart 1

The CH32H417 is supported as an AMP system using Zephyr hardware model v2
CPU cluster targets. It is not an SMP system, and CONFIG_SMP must not be
enabled. Zephyr images for the V3F and V5F cores are built separately.

On cold reset, only the V3F core starts executing. The V5F core does not start
by itself; it must be woken by V3F firmware using the CH32H417 PFIC wakeup
mechanism. The recommended Zephyr application target is the V5F target:
ch32h417evt/ch32h417/v5f.

Hardware
********

The current board support configures the V5F core to run at 480 MHz. The HCLK
and USART clock domain are configured at 120 MHz. The validated console is
USART8 TX on PB4, alternate function 11, at 115200 baud.

The V5F Zephyr image is linked at flash address 0x08010000 and uses the
V5F DTCM RAM region at 0x200c0400. The V3F waker image is linked at flash
address 0x00000000. The CH32H417 aliases the main flash bank into the low
0x00000000 window, so the V5F image at 0x08010000 is the same physical flash
as offset 0x10000 used by the OpenOCD programming commands below.

Supported Features
^^^^^^^^^^^^^^^^^^

.. zephyr:board-supported-hw::

Connections and IOs
^^^^^^^^^^^^^^^^^^^

Console
-------

The V5F console uses USART8:

.. list-table:: V5F console
   :header-rows: 1

   * - Signal
     - Pin
     - Alternate function
     - Settings
   * - USART8 TX
     - PB4
     - AF11
     - 115200 8N1

The V3F waker target does not configure a console and should not print
anything. This avoids contention with the V5F console.

LED
---

The board DTS provides led0 on PB1 for samples such as blinky.

.. list-table:: LED connection
   :header-rows: 1

   * - LED alias
     - Pin
     - Polarity
   * - led0
     - PB1
     - Active high

CPU Cluster Targets
*******************

Applications must be built for one of the CH32H417 CPU cluster targets:

.. list-table:: Board targets
   :header-rows: 1

   * - Target
     - Purpose
     - Boot hart
     - Image address
   * - ch32h417evt/ch32h417/v3f
     - Minimal V3F Zephyr waker image
     - 0
     - 0x00000000
   * - ch32h417evt/ch32h417/v5f
     - Main Zephyr application image
     - 1
     - 0x08010000

The V3F image wakes the V5F image from soc_early_init_hook() and then idles.
The V3F target is not intended to own application peripherals in this initial
board support.

Programming and Debugging
*************************

A one-step ``west flash`` runner is not provided for this board yet. A
bootable setup requires a merged V3F and V5F image, programmed in a single
OpenOCD operation.

Building
^^^^^^^^

Build a V5F hello_world application:

.. code-block:: console

   $ west build -p always -b ch32h417evt/ch32h417/v5f samples/hello_world

Build a V5F blinky application:

.. code-block:: console

   $ west build -p always -b ch32h417evt/ch32h417/v5f samples/basic/blinky

Build the V3F waker image. The V3F target wakes the V5F core from
``soc_early_init_hook()`` (gated by :kconfig:option:`CONFIG_SOC_CH32H417_BOOT_V5F`,
enabled by default), so the standard minimal sample is sufficient:

.. code-block:: console

   $ west build -p always -b ch32h417evt/ch32h417/v3f samples/basic/minimal

Flashing
^^^^^^^^

A complete bootable setup requires both images in flash, addressed through the
low aliased flash window that the OpenOCD flow programs:

* V3F waker image at flash offset 0x00000000
* V5F application image at flash offset 0x00010000 (the V5F image is linked at
  the aliased address 0x08010000, which is the same physical flash)

Do not assume that two independent OpenOCD program operations will leave both
images valid. On the currently tested WCH OpenOCD flow, programming one image
can erase the other image. A reliable method is to pad the generated V3F
binary up to 0x10000, append the generated V5F binary, and program the merged
binary once.

For example, after building the V3F waker in build-v3f and the V5F application
in build-v5f:

.. code-block:: console

   $ python3 - <<'PY'
   from pathlib import Path

   waker = Path('build-v3f/zephyr/zephyr.bin').read_bytes()
   app = Path('build-v5f/zephyr/zephyr.bin').read_bytes()
   merged = bytearray([0xff]) * 0x10000
   merged[:len(waker)] = waker
   Path('ch32h417_dual.bin').write_bytes(merged + app)
   PY
   $ openocd -f boards/wch/ch32h417evt/support/openocd.cfg -c init -c halt -c "program ch32h417_dual.bin 0x00000000 verify" -c reset -c shutdown

Use verify when programming. Some WCH OpenOCD versions can terminate after a
successful verify while restoring the flash algorithm work area; if the log
shows that verification completed successfully, the image has already been
written.

Running
^^^^^^^

Open a serial terminal on the V5F console port at 115200 baud, then reset the
board:

.. code-block:: console

   $ screen /dev/ttyACM0 115200

The hello_world sample on the V5F target should print:

.. code-block:: console

   Hello World! ch32h417evt/ch32h417/v5f

The blinky sample on the V5F target should toggle PB1 with the sample's normal
timing.

Debugging
^^^^^^^^^

The board can be debugged with a WCH-LinkE compatible OpenOCD configuration.
When debugging the V5F target, remember that V5F is not active immediately
after cold reset. The V3F waker must run first and wake V5F at 0x08010000.

References
**********

.. target-notes::

.. _WCH: http://www.wch-ic.com
