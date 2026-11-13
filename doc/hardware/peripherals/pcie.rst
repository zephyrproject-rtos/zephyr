.. _pcie_api:

Peripheral Component Interconnect express Bus (PCIe)
####################################################

Overview
********

Device-model host controllers have independent configuration spaces. A bus/device/function
tuple (BDF) is unique only within one controller. With
:kconfig:option:`CONFIG_PCIE_CONTROLLER`, the legacy BDF-only interfaces select
``zephyr,pcie-controller``. This includes :c:func:`pcie_scan`, even with
``PCIE_SCAN_RECURSIVE``. Architecture backends such as x86 retain their existing configuration
access without a controller device.

Controller-qualified endpoints
==============================

Use :c:struct:`pcie_endpoint` to identify an endpoint on a particular controller. The
``pcie_endpoint_*`` interfaces require a non-NULL controller, return zero on success and a negative
errno on failure, and never fall back to the chosen controller. A missing capability is a successful
query with an output word index of zero. Missing memory BARs return ``-ENOENT``; unsupported
operations and untranslatable BARs return ``-ENOTSUP``. BAR sizing disables address decoding and
restores it only after the original BAR words have been restored. A failed BAR restoration leaves
decoding disabled and may require a device reset before retrying.

For example, discover a device on a controller during boot:

.. code-block:: c

   static bool find_device(const struct pcie_endpoint *ep, pcie_id_t id, void *arg)
   {
       struct pcie_endpoint *result = arg;

       if (id == PCIE_ID(0x1234, 0x5678)) {
           *result = *ep;
           return false;
       }
       return true;
   }

   static int discover(const struct device *ctrl, struct pcie_endpoint *ep)
   {
       const struct pcie_scan_ctrl_opt opt = {
           .cb = find_device,
           .cb_data = ep,
           .flags = PCIE_SCAN_RECURSIVE,
       };
       int ret;

       *ep = (struct pcie_endpoint){ .bdf = PCIE_BDF_NONE };
       ret = pcie_scan_ctrl(ctrl, &opt);
       if (ret != 0) {
           return ret;
       }
       return ep->ctrl != NULL ? 0 : -ENODEV;
   }

:c:func:`pcie_scan_ctrl` scans only the supplied controller. Recursion follows that controller's
bridges; callbacks and masking callbacks receive the complete endpoint identity. The caller must
initialize the controller first. Scan callbacks must copy the temporary endpoint value to retain it.

Configuration callbacks without status returns cannot distinguish transport failures from data.
An all-ones read is therefore returned unchanged with success. Controllers may implement optional
``conf_read_status`` and ``conf_write_status`` callbacks; endpoint operations prefer these and
propagate their negative errno values. Legacy configuration functions retain the original callbacks.

Static declarations
===================

``DEVICE_PCIE_ENDPOINT_DECLARE()`` records the expected controller before enumeration. Direct
children of a controller use their devicetree parent. Other placements, including devices below
bridges or ``simple-bus`` nodes, must specify a ``pcie-controller`` phandle. An unset or invalid
controller fails binding validation; it never means the chosen controller. Matching compares the
controller before vendor/device ID and class/revision. :c:func:`pcie_dev_endpoint` constructs an
endpoint value from a bound declaration without storing a second identity.
The ``pcie-controller`` property selects an owner; it does not select a physical slot among
devices with identical IDs and class on that controller. Those declarations bind in scan order.

Drivers that still use BDF-only host operations declare their context with
``DEVICE_PCIE_DECLARE()`` or ``DEVICE_PCIE_INST_DECLARE()``. These macros reject
non-chosen controller placements at build time. Moving such a driver to another controller requires
converting its configuration, BAR and interrupt paths to endpoint operations; the host must not bind
it there and then let its BDF-only accesses reach the chosen controller. Endpoint-aware drivers
opt in with ``DEVICE_PCIE_ENDPOINT_DECLARE()`` or ``DEVICE_PCIE_ENDPOINT_INST_DECLARE()``.

Generic static binding runs at ``PRE_KERNEL_2`` on device-model platforms. Each controller is
validated and scanned independently, starting at bus 0 and following reachable bridges into their
subordinate buses. Other root-bus layouts require caller-directed discovery with
:c:func:`pcie_scan_ctrl`. An unready controller leaves its declarations unbound while
healthy controllers continue enumeration; initialization returns the first error. A scan error
retains declarations already matched on that controller. A ``PRE_KERNEL_1``
consumer must discover its endpoint through :c:func:`pcie_scan_ctrl` after the host is initialized,
rather than relying on these later binding results. In particular, an RP1 interrupt controller must
derive its host from the devicetree hierarchy, initialize after its PCIe host and MSI proxy, and
be ready before peripheral drivers enable interrupts. Its early capability and BAR accesses must use
that same host. RP1 interrupt-controller and MSI-proxy drivers are separate from the host interface.

MSI ownership and allocation
============================

The endpoint MSI interfaces use the supplied controller for capability discovery, MSI-X table
translation, vector setup, programming and connection. Vectors retain their controller and BDF;
connection and enable reject a mismatched identity. MSI-X and 64-bit MSI preserve both target
address words. A target above 4 GiB cannot be enabled on a device limited to 32-bit MSI.

Endpoint vector allocation requires ``msi_device_setup_errno``. A controller with only the legacy
8-bit-count callback is rejected with ``-ENOTSUP`` before capability discovery or table mapping.
The callback must allocate every requested vector or release every IRQ and hardware mapping acquired
by the request. It returns the original errno and an output count of zero on failure. The host
uses an independent virtual mapping for each MSI-X request and releases it after failure, leaving
any earlier successful allocation mapped. Insufficient capacity returns ``-ENOSPC``.
MSI requests must use a power-of-two count supported by the endpoint; MSI-X counts need not be
powers of two. Ordinary MSI vectors must share one target address and use consecutive message data,
with the first value aligned to the vector count. Enable validates that layout before programming.
MSI-X entries may use independent messages.

Only one allocation may succeed per endpoint until reboot. The backend reserves the controller/BDF
identity, rejects subsequent requests with ``-EALREADY``, and releases the reservation on failure so
the caller can retry. The caller must keep the successfully allocated array intact and must not
pass that storage, including aliases or subarrays, as the output of another allocation request.
An ``-EALREADY`` result preserves an earlier allocation held in separate storage. A backend
that reports partial success or an error with a nonzero allocated count violates the ownership
contract and causes a kernel panic; the host cannot safely reclaim unknown backend resources.

The current ITS interface has no IRQ-free or DeviceID/EventID revoke operation. ECAM therefore
leaves the transactional callback unimplemented and endpoint MSI allocation returns ``-ENOTSUP``.
Controllers sharing an ITS are unsupported by this endpoint MSI path: BDF-derived DeviceIDs can
collide, and a software controller pointer does not disambiguate those hardware IDs. Supporting
that topology requires a controller-specific requester-ID to DeviceID mapping and ITS resource
reclamation. Legacy MSI entry points remain available with their existing count and Boolean results.

Connection installs the interrupt handler and then enables its IRQ. Initialize the handler's state
before connecting. After successful allocation, vectors and their mappings belong to the caller
until reboot. An enable failure keeps them allocated but may leave the device partially programmed
or already enabled. Keep handlers and their state valid while retrying or stopping boot after such
a failure. INTx routing, runtime link recovery, hotplug and runtime vector release require separate
interfaces and are outside this boot-time topology model.

API Reference
*************

.. doxygengroup:: pcie_host_interface
