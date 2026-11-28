.. _led_player_api:

LED Player
##########

Overview
********

The LED player plays simple on/off animations on a single LED, driven by the
:ref:`LED driver API <led_api>`. It is intended for status reporting use
cases, where a single LED must convey one of several possible states (for
example idle, warning, error) using a distinct blink pattern for each.

An animation is a fixed sequence of frames. Each frame sets the LED on or
off and holds that state for a given duration before moving to the next
frame. A sequence can be played once, a fixed number of times, or looped
forever.

The subsystem owns the scheduling for all animations started this way: a
single delayed work item is (re)armed for whichever tracked animation has
the closest deadline, so any number of LEDs can be animated concurrently
without each needing its own timer. Only a subset of the LED driver API is
used: the player can only turn a LED fully on or off, it cannot control
brightness or color.

Usage
=====

To use the LED player API, include the header file:

.. code-block:: c

   #include <zephyr/led/led_player.h>

Describe an animation as an array of frames, and declare one
:c:struct:`led_player_channel` per LED that will be animated:

.. code-block:: c

   static const struct led_anim_frame blink_frames[] = {
           { .delay_ms = 500, .state = true },
           { .delay_ms = 500, .state = false },
   };

   static const struct led_anim blink_anim = {
           .frames = blink_frames,
           .count = ARRAY_SIZE(blink_frames),
           .loop = LED_ANIM_INFINITE,
   };

   static struct led_player_channel status_led;

Animations alternating on and off frames can also be defined at file scope with
:c:macro:`DEFINE_LED_ANIM` or :c:macro:`DEFINE_LED_INFINITE_ANIM`. For example,
a 100 ms flash followed by 900 ms off, played 3 times:

.. code-block:: c

   static DEFINE_LED_ANIM(flash_anim, 3, 100, 900);

Initialize the channel once with the LED controller device, the LED index to
drive and the default state of the LED, then play an animation on it:

.. code-block:: c

   led_player_channel_init(&status_led, led_dev, 0, false);
   led_player_play_anim(&status_led, &blink_anim);

The default state is the state of the LED while no animation is playing. It is
applied by :c:func:`led_player_channel_init`, when an animation is stopped and
when an animation completes.

When the LED is known at build time, the channel can be initialized
statically instead of calling :c:func:`led_player_channel_init`:

.. code-block:: c

   static struct led_player_channel status_led =
           LED_PLAYER_CHANNEL_INITIALIZER(DEVICE_DT_GET(DT_NODELABEL(led)), 0, false);

The default state of such a channel is only applied once
:c:func:`led_player_stop_anim` is called on it, or once an animation completes.
Call :c:func:`led_player_stop_anim` at startup to apply it right away.

Stop it when the status it represents clears, which sets the LED to its default
state:

.. code-block:: c

   led_player_stop_anim(&status_led);

To be notified when a non-looping animation finishes on its own, use
:c:func:`led_player_play_anim_with_cb` instead of :c:func:`led_player_play_anim`.
The callback receives the channel whose animation has completed, so a single
callback can serve several channels. It is not invoked if the animation is
stopped with :c:func:`led_player_stop_anim`, replaced by another animation, or
if it loops forever.

.. code-block:: c

   static void flash_done(struct led_player_channel *channel, void *user_data)
   {
           /* channel has completed its animation */
   }

   led_player_play_anim_with_cb(&status_led, &flash_anim, flash_done, NULL);

Use :c:func:`led_player_is_playing` to know whether a channel is playing. A
channel whose animation has completed is not playing anymore, even before its
completion callback is invoked.

Playing an animation on a channel that is already playing stops its current
animation, without invoking its completion callback, and starts the new one
from its first frame.

If the LED driver fails to turn the LED on or off, the error is logged and the
animation of that channel is stopped, without invoking its completion callback.

The player is initialized at the ``POST_KERNEL`` level, see
:kconfig:option:`CONFIG_LED_PLAYER_INIT_PRIORITY`: its API must not be used
before.

Status LED
**********

The status LED, built on top of the LED player, reports statuses on a single
LED. Each status has an identifier, a priority and an animation. Several
statuses can be active at once: the LED plays the animation of the active
status with the highest priority. Between active statuses of equal priority,
the one defined first wins. When the status being reported is cleared, the
animation of the next active status, if any, is played.

A status whose animation is finite clears itself once its animation has
completed. A status whose animation loops forever stays active until it is
cleared.

When no status is active, the LED is set to the default state of the status
LED. The default state is also applied at boot, at the ``APPLICATION`` init
level, once the LED drivers are ready.

Usage
=====

To use the status LED API, include the header file:

.. code-block:: c

   #include <zephyr/led/status_led.h>

Define the statuses in a constant array at file scope, then the status LED
itself, with its LED and its default state. A status LED supports up to 32
statuses, with priorities from 0 to 255:

.. code-block:: c

   enum {
           STATUS_NO_NETWORK,
           STATUS_ERROR,
   };

   static const struct status_led_anim board_statuses[] = {
           /* Blink while there is no network */
           STATUS_LED_BLINK_ANIM(STATUS_NO_NETWORK, 1, 500, 500),
           /* Flash 3 times on error, over the network status */
           STATUS_LED_ANIM(STATUS_ERROR, 2, 3, 100, 100),
   };

   STATUS_LED_DEFINE(board_status, DEVICE_DT_GET(DT_NODELABEL(led)), 0, false,
                     board_statuses);

Set and clear statuses as the device state changes:

.. code-block:: c

   status_led_set(&board_status, STATUS_NO_NETWORK);

   /* Played over STATUS_NO_NETWORK, then clears itself */
   status_led_set(&board_status, STATUS_ERROR);

   /* Once STATUS_ERROR has completed, the LED is set to its default state */
   status_led_clear(&board_status, STATUS_NO_NETWORK);

Setting a status that is already active does not restart its animation.

:c:macro:`STATUS_LED_DEFINE` defines a ``static`` variable: other files can
only use the status LED through a pointer to it.

Configuration
*************

Related configuration options:

* :kconfig:option:`CONFIG_LED_PLAYER`
* :kconfig:option:`CONFIG_LED_PLAYER_INIT_PRIORITY`
* :kconfig:option:`CONFIG_STATUS_LED`

API Reference
*************

.. doxygengroup:: led_player_interface

.. doxygengroup:: status_led_interface
