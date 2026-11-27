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
initialization, and shared locking. The child has no separate register region.
For example:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/wuc/wuc_nxp_s32_wkpu.h>

   &wkpu {
       status = "okay";

       wkpu_wuc: wakeup-controller {
           compatible = "nxp,s32-wkpu-wuc";
           #wakeup-ctrl-cells = <1>;
       };
   };

   / {
       zephyr,user {
           wakeup-ctrls = <&wkpu_wuc NXP_S32_WKPU_SOURCE(1, NXP_S32_WKPU_EDGE_RISING)>;
       };
   };

Source IDs encode a hardware source number and edge selection, not an external
pin index or CPU interrupt number. Use ``NXP_S32_WKPU_SOURCE(source, edge)`` with
``NXP_S32_WKPU_EDGE_RISING``, ``NXP_S32_WKPU_EDGE_FALLING``, or
``NXP_S32_WKPU_EDGE_BOTH``. Internal sources 0-3 support rising edges only.
Invalid IDs return ``-EINVAL``.

Each physical source must have one logical WUC owner. Repeated enable and
disable calls are idempotent state changes, not reference-counted requests.
Clients sharing a source must coordinate enable, disable, and triggered-state
clearing; an encoded ID with a different edge does not create a separate source.
This ownership requirement does not prevent an IRQ consumer from using the
same source alongside its WUC owner.

Enabling wakeup configures the source's edge and opens WRER without enabling
IRER or clearing status. A wake-only consumer needs only the WUC API.
Disabling wakeup preserves edges while an interrupt remains enabled, and
disabling an interrupt preserves edges while wakeup remains armed.

WKPU has a single edge detector for each source. IRQ and wake requests on the
same source must use identical edges; their edges are never silently merged or
overwritten. A conflicting enable or reconfiguration returns ``-EBUSY`` without
changing the armed source. Disable the current request before choosing another
edge. The WKPU interrupt enable API returns an error for conflicts, and the
GPIO interrupt configuration path propagates it to the caller.

Triggered status combines sampled live flags, events acknowledged by the
interrupt driver, and an optional early-boot snapshot. It remains available
until explicitly cleared. A positive result reports a source event, including
an ordinary IRQ event, not proof that the source caused a low-power wakeup or
was enabled for wakeup when the event occurred.

A successful clear acknowledges the WUC-visible event record; it does not
guarantee that WISR or the originating peripheral's status is deasserted.
Clearing one source leaves unrelated flags and ordinary interrupts intact.
While IRER is enabled, WUC never acknowledges WISR.
It only suppresses reporting of an already observed flag, leaving acknowledgment
to the interrupt consumer. An event arriving after the clear's status read
remains available to both consumers.

Internal sources can reflect status owned by another peripheral. Their flags
must also be cleared at that peripheral; WUC does not clear an RTC alarm or
other peripheral condition. Software cannot reconstruct an event that was
cleared before either the WKPU parent or WUC sampled it.

The parent provides a private early-boot snapshot intake for SoC startup code.
It must run after BSS initialization, before WKPU device initialization or
concurrent access, and before clearing or resetting the captured hardware
status. It only saves caller-supplied bits and does not acknowledge hardware.
No application-facing SoC wake-status getter is required. SoC startup wiring
and complete low-power entry/resume integration are separate from this backend.

The register-model tests exercise both banks, W1C isolation, shared interrupt
access, and snapshot retention with 32- and 64-source configurations. The RTC
fixture on FRDM-MCXE31B and FRDM-MCXE32B tests runtime event reporting without
external wiring; it does not establish Standby wakeup or external upper-bank
pin behavior.

API Reference
*************

.. doxygengroup:: wuc_interface
