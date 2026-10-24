PM Companion Timer Test
#######################

Overview
********

Tests low-power state entry and measures sleep duration. The test automatically
adapts to available timing sources:

- **Systick**: Uses ARM SysTick timer when no companion timer is
  available. Timing granularity is coarser and doesn't run in certain
  low power modes.

- **Companion Timer**: Uses dedicated low-power counter for precise
  microsecond-level timing that works in all sleep states.

Device Tree
***********

Optionally configure a companion timer in the board overlay:

.. code-block:: devicetree

   chosen {
       zephyr,system-timer-companion = &counterg0; /* Optional */
       zephyr,test-pm-state = &stdby1;
   };

If not defined, the test automatically uses systick.
