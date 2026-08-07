.. _buzzer_api:

Buzzer
######

The buzzer subsystem exposes an API to drive buzzer hardware uniformly, regardless of whether the
underlying part is a passive piezo driven by a PWM channel or an active buzzer controlled by a
single GPIO line.

Basic Operation
***************

Applications obtain a buzzer device through devicetree and drive it through the functions in
:zephyr_file:`include/zephyr/drivers/buzzer.h`:

Buzzer API calls do not wait for the requested tone duration. Durations describe how long the
hardware should keep emitting a tone, not how long the calling thread should sleep; use
:c:macro:`BUZZER_DURATION_FOREVER` to play until an explicit stop.

- :c:func:`buzzer_tone` plays a tone at a specific frequency for a specific duration. The hardware
  keeps emitting the tone for the requested duration and the driver silences it automatically.
- :c:func:`buzzer_beep` plays the buzzer's natural operating frequency, encoded by the board file
  in the period cell of the ``pwms`` DT property (typically the piezo's mechanical resonance,
  which is the loudest tone the part can produce). On active buzzers the call is equivalent to
  driving the GPIO line on, since the hardware oscillator determines the actual pitch.
- :c:func:`buzzer_set_volume` adjusts the perceived loudness. Zero silences immediately; non-zero
  values are stored and applied on the next tone. Active buzzers, lacking analog volume control,
  map zero to silent and any non-zero value to their single audible level.
- :c:func:`buzzer_stop` cancels any in-progress tone immediately.

Backends
********

Two devicetree-discoverable backends are provided:

- :dtcompatible:`pwm-buzzer` for passive piezo buzzers driven from a PWM channel. The PWM channel
  period sets the audio frequency and the duty cycle sets the perceived volume; the driver derives
  both from the application's tone and volume requests.
- :dtcompatible:`gpio-buzzer` for active buzzers driven through a single GPIO. The driver toggles
  the line on for any non-zero frequency and off for :c:macro:`BUZZER_FREQ_REST`.

API Reference
*************

.. doxygengroup:: buzzer_interface
