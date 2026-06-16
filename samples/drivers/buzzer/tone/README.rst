.. zephyr:code-sample:: buzzer-tone
   :name: Buzzer Tone
   :relevant-api: buzzer_interface

   Play a short beep and a few tones through the buzzer device aliased as ``buzzer0``.

Overview
********

This application plays a short attention beep followed by three tones through the buzzer device
referenced by the ``buzzer0`` :ref:`devicetree <dt-guide>` alias, repeating once every three
seconds.

The sample exercises the high-level :ref:`buzzer API <buzzer_api>` through :c:func:`buzzer_beep`
and :c:func:`buzzer_tone`, which drive both backends:

* :dtcompatible:`pwm-buzzer` for passive piezo buzzers wired to a PWM channel.
* :dtcompatible:`gpio-buzzer` for active buzzers controlled by a single GPIO line. Pitch
  information is ignored by this backend.

Requirements
************

The target board must expose a buzzer device through the ``buzzer0`` devicetree alias. Boards that
do not have a buzzer wired in their DTS can provide an :file:`<board>.overlay` file under
:file:`boards/`.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/buzzer/tone
   :goals: build flash
