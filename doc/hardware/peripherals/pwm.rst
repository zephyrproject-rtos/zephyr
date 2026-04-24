.. _pwm_api:

Pulse Width Modulation (PWM)
############################

Overview
********

The PWM interface provides access to `pulse width modulation`_ controllers.

A PWM signal is a periodic digital signal. Each period contains an active pulse and an inactive
interval. Applications control the signal by setting the period, pulse width, and polarity.

PWM is commonly used to dim LEDs, drive servos, generate simple clocks, or control other devices
that encode a value in the duty cycle of a digital waveform. From a Zephyr point of view, a PWM
device controls one or more channels. The application selects a channel and sets the timing values
used to generate the output waveform.

Key concepts
************

To generate a PWM output, an application specifies:

#. Which PWM controller and channel are used,
#. How long each PWM period is, and
#. How much of each period is active.

These pieces are usually described in Devicetree and combined in :c:struct:`pwm_dt_spec`.

**PWM controller and channel** (:c:member:`pwm_dt_spec.dev`, :c:member:`pwm_dt_spec.channel`)
  A PWM controller is the hardware peripheral that generates the waveform; a channel is an
  individual output of that controller. A controller typically exposes several channels that share
  a common timebase, but each produce their own waveform with its own pulse width and polarity.

  Each channel maps to a specific output pin, which is selected through board-level
  :ref:`pin control <pinctrl-guide>` configuration.

**Period and pulse width** (:c:member:`pwm_dt_spec.period`)
  The period is the total length of one PWM cycle. The pulse width is the active part of that
  period. The duty cycle is the ratio between the two values:

  .. code-block:: none

     duty cycle = pulse width / period

  For example, a 20 ms period with a 10 ms pulse width produces a 50% duty cycle. Passing a pulse
  width of 0 drives the output inactive, while a pulse width equal to the period drives the output
  active.

**Polarity** (:c:member:`pwm_dt_spec.flags`)
  Polarity controls which electrical level is considered active. :c:macro:`PWM_POLARITY_NORMAL`
  represents an active-high pulse, and :c:macro:`PWM_POLARITY_INVERTED` represents an active-low
  pulse. Inverted polarity is useful, for example, when driving a common-anode LED that turns on
  when the line is driven low.

**Time units**
  :c:func:`pwm_set`, :c:func:`pwm_set_dt`, and :c:func:`pwm_set_pulse_dt` use nanoseconds for period
  and pulse width values.

  The :c:macro:`PWM_NSEC`, :c:macro:`PWM_USEC`, :c:macro:`PWM_MSEC`, :c:macro:`PWM_SEC`,
  :c:macro:`PWM_HZ`, and :c:macro:`PWM_KHZ` helper macros can be used to express these values in
  more convenient units.

Typical application flow
************************

Typical use of the PWM API to generate an output signal is:

#. Get a :c:struct:`pwm_dt_spec` from Devicetree, usually with :c:macro:`PWM_DT_SPEC_GET`.
#. Check that the PWM device is ready using :c:func:`pwm_is_ready_dt`.
#. Set the period and pulse width using :c:func:`pwm_set_dt`.
#. If the period is fixed in Devicetree, update only the pulse width with
   :c:func:`pwm_set_pulse_dt`.

Devicetree
**********

PWM consumers describe their PWM signals with a ``pwms`` property. The exact channel values are
controller-specific, but the period cell is always expressed in nanoseconds and can use the PWM
period helper macros.

The ``pwms`` property is a list, so a consumer may reference multiple PWM channels (potentially on
different controllers). When a consumer uses more than one channel, a matching ``pwm-names``
property can be added so that individual entries can be selected by name with
:c:macro:`PWM_DT_SPEC_GET_BY_NAME`.

.. code-block:: devicetree
   :caption: PWM consumer node with a 50 Hz period

   #include <zephyr/dt-bindings/pwm/pwm.h>

   / {
       pwm_leds {
           compatible = "pwm-leds";

           pwm_led0: pwm_led_0 {
               pwms = <&pwm0 0 PWM_HZ(50) PWM_POLARITY_NORMAL>;
           };
       };

       aliases {
           pwm-led0 = &pwm_led0;
       };
   };

In this example, the consumer uses channel 0 of ``pwm0`` with a 50 Hz (20 ms) period and normal
polarity.

