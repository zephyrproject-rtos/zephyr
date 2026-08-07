.. _pwm_api:

Pulse Width Modulation (PWM)
############################

Overview
********

Pulse Width Modulation (PWM) is a technique for encoding information or
controlling power delivery by varying the duty cycle of a digital signal.
A PWM signal switches between high and low states at a fixed frequency; the
fraction of each period spent in the high state is the *duty cycle*.

Common uses of PWM in embedded systems:

* **LED brightness control** — duty cycle maps directly to perceived brightness
* **DC motor speed control** — average voltage is proportional to duty cycle
* **Servo motor positioning** — pulse width encodes the desired angle
* **Audio tone generation** — varying frequency produces audible tones
* **Power conversion** — buck/boost converters and class-D amplifiers

PWM Signal Parameters
*********************

A PWM signal is described by three values:

* **Period** — the total duration of one on/off cycle, in nanoseconds
* **Pulse width** — the duration of the high (active) portion, in nanoseconds
* **Duty cycle** — the ratio of pulse width to period (expressed as a
  percentage); a 50% duty cycle means the signal is high for half the period

.. code-block:: none

   |<-- pulse -->|
   +--------------+            +-
   |              |            |
   +              +------------+
   |<----------- period -------->|

Zephyr PWM Driver Model
***********************

Each PWM controller exposes one or more independent *channels*. A channel is
identified by a zero-based index and drives a single output pin.

All PWM drivers implement the same API defined in
:file:`include/zephyr/drivers/pwm.h`. The central function is
:c:func:`pwm_set`, which programs the period and pulse width for a channel.
Helper macros convert common time units to nanoseconds:

* :c:macro:`PWM_HZ` — frequency in Hz (e.g. ``PWM_HZ(1000)`` → 1 ms period)
* :c:macro:`PWM_KHZ` — frequency in kHz
* :c:macro:`PWM_USEC` — period or pulse width in microseconds
* :c:macro:`PWM_MSEC` — period or pulse width in milliseconds

Device Tree Configuration
*************************

PWM pins are described in devicetree under the ``pwms`` property. Each entry
specifies the controller phandle, channel number, period in nanoseconds, and
optional polarity flags:

.. code-block:: dts

   / {
       my_node {
           pwms = <&pwm0 0 PWM_MSEC(20) PWM_POLARITY_NORMAL>;
           pwm-names = "servo";
       };
   };

In C, retrieve the spec with the :c:macro:`PWM_DT_SPEC_GET` family of macros:

.. code-block:: c

   static const struct pwm_dt_spec servo =
       PWM_DT_SPEC_GET(DT_NODELABEL(my_node));

Usage Examples
**************

Setting duty cycle using a ``pwm_dt_spec``:

.. code-block:: c

   #include <zephyr/drivers/pwm.h>

   static const struct pwm_dt_spec led_pwm =
       PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

   int set_led_brightness(uint8_t percent)
   {
       if (!device_is_ready(led_pwm.dev)) {
           return -ENODEV;
       }

       /* pulse = period * duty_cycle / 100 */
       uint32_t pulse = led_pwm.period / 100 * percent;

       return pwm_set_dt(&led_pwm, led_pwm.period, pulse);
   }

Setting a servo position (1–2 ms pulse within a 20 ms period):

.. code-block:: c

   #include <zephyr/drivers/pwm.h>

   #define SERVO_PERIOD_NS   PWM_MSEC(20)
   #define SERVO_MIN_PULSE   PWM_USEC(1000)
   #define SERVO_MAX_PULSE   PWM_USEC(2000)

   int set_servo_angle(const struct device *pwm_dev, uint32_t channel,
                       uint8_t angle_deg)
   {
       uint32_t pulse = SERVO_MIN_PULSE +
           (SERVO_MAX_PULSE - SERVO_MIN_PULSE) * angle_deg / 180;

       return pwm_set(pwm_dev, channel, SERVO_PERIOD_NS, pulse,
                      PWM_POLARITY_NORMAL);
   }

PWM Capture
***********

Some PWM controllers support *input capture*, which measures the period and/or
pulse width of an incoming signal. This is useful for decoding PWM signals from
external sources such as RC receivers or sensor outputs.

Enable capture with :c:func:`pwm_configure_capture` and
:c:func:`pwm_enable_capture`:

.. code-block:: c

   void pwm_capture_cb(const struct device *dev, uint32_t channel,
                       uint32_t period_cycles, uint32_t pulse_cycles,
                       int status, void *user_data)
   {
       if (status != 0) {
           return;
       }
       /* convert cycles to nanoseconds using pwm_get_cycles_per_sec() */
   }

   /* configure for single-shot capture of both period and pulse */
   pwm_configure_capture(dev, channel,
                         PWM_CAPTURE_TYPE_BOTH | PWM_CAPTURE_MODE_SINGLE,
                         pwm_capture_cb, NULL);
   pwm_enable_capture(dev, channel);

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_PWM`
* :kconfig:option:`CONFIG_PWM_SHELL`
* :kconfig:option:`CONFIG_PWM_CAPTURE`
* :kconfig:option:`CONFIG_PWM_EVENT`

API Reference
*************

.. doxygengroup:: pwm_interface
