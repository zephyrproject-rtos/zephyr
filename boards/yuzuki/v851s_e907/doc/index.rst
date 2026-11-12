.. zephyr:board:: v851s_e907

Overview
********

The YuzukiPI-Lizard is an Allwinner V851S board with a Cortex-A7 application
core and a XuanTie E907 RISC-V co-processor running at 600 MHz. This board
target builds Zephyr for the E907 core only; the Cortex-A7 runs Linux.

The E907 has no XIP flash: the whole image executes in place from a 4 MiB DRAM
window at ``0x43c00000``, which the A7-side Linux remoteproc driver fills by
loading :file:`/lib/firmware/zephyr.elf`.

The board does not use UART3 as a console. UART3 is routed to the expansion
header and UART0 belongs to the A7 Linux console, so the Zephyr target uses a
RAM console by default (:kconfig:option:`CONFIG_RAM_CONSOLE` enabled,
:kconfig:option:`CONFIG_UART_CONSOLE` disabled); the A7 can read that buffer
over OpenAMP. For early bring-up, :file:`bringup_uart_console.overlay`
temporarily routes the console to UART3.

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
conflict with Linux. The A7 must therefore:

* mux the UART3 pins (PE12/PE13, mux 7) and enable the UART3 clock;
* enable the MSGBOX clock gate and de-assert its reset;
* reserve the DRAM window and shared-memory regions described below.

The MSGBOX driver only touches the MSGBOX register windows themselves.

Memory layout
*************

The A7 side must reserve the firmware window and the OpenAMP shared memory,
and pass them to the remoteproc driver. The addresses below are the ones this
BSP expects; the ``compatible`` and ``core-name`` values depend on the
remoteproc binding used by the A7 device tree and must be kept in sync with
it.

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
and two vrings. The Zephyr OpenAMP resource table leaves the vring addresses
unset (``FW_RSC_ADDR_ANY``); the A7-side driver fills them in from the
carveouts above. On the E907 the shared window itself is described by the
``zephyr,ipc_shm`` devicetree node. The ``openamp_rsc_table`` sample
announces the endpoints ``rpmsg-client-sample`` and ``rpmsg-tty``, which the
Linux ``rpmsg_char`` driver exposes as ``/dev/rpmsg0`` and ``/dev/rpmsg1``.

Console and UART allocation
***************************

The V851S has four UARTs. On the YuzukiPI-Lizard they are used as follows:

.. list-table::
   :header-rows: 1

   * - UART
     - Base
     - Pins
     - Usage
   * - UART0
     - ``0x02500000``
     - PH9/PH10
     - A7 Linux console (CH343P USB-to-serial), not usable by the E907
   * - UART1
     - ``0x02500400``
     - PE6/PE7
     - Wired to the XR829 ``WL_REG_ON`` / ``WL_WAKE_AP`` pins
   * - UART2
     - ``0x02500800``
     - PD18..PD21
     - XR829 Bluetooth
   * - UART3
     - ``0x02500c00``
     - PE0/PE1 or PE12/PE13
     - Expansion header, available to the E907

UART3 (PE12/PE13, mux 7) is brought out on the 12-pin expansion header. The
E907 cannot write the PIO block, so the A7 must mux the pins. Keep ``&uart3``
disabled in the A7 device tree so Linux does not claim them. The board
devicetree also leaves ``&uart3`` disabled; :file:`bringup_uart_console.overlay`
enables it for a bring-up console.

OpenAMP and RPMsg
*****************

The BSP implements the full remote-side transport: the
``allwinner,sunxi-msgbox-riscv`` mailbox driver (RX FIFO drained in the CLIC
144 ISR, TX writes to the CPUX MSGBOX block), the generic ``ipm_mbox`` IPM
shim, and an OpenAMP ``VIRTIO_DEV_DEVICE`` with a static resource table.

Building with a public GCC toolchain
************************************

The E907 firmware must be built with an upstream GNU toolchain, **not** with
the old XuanTie / Allwinner RISC-V toolchain shipped with some vendor BSPs.
Those are based on much older GCC and binutils releases which do not know all
of the ``xthead*`` extensions, so the assembler can reject instructions that
Zephyr emits for
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

``printk`` output goes to the RAM console by default. To route it to UART3
(115200 8N1, PE12/PE13) during bring-up, pass the overlay by its full path
and switch the console Kconfig options:

.. code-block:: console

   west build -b v851s_e907 -s zephyr/samples/hello_world -d build-uart -- \
     -DDTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/yuzuki/v851s_e907/bringup_uart_console.overlay" \
     -DCONFIG_RAM_CONSOLE=n -DCONFIG_UART_CONSOLE=y

Alternatively, copy :file:`bringup_uart_console.overlay` into the board
directory of the application. The A7 can also read the RAM console buffer
over OpenAMP.
