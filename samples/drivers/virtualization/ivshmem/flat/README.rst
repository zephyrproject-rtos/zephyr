.. zephyr:code-sample:: ivshmem-flat
   :name: IVSHMEM flat ping-pong
   :relevant-api: ivshmem

   Exchange shared-memory messages and DoorBell notifications between two Cortex-M VMs.

Overview
********

Two instances of the same Zephyr image exchange and verify 100 ping-pong messages through
the ivshmem driver API. A fresh ivshmem server assigns peer IDs 0 and 1. Each peer owns one
shared-memory slot containing readiness, sequence, payload, checksum, and completion fields.
Peer 0 initiates each exchange; peer 1 validates the message and echoes it. Both peers verify
every received message before reporting success. No application code accesses device registers.

The sample uses vector 0 and requires ``ivshmem-server -n 1``. The single QEMU output IRQ
does not identify the incoming vector. INTMASK and INTSTATUS are reserved and do not control
or acknowledge notifications. The driver raises a poll signal directly from the ISR.

Readiness is checked in shared memory before ringing a peer, since its eventfds may not yet
exist. Startup has a 30-second timeout and each receive has a 5-second timeout. The application
resets the signal before checking shared memory, then waits only if the expected state is
absent. Sequence numbers remain authoritative when notifications coalesce.

Requirements
************

* A Linux host with Git, a C build toolchain, Ninja, Python, and GLib development headers.
* A Zephyr west workspace and ARM toolchain.
* QEMU v10.0.0 with the local machine wiring below, and its ``ivshmem-server`` executable.

QEMU 10.0 already contains the ivshmem-flat device model. It is not user-creatable with
``-device``; machine code must realize the device, map both regions, and connect its IRQ.
The ordinary ``lm3s6965evb`` machine does not provide these connections. The patch below
creates the device only when a chardev named ``ivshmem`` is supplied.

The patch and ``app.overlay`` agree on:

.. list-table:: Local machine connections
   :header-rows: 1

   * - Resource
     - Address or IRQ
     - Size
   * - MMR
     - 0x40010000
     - 0x10 bytes
   * - Shared memory
     - 0x60000000
     - 1 MiB
   * - NVIC input
     - IRQ 25
     - One pulse notification line

These are local machine connections, not standard board hardware addresses. The sample
provides the node through its overlay; no change to the base board DTS is needed.
Changing the machine wiring requires matching overlay changes. The QEMU ``shmem-size``,
overlay region size, and server size must agree.

Build and run
*************

Save the complete diff in the next section as ``ivshmem-flat-wiring.patch``. From the west
workspace root, build QEMU outside the Zephyr repository:

.. code-block:: console

   git clone --depth 1 --branch v10.0.0 https://github.com/qemu/qemu.git qemu-10.0.0
   git -C qemu-10.0.0 apply ../ivshmem-flat-wiring.patch
   mkdir qemu-10.0.0-build
   cd qemu-10.0.0-build
   ../qemu-10.0.0/configure --target-list=arm-softmmu --disable-docs --disable-pixman
   ninja qemu-system-arm contrib/ivshmem-server/ivshmem-server
   cd ../zephyr

QEMU configuration may download additional build dependencies. Build and run the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/virtualization/ivshmem/flat
   :board: qemu_cortex_m3
   :build-dir: build/ivshmem-flat
   :goals: build

.. code-block:: console

   python samples/drivers/virtualization/ivshmem/flat/run.py \
     --qemu ../qemu-10.0.0-build/qemu-system-arm \
     --server ../qemu-10.0.0-build/contrib/ivshmem-server/ivshmem-server \
     --kernel build/ivshmem-flat/zephyr/zephyr.elf

The runner creates a private temporary directory for the server socket and shared-memory
backing file. It starts a fresh single-vector server, waits for peer 0 to start, then starts
peer 1. The overall timeout defaults to 90 seconds. It stops all child processes on success,
failure, timeout, or interruption. An existing server or backing file must not be reused:
the sample expects IDs 0 and 1 and zero-initialized shared memory.

The runner prints the persistent log directory containing ``server.log``, ``vm0.log``, and
``vm1.log``. Use ``--log-dir`` to specify a new directory; existing directories are rejected.
A successful run requires both messages:

.. code-block:: text

   PASS: peer 0 completed 100 rounds
   PASS: peer 1 completed 100 rounds

The runner exits with status zero only after both messages are present. ``west build -t run``
alone does not start the required server and second VM. Twister builds this sample but does
not run its two-VM environment automatically.

Driver tests
************

The driver tests run on the ordinary QEMU Cortex-M3 machine without the wiring patch:

.. code-block:: console

   west twister -p qemu_cortex_m3 -T tests/drivers/virtualization/ivshmem/flat -i

Their overlay reserves SRAM for fake MMR and shared-memory regions and assigns unused
NVIC inputs. Tests pend those inputs to test the actual driver ISR. This covers the
driver API and notification lifecycle; it does not replace the two-VM eventfd validation.

Machine wiring patch
********************

Apply this diff to QEMU v10.0.0. It modifies only machine wiring, leaving the existing
ivshmem-flat device model unchanged.

.. code-block:: diff

   diff --git a/hw/arm/stellaris.c b/hw/arm/stellaris.c
   index 3361111..e2d5553 100644
   --- a/hw/arm/stellaris.c
   +++ b/hw/arm/stellaris.c
   @@ -8,13 +8,17 @@
     */

    #include "qemu/osdep.h"
   +#include CONFIG_DEVICES
    #include "qemu/bitops.h"
   +#include "qemu/units.h"
    #include "qapi/error.h"
    #include "hw/core/split-irq.h"
    #include "hw/sysbus.h"
    #include "hw/sd/sd.h"
    #include "hw/ssi/ssi.h"
    #include "hw/arm/boot.h"
   +#include "chardev/char.h"
   +#include "hw/misc/ivshmem-flat.h"
    #include "qemu/timer.h"
    #include "hw/i2c/i2c.h"
    #include "net/net.h"
   @@ -1141,6 +1145,23 @@ static void stellaris_init(MachineState *ms, stellaris_board_info *board)
        sysbus_mmio_map(SYS_BUS_DEVICE(ssys_dev), 0, 0x400fe000);
        sysbus_connect_irq(SYS_BUS_DEVICE(ssys_dev), 0, qdev_get_gpio_in(nvic, 28));

   +#ifdef CONFIG_IVSHMEM_FLAT_DEVICE
   +    /* Local ivshmem-flat wiring for Zephyr's application overlay. */
   +    Chardev *ivshmem_chr = qemu_chr_find("ivshmem");
   +
   +    if (ivshmem_chr) {
   +        DeviceState *ivshmem = qdev_new(TYPE_IVSHMEM_FLAT);
   +        SysBusDevice *sbd = SYS_BUS_DEVICE(ivshmem);
   +
   +        qdev_prop_set_chr(ivshmem, "chardev", ivshmem_chr);
   +        qdev_prop_set_uint32(ivshmem, "shmem-size", 1 * MiB);
   +        sysbus_realize_and_unref(sbd, &error_fatal);
   +        sysbus_mmio_map(sbd, 0, 0x40010000);
   +        sysbus_mmio_map(sbd, 1, 0x60000000);
   +        sysbus_connect_irq(sbd, 0, qdev_get_gpio_in(nvic, 25));
   +    }
   +#endif
   +
        if (DEV_CAP(1, ADC)) {
            dev = sysbus_create_varargs(TYPE_STELLARIS_ADC, 0x40038000,
                                        qdev_get_gpio_in(nvic, 14),
