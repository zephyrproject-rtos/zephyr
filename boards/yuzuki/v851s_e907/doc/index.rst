.. zephyr:board:: v851s_e907

Overview
********

The YuzukiPI-Lizard (Allwinner V851S) contains a Cortex-A7 application core
and a XuanTie E907 RISC-V co-processor running at 600 MHz. This board target
builds Zephyr for the E907 core only.

The E907 firmware is loaded into DDR by the Linux remoteproc driver running on
the Cortex-A7 (:file:`/lib/firmware/zephyr.elf`). The E907 has no XIP flash:
the whole image executes in place from the DRAM window at ``0x43c00000``
(4 MiB).

Because UART3 is dedicated to the serial bridge, the board uses a RAM console
by default (:kconfig:option:`CONFIG_RAM_CONSOLE` enabled,
:kconfig:option:`CONFIG_UART_CONSOLE` disabled); the A7 side can read that
buffer over OpenAMP. For early bring-up, :file:`bringup_uart_console.overlay`
temporarily routes the console to UART3 -- do not use it together with the
bridge application.

Supported Features
******************

.. zephyr:board-supported-hw::

Interrupt controller and timer
******************************

The E907 integrates a CLIC (spec 0.8) and a CLINT; it does **not** have a
PLIC. The CLIC register window is at ``0x30800000`` (20 KiB) and the CLINT at
``0x30000000``. The machine timer uses CLINT ID 7 (mtime ``0x3000bff8``,
mtimecmp ``0x30004000``), with CLIC ID 7, and is described by a
``riscv,machine-timer`` node.

**CLIC ID = GIC ID - 16.** UART3 is GIC SPI 5 (GIC 37) -> CLIC 21; MSGBOX is
GIC 160 -> CLIC 144. The ``ns16550`` Kconfig only enables
:kconfig:option:`CONFIG_SERIAL_SUPPORT_INTERRUPT` when the node has an
``interrupts`` property, so UART3 declares ``interrupt-parent = <&clic>`` and
``interrupts = <21 1>``; MSGBOX uses ``interrupts-extended = <&clic 144 1>``.
:kconfig:option:`CONFIG_NUM_IRQS` defaults to 256 for this SoC.

Peripheral ownership (A7 vs E907)
*********************************

This BSP intentionally declares no pinctrl, GPIO, clock-controller or reset
nodes. On the V851S those blocks are owned by the Cortex-A7 side: the E907
cannot program the PIO pin-mux, and the UART3/MSGBOX clock gates and resets
live in the CCU, which the A7 configures before starting the remote core.
Describing or driving them from the E907 would be non-functional and could
conflict with Linux. Consequently the A7 must:

* mux PE12/PE13 to UART3 (mux 7) and enable the UART3 clock;
* enable the MSGBOX clock gate / de-assert its reset;
* reserve the DRAM window and shared-memory regions below.

The MSGBOX driver only touches the MSGBOX register windows themselves. Refer
to the board's Linux device tree and init scripts for the A7-side setup; the
values there are board-specific and are not part of this Zephyr BSP.

Provenance and licensing
************************

All code in this BSP is original work released under Apache-2.0. Register
addresses and bit definitions were taken from the public Allwinner V851S
datasheet and the XuanTie E907 user manual, and cross-checked against the
vendor Linux driver behaviour. Validation was done on a YuzukiPI-Lizard
running a Tina Linux image derived from the 100ask (百问网) V851S SDK; the
100ask maintainers were asked and confirmed that publishing this Zephyr BSP
is acceptable. No vendor source code is copied here, and the board/SoC names
are used descriptively only.

Memory layout (A7-side Linux device tree)
*****************************************

The following reserved-memory nodes and remoteproc properties must be present
on the Linux side. This is an illustrative snippet: the ``compatible`` and
``core-name`` values depend on the remoteproc binding used by your board's
Linux device tree and must be kept in sync with it; the fixed addresses are
the ones this BSP expects.

.. code-block:: devicetree

   reserved-memory {
       #address-cells = <2>;
       #size-cells = <2>;

       e907_dram: memory@43c00000 {
           reg = <0x0 0x43c00000 0x0 0x00400000>;
           no-map;
       };

       vdev0buffer: vdev0buffer@43000000 {
           compatible = "shared-dma-pool";
           reg = <0x0 0x43000000 0x0 0x40000>;
           no-map;
       };

       vdev0vring0: vdev0vring0@43040000 {
           reg = <0x0 0x43040000 0x0 0x20000>;
           no-map;
       };

       vdev0vring1: vdev0vring1@43060000 {
           reg = <0x0 0x43060000 0x0 0x20000>;
           no-map;
       };
   };

   e907_rproc: remoteproc {
       compatible = "<linux-rproc-compatible>";
       clock-frequency = <600000000>;
       memory-region = <&e907_dram>, <&vdev0buffer>,
                       <&vdev0vring0>, <&vdev0vring1>;
       mboxes = <&msgbox 0>;
       mbox-names = "mbox-chan";
       memory-mappings = <0x43c00000 0x00400000 0x43c00000>;
       core-name = "<remote-core-name>";
       firmware-name = "zephyr.elf";
       status = "okay";
   };

The Zephyr ELF must contain a ``.resource_table`` section declaring one vdev
and two vrings at the fixed addresses above (OpenAMP / RPMsg). The endpoint is
announced as ``rpmsg_chrdev``, so the Linux ``rpmsg_char`` driver exposes
``/dev/rpmsg_ctrl0`` and ``/dev/rpmsg0`` to user space.

