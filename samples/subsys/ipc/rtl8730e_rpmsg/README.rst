.. zephyr:code-sample:: rtl8730e_rpmsg
   :name: RTL8730E CA32 <-> KM4/KM0 OpenAMP rpmsg

   Minimal OpenAMP rpmsg (ipc_service static-vrings) link test between the
   RTL8730E CA32 (host) and the KM4 and KM0 (remote) cores.

Overview
********

This sample exercises the ``zephyr,ipc-openamp-static-vrings`` backend over the
Realtek AmebaSmart IPC doorbell (``realtek,rtl8730e-ipc`` MBOX driver). The same
firmware is built independently for each core; the host/remote role and the set
of rpmsg instances present are selected entirely in the device tree.

The CA32 is the host for two independent instances:

* ``ipc0``: CA32 <-> KM4
* ``ipc1``: CA32 <-> KM0

Each remote board defines only ``ipc0`` (its single link to the host).

Shared memory
*************

* **KM4 link**: 64 KB at ``0x60700000`` (base of ``KM4_DRAM_HEAP_EXT``),
  non-secure PSRAM addressed identically by CA32 and KM4. On the CA32 side the
  application RAM (``dram0``) is shrunk so it no longer overlaps this window.
* **KM0 link**: 16 KB at ``0x2301B000`` in KM0 SRAM. KM0 lives in the LP domain
  and **cannot address PSRAM** (``0x60xxxxxx``), so its vrings must sit in the
  ``0x23xxxxxx`` SRAM window that both CA32 and KM0 can reach. KM0's ``sram0`` is
  shrunk to ``0x23002020..0x2301B000`` to reserve this region.

Both regions are mapped **non-cacheable** for coherency: via
``ATTR_MPU_RAM_NOCACHE`` on the KM4/KM0 (MPU) side and via dedicated
Normal-Non-cacheable MMU entries (``REGION_KM4_VRING`` / ``REGION_KM0_VRING`` in
the SoC's ``mmu_regions.c``) on the CA32 (MMU) side. ``OPENAMP_WITH_DCACHE`` is
therefore not needed.

Doorbell bits
*************

The ``realtek,rtl8730e-ipc`` mbox cell is the **absolute IPC register bit**
(0..31), because the group shift is direction-dependent in hardware:

===================  ==============  ====================
Direction            TX bit          RX-full bit
===================  ==============  ====================
CA32 -> KM4          5               (KM4 sees 21)
KM4 -> CA32          2               (CA32 sees 18)
CA32 -> KM0          12              (KM0 sees 20)
KM0 -> CA32          4               (CA32 sees 28)
===================  ==============  ====================

On the CA32 the single IPCAP block and its one interrupt are shared by both
instances (RX callbacks on disjoint bits 18 and 28).

Building
********

Host (CA32):

.. code-block:: console

   west build -p always -b rtl8730e_evb/rtl8730e/ca32 \
       samples/subsys/ipc/rtl8730e_rpmsg

Remote (KM4):

.. code-block:: console

   west build -p always -b rtl8730e_evb/rtl8730e/km4 \
       samples/subsys/ipc/rtl8730e_rpmsg

Remote (KM0):

.. code-block:: console

   west build -p always -b rtl8730e_evb/rtl8730e/km0 \
       samples/subsys/ipc/rtl8730e_rpmsg
