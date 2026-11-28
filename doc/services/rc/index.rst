.. _rc:

Remote Control (IR)
###################

Overview
********

The remote control (rc) subsystem receives and transmits consumer infrared
remote control signals. Drivers report raw pulse and space timings. The core
decodes them with protocol decoders, tracks key state and delivers decoded
events to the application.

.. graphviz::
   :align: center
   :caption: Receive and transmit paths of an rc device.

   digraph rc {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8", fontname="sans"];
     edge [fontname="sans", fontsize=10];

     drv   [label="rc driver"];
     ring  [label="raw ring\n(per device)"];
     dec   [label="decoders\n(rc_rx work queue)"];
     key   [label="key state"];
     app   [label="event iodev\ncallbacks\nrc_receive_sync()"];
     raw   [label="raw iodev"];
     tx    [label="encoders\n(rc tx work queue)"];
     txapp [label="tx iodev\nrc_send_sync()\nrc_send_raw_sync()"];

     drv -> ring [label="raw words"];
     ring -> dec;
     ring -> raw;
     dec -> key [label="scancodes"];
     key -> app [label="rc_event"];
     txapp -> tx;
     tx -> drv [label="raw words"];
   }

Concepts
********

Raw words
=========

A raw word is a ``uint32_t``. Bits 0 to 23 hold a duration in microseconds.
:c:macro:`RC_RAW_PULSE` marks a pulse (carrier on); a word without it is a
space. :c:macro:`RC_RAW_TIMEOUT` marks the end of a frame and
:c:macro:`RC_RAW_OVERFLOW` marks lost words. Drivers store words with
:c:func:`rc_raw_store` or report edges with :c:func:`rc_raw_store_edge`. Each
device has a ring of :kconfig:option:`CONFIG_RC_RAW_BUFFER_SIZE` words. On
overflow the core inserts an overflow word and resets all decoders of the
device.

Protocols
=========

Protocol ids are the ``RC_PROTO_*`` values in
:zephyr_file:`include/zephyr/dt-bindings/rc/rc-proto.h`. A decoder is defined
with :c:macro:`RC_PROTOCOL_DEFINE` and handles a range of ids, for example the
NEC decoder handles ``RC_PROTO_NEC``, ``RC_PROTO_NECX`` and
``RC_PROTO_NEC32``. Each device keeps a bitmap of enabled decoders, changed
with :c:func:`rc_protocol_enable` and :c:func:`rc_protocol_disable`. Enabling
one id enables the decoder with all ids of its range.

Key state
=========

A decoded frame starts a key press. A repeated frame of the same key, or a
repeat code, produces an event with :c:macro:`RC_EVENT_FLAG_REPEAT`. When no
frame of the key arrives within the receiver timeout plus the protocol repeat
period, the core produces an event with :c:macro:`RC_EVENT_FLAG_RELEASE`.

Receiver timeout
================

A space longer than the receiver timeout ends a frame. The timeout comes from
the ``rx-timeout-us`` devicetree property, or from
:c:func:`rc_set_rx_timeout`. Otherwise it is the longest minimum timeout of the
enabled decoders plus 10 ms, or 125 ms if none needs one, clamped to the driver
range. When the driver cannot generate the timeout itself, the core does.

Receive
*******

* :c:macro:`RC_DT_EVENT_IODEV` defines an RTIO iodev that completes with one
  :c:struct:`rc_event` per read.
* :c:macro:`RC_DT_RAW_IODEV` defines an RTIO iodev that completes with up to
  :kconfig:option:`CONFIG_RC_RAW_CHUNK_WORDS` raw words per read.
* :c:func:`rc_stream` submits a multishot read on either iodev, using the
  mempool of the RTIO context.
* :c:func:`rc_receive_sync` waits for the next event of a device.
* :c:macro:`RC_CALLBACK_DEFINE` registers a callback for the events of a
  device, or of all devices with ``NULL``. Callbacks run on the rc receive work
  queue.

Receive is started with :c:func:`rc_rx_enable`.

Transmit
********

Transmit needs a device whose driver supports it; ``CONFIG_RC_TX`` is selected
by such drivers. Transmissions run on a separate work queue.

* :c:func:`rc_send_sync` encodes and sends a scancode described by
  :c:struct:`rc_tx_req`. ``repeats`` adds frames after the first; the protocol's
  repeat encoder is used when it has one, the frames are spaced by the protocol
  transmit period.
* :c:func:`rc_send_raw_sync` sends raw words with the carrier set by the
  ``carrier-hz`` and ``duty-cycle-percent`` properties or
  :c:func:`rc_set_tx_carrier`.
* :c:macro:`RC_DT_TX_IODEV` defines an RTIO iodev for both forms;
  :c:func:`rc_sqe_prep_send` prepares a scancode write.

All transmit calls are supervisor only. The transmit iodev must not be granted
to user threads.

Userspace
*********

:c:func:`rc_get_caps`, :c:func:`rc_set_rx_timeout`,
:c:func:`rc_protocol_enable`, :c:func:`rc_protocol_disable` and
:c:func:`rc_protocol_is_enabled` are system calls. User threads read events and
raw words through the event and raw iodevs granted to them with
:c:func:`k_object_access_grant`. :c:func:`rc_receive_sync`, the callbacks and
all transmit calls are supervisor only.

Devicetree
**********

All rc devices include :zephyr_file:`dts/bindings/rc/rc-device.yaml`:

* ``protocols``: ``RC_PROTO_*`` values enabled at init. If absent, all built
  decoders are enabled.
* ``rx-timeout-us``: fixed receiver timeout.
* ``carrier-hz``: raw transmit carrier, default 38000.
* ``duty-cycle-percent``: raw transmit duty cycle, default 50.

A demodulating receiver module on a GPIO:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/gpio/gpio.h>
   #include <zephyr/dt-bindings/rc/rc-proto.h>

   ir_rx: ir-receiver {
           compatible = "gpio-ir-receiver";
           gpios = <&gpio0 5 GPIO_ACTIVE_LOW>;
           protocols = <RC_PROTO_NEC>;
   };

Relation to existing drivers
****************************

The ``vishay,vs1838b`` and ``bflb,irx`` input drivers decode frames in the
driver and report scancodes as input events. The rc subsystem keeps decoding in
the core, so drivers only report timings and decoders are shared between
drivers.

API Reference
*************

.. doxygengroup:: rc_interface

.. doxygengroup:: rc_protocol_interface

.. doxygengroup:: rc_driver_interface

.. doxygengroup:: rc_loopback
