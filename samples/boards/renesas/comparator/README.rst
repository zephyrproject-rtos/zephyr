.. zephyr:code-sample:: renesas_comparator
   :name: Renesas comparator

   Monitor the output of comparator.

Overview
********

A sample that help to monitor the output of comparator.

Hardware Setup
**************

* The IVREF is connected to the internal output of DA0 (DAC module).
  By default, DA0 is set to ~1.75V (half of the full-scale voltage).

* For IVCOMP, the user can connect the IVCMP3 input to either:

   * Case 1: IVCMP3 with SW0 (button on board, on ek_ra8m1 is P600).
   * Case 2: IVCMP3 with external voltage for comparison.

Building and Running
********************

This application can be built and executed on RA boards as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/renesas/comparator
   :board: ek_ra8m1
   :goals: build flash
   :compact:

The comparator's output controls LED0:

   * If the comparator output is HIGH, LED0 turns on.
   * If the comparator output is LOW, LED0 turns off.

   Example case:

   * Case 1: IVCMP3 connected to SW0 (Pull up)

      * SW0 not pressed --> Comparator output HIGH --> LED0 on
      * SW0 pressed --> Comparator output LOW --> LED0 off

   * Case 2: IVCMP3 connected to another reference voltage

      * IVCMP3 HIGH --> Comparator output HIGH --> LED0 on
      * IVCMP3 LOW --> Comparator output LOW --> LED0 off
