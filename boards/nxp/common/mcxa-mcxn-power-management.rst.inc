.. SPDX-FileCopyrightText: Copyright 2026 NXP
.. SPDX-License-Identifier: Apache-2.0

Power Management
****************

Enable :kconfig:option:`CONFIG_PM` to use :ref:`pm-system`. The SoC PM hooks
implement the following state mapping:

.. list-table::
   :header-rows: 1

   * - Zephyr state
     - NXP power mode
     - Availability
   * - ``runtime-idle``
     - Sleep
     - Available with ``CONFIG_PM``
   * - ``suspend-to-idle``
     - Deep Sleep
     - Available with ``CONFIG_PM``
   * - ``standby``
     - Power Down
     - Available with ``CONFIG_PM``
   * - ``suspend-to-ram``
     - Deep Power Down
     - Disabled by default in the SoC devicetree

The PM policy chooses among enabled states according to the expected idle
duration, latency constraints, and :ref:`power-state locks
<pm-policy-power-states>`. The devicetree residency and exit-latency values are
configuration defaults, not measured guarantees for every application.

Wakeup and Timekeeping
======================

When SysTick is the kernel timer, it stops while the core clock is gated.
The board selects LPTMR0 through ``zephyr,system-timer-companion`` to keep time
and provide timed wakeups. Keep the companion timer enabled and use a clock
source that operates in the intended power mode. Other kernel timer
configurations must also provide timekeeping and wakeups across that mode.

The ``wakeup-button`` alias describes a button's ``wakeup-ctrls`` connection to
the WUU. The alias alone does not arm a wakeup: enable the source through the
:ref:`WUC API <wuc_api>` before entering low power. The button and WUU pin are
listed below.

Deep Power Down
===============

Suspend-to-RAM is opt-in. Before enabling the ``deeppowerdown`` devicetree node
in an application overlay, place the entire retained working set, including
stacks, kernel objects, and saved CPU context, in RAM that survives this mode
and arm a wakeup source. Enabling the node with ``CONFIG_PM`` also enables
:kconfig:option:`CONFIG_PM_S2RAM` when supported by the architecture.
The :zephyr:code-sample:`nxp_mcx_s2ram` sample provides timer and button wakeup
configurations and board-specific memory layouts.

Deep Power Down resets the CORE domain and its peripheral register blocks.
Applications relying on peripheral recovery must enable
:kconfig:option:`CONFIG_PM_DEVICE` and the required :ref:`power-domain support
<pm-power-domain>`, and use drivers that restore their hardware after power
loss. :ref:`System-managed device PM <pm-device-system-pm>` and
:ref:`runtime device PM <pm-device-runtime-pm>` are separate mechanisms;
``CONFIG_PM`` alone does not ensure peripheral recovery.

With :kconfig:option:`CONFIG_POWEROFF`, system off also enters Deep Power Down,
but wakeup starts through the reset path rather than resuming the suspended
application. See :zephyr:code-sample:`nxp_mcx_system_off`.

Debugging and Validation
========================

Use :kconfig:option:`CONFIG_SOC_FAMILY_MCXA_DEBUG_IN_LOW_POWER` or
:kconfig:option:`CONFIG_SOC_FAMILY_MCXN_DEBUG_IN_LOW_POWER`, as appropriate, to
keep debug logic enabled in modes that retain the CORE domain, at the cost of
higher low-power current. Deep Power Down loses the debug connection regardless
of this option.

The :zephyr_file:`tests/subsys/pm/power_mgmt_soc` test covers the targets in its
platform allow list. Its default configuration does not enable Deep Power
Down. Validate the optional state with the S2RAM sample, then verify wakeup
and peripheral operation with the application's own configuration. A sample
or test target listing is not evidence of measured power consumption.

Board-specific Requirements
===========================
