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

API Reference
*************

.. doxygengroup:: wuc_interface