Console and UART allocation
***************************

The V851S has four UARTs. On the YuzukiPI-Lizard they are wired as follows
(confirmed from the board schematic ``SCH_Schematic1_2022-11-23.pdf``):

.. list-table::
   :header-rows: 1

   * - UART
     - Base
     - Pins
     - Board usage
   * - UART0
     - ``0x02500000``
     - PH9/PH10
     - CH343P USB-to-serial, A7 Linux console -- do not use
   * - UART1
     - ``0x02500400``
     - PE6/PE7
     - Wired to XR829 ``WL_REG_ON`` / ``WL_WAKE_AP``; not a usable serial port
   * - UART2
     - ``0x02500800``
     - PD18..PD21
     - XR829 Bluetooth
   * - UART3
     - ``0x02500c00``
     - PE0/PE1 or PE12/PE13
     - **Linkr Bee bridge target serial**

UART3 (PE12/PE13, mux 7) is used as the bridge target serial. PE12 and PE13
are brought out on the 12-pin expansion header behind 2K series resistors
(``R38``/``R16`` in the schematic; header order GND / TX0 / RX0 / PH11 /
PH12 / PH13 / PE14 / PE10 / **PE13** / **PE12** / PE11 / GND / 5V). There is
no dedicated UART3 header. UART0 (TX0/RX0, PH9/PH10 via the CH343P) stays the
A7 Linux console. Keep ``&uart3`` disabled in the Linux device tree so that
Linux does not claim the pins.

.. note::

   The E907 cannot write the PIO (pin-mux) block: writes to ``0x02000000``
   from the RISC-V core do not take effect, while writes to the UART3
   controller at ``0x02500c00`` do. On the Tina Linux setup the A7-side
   :file:`S52e907` init script therefore loads :file:`uart3pin.ko` to mux
   PE12/PE13 to UART3 (mux 7) and enable the UART3 clock gate before starting
   the remote core. This must happen on the A7, before the E907 is released
   from reset; it is not part of the Zephyr image.

OpenAMP and RPMsg
*****************

The BSP implements the full remote-side transport: the
``allwinner,sunxi-msgbox-riscv`` mailbox driver (RX FIFO drained in the CLIC
144 ISR, TX writes to the CPUX MSGBOX block), the generic ``ipm_mbox`` IPM
shim, and an OpenAMP ``VIRTIO_DEV_DEVICE`` with a static resource table.
The application announces a ``rpmsg_chrdev`` endpoint and bridges UART3 to
rpmsg in both directions. A ``rpmsg_bench 2000 32`` run on hardware reports
``ok=2000 bad=0`` with a minimum round-trip time of about 193 us.

Building with a public GCC toolchain
************************************

The E907 firmware must be built with an upstream GNU toolchain, **not** with
the old XuanTie / Allwinner RISC-V toolchain shipped with some vendor BSPs
(for example ``riscv64-elf-x86_64-20201104``). Those are based on much older
GCC and binutils releases which do not know all of the ``xthead*`` extensions,
so the assembler can reject instructions that Zephyr emits for
``-march=rv32imac_..._xtheadba_xtheadbb_xtheadbs_xtheadcmo``.

The Zephyr SDK (``ZEPHYR_TOOLCHAIN_VARIANT=zephyr``) bundles a public GCC
(14.3 in SDK 1.0.1) and is sufficient for this board -- it already assembles
the full ISA string above. To use a newer public GCC (for example GCC 15 from
``riscv-collab/riscv-gnu-toolchain``, which also adds ``xtheadvector``), point
Zephyr at it with the ``cross-compile`` variant:

.. code-block:: console

   export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
   export CROSS_COMPILE=/path/to/riscv32-elf-
   west build -b v851s_e907 -s zephyr/samples/hello_world -d build

The toolchain **must** provide an ``rv32imac/ilp32`` multilib. The
``riscv32-elf-*`` nightly bundles are built for ``rv32imafdc/ilp32d`` with
``--disable-multilib``, so linking a soft-float RV32IMAC Zephyr image against
their libgcc fails with ``can't link double-float modules with soft-float
modules``. Use a multilib toolchain (for example the ``riscv64-elf-*`` bundle,
or a custom ``--with-arch=rv32imac --with-abi=ilp32 --enable-multilib`` build).
A newlib-only toolchain also needs :kconfig:option:`CONFIG_MINIMAL_LIBC`
enabled (and :kconfig:option:`CONFIG_PICOLIBC` disabled) passed to
``west build``.

Programming and Debugging
*************************

The board registers no ``west flash`` runner: the E907 has no flash and no
debug adapter of its own. Copy the ELF to :file:`/lib/firmware/zephyr.elf` on
the A7 root filesystem and start the remote core through the Linux remoteproc
framework:

.. code-block:: console

   mount -t debugfs none /sys/kernel/debug
   echo zephyr.elf > /sys/kernel/debug/remoteproc/remoteproc0/firmware
   echo start > /sys/kernel/debug/remoteproc/remoteproc0/state

On Tina Linux the :file:`S52e907` init script performs the pin-mux, firmware
selection and start automatically at boot. ``printk`` output goes to the RAM
console by default; apply :file:`bringup_uart_console.overlay` to route it to
UART3 (115200 8N1, PE12/PE13) during bring-up. The A7 can also read the RAM
console buffer over OpenAMP.
