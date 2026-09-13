.. zephyr:board:: mimxrt2660_evk

.. _mimxrt2660_evk:

MIMXRT2660-EVK
##############

Overview
********

The MIMXRT2660-EVK is an evaluation board for the NXP i.MX RT266x family, built around
the MIMXRT2663 crossover MCU. The i.MX RT266x is a subsystem-partitioned design: a single
Arm Cortex-M85 compute core sits alongside HSP, MAIN, WAKE, AUDIO, MEDIA, COMM, SYSCON and
VBAT domains, each with its own clock and power controls.

Hardware
********

- MIMXRT2663 crossover MCU:

  - Arm Cortex-M85 at up to 1 GHz, with Helium, TrustZone, MPU and L1 caches
  - 256 KB ITCM, 256 KB DTCM, and three 256 KB banks of on-chip RAM
  - Neutron N256 neural processing unit and a display/graphics subsystem
  - Two xSPI interfaces (one 8-bit, one 16-bit), 8 UART, 4 I2C, 2 I3C, 6 SPI
  - Two Gbps Ethernet with TSN, two 10BASE-T1S, three CAN-FD
  - Security enclave (EdgeLock) and a last-level cache

- Board:

  - MIMXRT2663CVVAA, Industrial grade, 289BGA 0.8 mm, 1 GHz
  - 64 MB Winbond W25H512NWEAM serial NOR flash on XSPI0
  - 32 MB Winbond W958D6NMYA Xccela PSRAM on XSPI1
  - eMMC, SD card slot, Ethernet, USB
  - On-board MCU-Link debug probe

For more information about the i.MX RT266x SoC and this board, see the
`i.MX RT2660 Reference Manual`_ and the `i.MX RT2660 Data Sheet`_.

Memory Layout
=============

This board executes in place from external flash and uses external RAM:

- ``zephyr,flash`` is the 64 MB XSPI0 NOR flash; code runs in place from it.
- ``zephyr,sram`` is the 32 MB XSPI1 PSRAM; ``.data``, ``.bss`` and the heap
  live there.
- ITCM and DTCM stay available to the SoC. The clock bring-up needs them:
  reprogramming the PLLs requires parking both XSPI controllers, so the code
  that does it and the stack it runs on cannot be in either external memory.

The boot ROM initializes both memories from the boot header before this image
runs -- the flash configuration block for the NOR and the external memory
configuration data for the PSRAM.

.. note::
   This SoC's boot ROM uses a container-based (AHAB) image format and does not
   read an image vector table. The build emits all three blocks the ROM reads --
   the flash configuration block at offset 0x400, the external-memory
   configuration data at 0xA00 and the AHAB container at 0x1000 -- and places
   the image itself at 0xC000, so ``zephyr.bin`` written to the boot flash runs
   from a cold reset with no post-build step. Signing is not required on an
   evaluation board: the out-of-fab life cycle ignores authentication errors, so
   the container carries an empty signature block.

.. note::
   The flash is used for execution only: this target carries no flash driver and
   no partitions, so the flash API, MCUboot and settings-on-flash are not
   available. Enabling the flash driver requires portability work in Zephyr's
   shared XSPI drivers, which use ``xspi_config_t`` fields this SoC does not
   implement.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and I/Os
====================

+-----------+-----------------+------------------------------------+
| Function  | Signal          | Pad                                |
+===========+=================+====================================+
| Console   | LPUART0 TXD     | PIO3_30                            |
+-----------+-----------------+------------------------------------+
| Console   | LPUART0 RXD     | PIO3_31                            |
+-----------+-----------------+------------------------------------+
| User LED  | HSP GPIO0 26    | PIO2_26 (D16, active low)          |
+-----------+-----------------+------------------------------------+
| User SW   | WAKE GPIO0 0    | PIO1_0 (WAKE_SS_BTN)               |
+-----------+-----------------+------------------------------------+
| XSPI0     | SCLK0, SS0_N,   | PIO6_0, PIO6_1, PIO6_2..PIO6_5     |
|           | DATA0..DATA3    | (quad; the flash is not wired      |
|           |                 | octal on this board)               |
+-----------+-----------------+------------------------------------+

