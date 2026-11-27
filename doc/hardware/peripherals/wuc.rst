.. _wuc_api:

Wakeup Controller (WUC)
#######################

Overview
********

The Wakeup Controller (WUC) API provides a common interface for enabling and
managing wakeup sources that can bring the system out of low-power states.
WUC devices are typically described in Devicetree and referenced by clients
using :c:struct:`wuc_dt_spec`.

Devicetree Configuration
************************

Wakeup controllers are referenced from client nodes with the ``wakeup-ctrls``
property. The property encodes a phandle to the WUC device and an identifier
for the wakeup source.

Example Devicetree fragment:

.. code-block:: devicetree

   wuc0: wakeup-controller@40000000 {
       compatible = "nxp,mcx-wuc";
       reg = <0x40000000 0x1000>;
       #wakeup-ctrl-cells = <1>;
   };

   button0: button@0 {
       wakeup-ctrls = <&wuc0 10>;
   };

Basic Operation
***************

Applications typically obtain a :c:struct:`wuc_dt_spec` using
:c:macro:`WUC_DT_SPEC_GET` and then enable or disable the wakeup source as
needed.

.. code-block:: c
   :caption: Enable a wakeup source from Devicetree

   #define BUTTON0_NODE DT_NODELABEL(button0)

   static const struct wuc_dt_spec button_wuc =
       WUC_DT_SPEC_GET(BUTTON0_NODE);

   if (!device_is_ready(button_wuc.dev)) {
       return -ENODEV;
   }

   return wuc_enable_wakeup_source_dt(&button_wuc);

If a driver supports it, applications can check and clear a wakeup source's
triggered state. When not implemented, the APIs return ``-ENOSYS``.

.. code-block:: c
   :caption: Check and clear a wakeup source

   int ret;

   ret = wuc_check_wakeup_source_triggered_dt(&button_wuc);
   if (ret > 0) {
       (void)wuc_clear_wakeup_source_triggered_dt(&button_wuc);
   }

NXP S32 WKPU
************

The :dtcompatible:`nxp,s32-wkpu-wuc` backend is a child of the existing
:dtcompatible:`nxp,s32-wkpu` interrupt controller. The parent owns the registers,
initialization, and locking. Use ``NXP_S32_WKPU_SOURCE(source, edge)`` from
:zephyr_file:`include/zephyr/dt-bindings/wuc/wuc_nxp_s32_wkpu.h` for source IDs.
WUC configures wakeup edges without requiring IRQ setup.

Each physical source has one logical WUC owner; enable/disable calls are
idempotent, not reference-counted. IRQ and wake requests can share a source
only with matching edges. Conflicting WUC or GPIO configuration returns
``-EBUSY`` without changing the existing configuration. The legacy WKPU IRQ
enable function retains its ``void`` signature and logs rejected requests.
Disabling one request preserves the edges needed by the other.

Triggered status is source-event history, including ordinary IRQ events,
not proof of a system wakeup. Successful clear does not guarantee hardware
status deassertion: pending IRQ flags remain for the IRQ consumer, and
internal sources may require clearing at the originating peripheral.

API Reference
*************

.. doxygengroup:: wuc_interface
