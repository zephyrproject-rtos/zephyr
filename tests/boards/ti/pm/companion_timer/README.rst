PM Companion Timer Test
#######################

Overview
********

This test verifies that the device enters a specified low-power state and
accurately measures the duration using the system timer companion. It runs
3 sleep/wake cycles and validates both state entry and timing accuracy using
a low-frequency clock that remains active during sleep.

Device Tree Configuration
*************************

The test requires two settings in the board overlay:

.. code-block:: devicetree

   / {
       chosen {
           zephyr,system-timer-companion = &counterg0;  /* Companion counter */
           zephyr,test-pm-state = &stdby1;              /* Target power state */
       };
   };

   &timg0 {
       status = "okay";
       clocks = <&ckm MSPM0_CLOCK_LFCLK>;
   };

   &counterg0 {
       status = "okay";
   };