The debug console runs at 115200 8N1 on the MCU-Link virtual COM port.

System Clock
============

The board's boot clock configuration enters the Over Drive Run operating point through
the power controller, so the Cortex-M85 runs at 1 GHz. Because that transition also has
to hand the PLL reference over from the on-chip oscillator to the board crystal, part of
it executes from RAM with the caches off and both xSPI controllers disabled -- which is
why that code, and the stack it runs on, live in ITCM and DTCM rather than in
either external memory.

The system timer counts an external 24 MHz reference derived from the board crystal
(the compute subsystem's SysTick clock root), not the core clock, so tick timing is
crystal-accurate rather than dependent on the core frequency.

Peripheral clock roots are described in devicetree as ``nxp,imx-ccm-rev3-root`` nodes,
and there are two ways to program them, matching where a root sits in the tree.

Upstream and shared roots -- the CGU distribution roots and any root without a single
consumer, such as ``main_rootclk`` or the ``peri3_rootclk`` root that feeds every
PERI3-sourced peripheral -- carry their mux and divider as properties on the root node
itself. An application overlay can retarget one of these without editing board C code,
which moves every leaf that draws from it at once. For example, to re-source the shared
PERI3 root:

.. code-block:: devicetree

   &peri3_rootclk {
           clock-mux = <IMX_CCM_MUX_PERI3_SYSPLL_DIVOUT2>;
           clock-div = <1>;
   };

Leaf peripheral roots -- the per-instance functional-clock roots owned by a single
device, such as each LPUART -- instead carry their default mux and divider
inline on the consuming peripheral node, as a second ``clocks`` entry alongside the gate.
This gives every such device a board default without a separate overlay. The console
LPUART0 uses PERI3 divided by 5 (80 MHz):

.. code-block:: devicetree

   &lpuart0 {
           clock-names = "gate", "source";
           clocks = <&ccm IMX_CCM_CLK(IMX_CCM_LPCG_MAIN_HSP_LPUART0,
                                      IMX_CCM_ROOT_MAIN_LPUART0_FCLK)>,
                    <&ccm IMX_CCM_ROOT_CFG(IMX_CCM_ROOT_MAIN_LPUART0_FCLK,
                                           IMX_CCM_MUX_LPUART0_PERI3, 5, 1)>;
   };

Only the LPUART driver reads that second entry today. A peripheral whose driver does
not yet apply it keeps its root at the reset value, so adding the entry alone changes
nothing for LPSPI, LPI2C or the others.

.. note::
   Only the leaf peripheral roots are safe to retarget this way. The analog sources, the
   CPU and bus roots and the PLL-distribution roots are devicetree nodes too -- the SoC
   clock bring-up reads them rather than hardcoding values -- but they are parked, powered
   down and re-hopped by the reference hand-over described above, so a property edit alone
   does not make a different setting work. The two xSPI functional roots are declared on
   ``xspi0``/``xspi1`` instead of as controller children, and are additionally written by
   hand inside that window.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

This board is configured by default to use the on-board
:ref:`mcu-link-cmsis-onboard-debug-probe`.

Using J-Link
------------

LinkServer is not supported for this board yet; attach an external J-Link
probe to the SWD header, then build and flash with:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt2660_evk/mimxrt2663/cm85
   :goals: flash
   :flash-args: -r jlink

Open a serial terminal on the MCU-Link virtual COM port at 115200 8N1, reset the board,
and the sample prints:

.. code-block:: console

   *** Booting Zephyr OS build v4.5.0 ***
   Hello World! mimxrt2660_evk/mimxrt2663/cm85

References
**********

.. target-notes::

.. note::
   The links below point to the NXP website's landing pages rather than a
   specific document, because the i.MX RT2660 reference manual and data
   sheet are not yet published. These links will be updated to the actual
   documents once NXP publishes them.

.. _i.MX RT2660 Reference Manual:
   https://www.nxp.com/

.. _i.MX RT2660 Data Sheet:
   https://www.nxp.com/
