.. zephyr:code-sample:: smf_calculator
   :name: SMF Calculator
   :relevant-api: smf

   Create a simple desk calculator using the State Machine Framework.

Overview
********

This sample creates a basic desk calculator driven by a state machine written
with the :ref:`State Machine Framework <smf>`.

The 'business logic' of the calculator is based on the statechart given in
Fig 2.18 of *Practical UML Statecharts in C/C++* 2nd Edition by Miro Samek.
This uses a three-layer hierarchical statechart to handle situations such as
ignoring leading zeroes, and disallowing multiple decimal points.

The statechart has been slightly modified to display different output on the
screen in the ``op_entered`` state depending on if a previous result is
available or not.

.. figure:: img/smf_calculator.svg
    :align: center
    :alt: SMF Calculator Statechart
    :figclass: align-center

    Statechart for the SMF Calculator business logic.

The graphical interface uses an LVGL button matrix for input and text label for
output, based on the sample in samples/drivers/display. The state machine updates
the output text label after every call to :c:func:`smf_run_state`.

:kconfig:option:`CONFIG_LV_Z_VDB_SIZE` has been reduced to 14% to allow it to run
on RAM-constrained boards like the :ref:`disco_l475_iot1_board`.

Requirements
************

The GUI should work with any touchscreen display supported by Zephyr. The shield
must be passed to ``west build`` using the ``--shield`` option, e.g.
``--shield adafruit_2_8_tft_touch_v2``

List of Arduino-based touchscreen shields:

- :ref:`adafruit_2_8_tft_touch_v2`
- :ref:`buydisplay_2_8_tft_touch_arduino`
- :ref:`buydisplay_3_5_tft_touch_arduino`

The demo should also work on STM32 Discovery Kits with built-in touchscreens e.g.

- :ref:`stm32f412g_disco_board`
- :ref:`st25dv_mb1283_disco_board`
- :ref:`stm32f7508_dk_board`
- :ref:`stm32f769i_disco_board`

etc. These will not need a shield defined as the touchscreen is built-in.


Building and Running
********************

Below is an example on how to build for a :ref:`disco_l475_iot1_board` board with
a :ref:`adafruit_2_8_tft_touch_v2`.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/smf/smf_calculator
   :board: disco_l475_iot1
   :goals: build
   :shield: adafruit_2_8_tft_touch_v2
   :compact:

For testing purpose without the need of any hardware, the :ref:`native_sim <native_sim>`
board is also supported and can be built as follows;

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/smf/smf_calculator
   :board: native_sim
   :goals: build
   :compact:

CLI control
===========

As well as control through the GUI, the calculator can be controlled through the shell,
demonstrating a state machine can receive inputs from multiple sources.
The ``key <key>`` command sends a keypress to the state machine. Valid keys are
``0`` through ``9`` for numbers, ``.``, ``+``, ``-``, ``*``, ``/`` and ``=`` to
perform the expected function, ``C`` for Cancel, and ``E`` for Cancel Entry.

GUI update speed on the :ref:`disco_l475_iot1_board` with :ref:`adafruit_2_8_tft_touch_v2`
touchscreen is of the order of 0.8s due to button matrices invalidating the entire
matrix area when pressed, rather than just the button that was selected. This could
be sped up by using 18 individual buttons rather than a single matrix, but is sufficient
for this demo.

References
**********

*Practical UML Statecharts in C/C++* 2nd Edition by Miro Samek
https://www.state-machine.com/psicc2