.. note::
   For SoC-integrated PWM controllers, the controller node (``pwm0`` in the example above) usually
   also needs board-specific :ref:`pin control <pinctrl-guide>` configuration so the selected
   channel is routed to the intended pin. This is separate from the consumer's ``pwms`` property.

Setting a PWM Output
********************

A common pattern is to initialize a :c:struct:`pwm_dt_spec` statically and then use
:c:func:`pwm_set_dt` to configure the output:

.. code-block:: c
   :caption: Setting a PWM output to a 50% duty cycle

   #include <zephyr/device.h>
   #include <zephyr/drivers/pwm.h>
   #include <zephyr/kernel.h>

   static const struct pwm_dt_spec pwm = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

   int main(void)
   {
       int ret;

       if (!pwm_is_ready_dt(&pwm)) {
           return 0;
       }

       ret = pwm_set_dt(&pwm, PWM_MSEC(10), PWM_MSEC(5));
       if (ret < 0) {
           return ret;
       }

       return 0;
   }

The call to :c:func:`pwm_set_dt` uses the controller, channel, and polarity from Devicetree. The
period argument overrides the period stored in :c:struct:`pwm_dt_spec`: in the example above, the
output runs at 100 Hz (10 ms) instead of the 50 Hz declared in Devicetree, while still keeping a
50 % duty cycle. If the application wants to keep the Devicetree period, it can call
:c:func:`pwm_set_pulse_dt` instead.

Capture
*******

Some PWM controllers can measure an incoming PWM signal. Capture support is enabled with
:kconfig:option:`CONFIG_PWM_CAPTURE`.

The blocking helper :c:func:`pwm_capture_cycles` captures the selected parts of a signal and returns
the measured values in controller clock cycles. Unlike :c:func:`pwm_configure_capture`, which
installs a callback and is therefore kernel-only, :c:func:`pwm_capture_cycles` is also callable
from user space.

The example below assumes a ``struct pwm_dt_spec pwm`` initialized as in the previous section:

.. code-block:: c
   :caption: Capturing period and pulse width once

   uint32_t period;
   uint32_t pulse;
   int ret;

   ret = pwm_capture_cycles(pwm.dev, pwm.channel,
                            PWM_CAPTURE_TYPE_BOTH | PWM_CAPTURE_MODE_SINGLE,
                            &period, &pulse, K_MSEC(500));
   if (ret < 0) {
       return ret;
   }

The :c:macro:`PWM_CAPTURE_TYPE_PERIOD`, :c:macro:`PWM_CAPTURE_TYPE_PULSE`, and
:c:macro:`PWM_CAPTURE_TYPE_BOTH` flags select which values are captured. The
:c:macro:`PWM_CAPTURE_MODE_SINGLE` and :c:macro:`PWM_CAPTURE_MODE_CONTINUOUS` flags select whether
the capture runs once or continuously. :c:macro:`PWM_CAPTURE_MODE_SINGLE` is the default (its value
is ``0``); it is included in the example above for explicitness.

Events
******

Some PWM controllers can notify the application about events such as period completion, faults, or
compare and capture matches. Event support is enabled with :kconfig:option:`CONFIG_PWM_EVENT`.

Applications initialize a :c:struct:`pwm_event_callback` with :c:func:`pwm_init_event_callback` and
register it with :c:func:`pwm_add_event_callback`.

The example below assumes a ``struct pwm_dt_spec pwm`` initialized as in the previous section:

.. code-block:: c
   :caption: Registering a PWM period event callback

   static struct pwm_event_callback callback;

   static void pwm_event_handler(const struct device *dev,
                                 struct pwm_event_callback *cb,
                                 uint32_t channel,
                                 pwm_events_t events)
   {
       /* Handle the event */
   }

   int ret;

   pwm_init_event_callback(&callback, pwm_event_handler, pwm.channel,
                           PWM_EVENT_TYPE_PERIOD);
   ret = pwm_add_event_callback(pwm.dev, &callback);

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_PWM`
* :kconfig:option:`CONFIG_PWM_CAPTURE`
* :kconfig:option:`CONFIG_PWM_EVENT`
* :kconfig:option:`CONFIG_PWM_SHELL`

API Reference
*************

.. doxygengroup:: pwm_interface

References
**********

.. target-notes::

.. _`pulse width modulation`: https://en.wikipedia.org/wiki/Pulse-width_modulation
