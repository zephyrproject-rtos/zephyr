.. zephyr:code-sample:: lis2dh_fifo
   :name: LIS2DH FIFO streaming
   :relevant-api: sensor_interface

   Drain LIS2DH hardware FIFO batches using Sensor Async API and RTIO.

Overview
********

This sample starts an RTIO ``sensor_stream()`` for the LIS2DH hardware FIFO.
It requests both ``SENSOR_TRIG_FIFO_WATERMARK`` and ``SENSOR_TRIG_FIFO_FULL``
with ``SENSOR_STREAM_DATA_INCLUDE``. The driver drains each FIFO batch and
returns it through RTIO. The standard Sensor Async API decoder converts the
raw frames into accelerometer samples with timestamps.

The supplied HOLYIOT-25008 overlay sets the FIFO watermark to 16 samples. The
board routes LIS2DH INT1 to GPIO2.0 and uses SPI mode 3, which is selected by
the LIS2DH driver.

The XIAO nRF52840 overlay uses an external LIS2DH12 on I2C: SDA on D4,
SCL on D5, INT1 on D0, address 0x18. It does not use the Sense board's IMU.
Build this configuration with board ``xiao_ble/nrf52840``.

Operation and recovery
**********************

FIFO uses stream mode (FM[7:6] = 10). INCLUDE reads all reported XYZ frames
in one bus transaction. DROP clears FIFO through bypass before completing.
NOP returns an event without reading XYZ; the still-asserted event is masked
until an INCLUDE/DROP request or stream restart. A FULL-only subscription
routes only the full interrupt, so watermark does not drain data early.

The classic software queue and RTIO are alternative consumers. RTIO does not
also fill that queue, but updates the latest-XYZ cache for the synchronous API.
Temperature remains a separate synchronous read, including with
``sensor_sample_fetch()`` during FIFO operation; the decoder supports XYZ only.
DATA_READY, ODR/range changes and power management return ``-EBUSY`` while FIFO
owns INT1. Register operations and lifecycle changes share a mutex.

Cancellation is checked in deferred work even if no sensor interrupt arrives.
After a bus/GPIO cleanup error the device remains faulted: retry
``lis2dh_fifo_stop()`` until it succeeds before starting or reconfiguring.
Persistent hardware faults cannot be repaired by software rollback alone.

Timestamps are host estimates at the FIFO status read, reconstructed using
the configured ODR, not hardware timestamps. Platforms with narrow cycle
counters use 64-bit uptime ticks. The decoder rebases each returned batch and
may return fewer frames than requested to keep nanosecond deltas within
32 bits (for example, five frames per decode call at 1 Hz).

``CONFIG_LIS2DH_FIFO_STATS`` counts only overwrites of the classic software
queue. The lifetime counter saturates at INT32_MAX; start/stop/drop do not
reset it. It cannot measure hardware sample loss or RTIO backlog.

Timing limits and hardware validation
************************************

At 5376 Hz the 32-frame FIFO fills in about 5.95 ms. A 192-byte I2C transfer
requires at least about 4.4 ms at 400 kHz, before scheduling overhead.
Use SPI for evaluating this rate. Increasing watermark reduces free slots:
at 16, nominal headroom is about 2.98 ms; at 32, the next sample arrives
about 186 microseconds later. Select watermark from measured worst-case IRQ
and bus latency, not solely from transfer efficiency.

These estimates follow the FIFO depth and rates in the
`LIS2DH12 datasheet <https://www.st.com/resource/en/datasheet/lis2dh12.pdf>`_.
Native tests validate driver logic and transaction sizes, not physical
signals. Before deployment verify WHO_AM_I = 0x33, SPI mode 3 and CS continuity
for command 0xe8 plus 192 received bytes, watermark/full behavior, all required
ODR/mode combinations, and sustained operation with application/BLE load.
Repeat on I2C and SPI and test recovery from disconnected hardware.

Classic FIFO interface
**********************

Applications that do not use RTIO can enable ``CONFIG_LIS2DH_FIFO`` and use
``lis2dh_fifo_start()``, ``lis2dh_fifo_read()`` and ``lis2dh_fifo_stop()`` from
``<zephyr/drivers/sensor/lis2dh.h>``. Set a nonzero ODR before starting.
Register a ``SENSOR_TRIG_FIFO_WATERMARK`` or ``SENSOR_TRIG_FIFO_FULL`` handler
on ``SENSOR_CHAN_ACCEL_XYZ`` with ``sensor_trigger_set()`` to receive batch
notifications. The trigger structure must remain valid while registered.

For example, a deferred trigger handler can copy the already-drained batch:

.. code-block:: c

   static void fifo_ready(const struct device *dev,
                          const struct sensor_trigger *trigger)
   {
           struct lis2dh_fifo_sample samples[32];
           size_t count;
           int err;

           ARG_UNUSED(trigger);
           err = lis2dh_fifo_read(dev, samples, ARRAY_SIZE(samples), &count);
           if (err != 0) {
                   return;
           }
           /* Process samples[0..count-1] or copy them to application storage. */
   }

The handler runs in the configured trigger thread, so account for this array
in its stack size and keep processing short. ``lis2dh_fifo_read()`` does not
access the bus or wait for new data; it removes at most the requested number
of frames from the software queue. This queue is not filled while RTIO owns
the stream. Call ``lis2dh_fifo_stop()`` to release FIFO and INT1, and retry it
if cleanup returns an error.


Building and Running
********************

Build and flash the sample for HOLYIOT-25008:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/lis2dh_fifo
   :board: holyiot_25008/nrf54l15/cpuapp
   :goals: build flash
   :compact:

Debug diagnostics
*****************

Enable the optional debug configuration when bringing up a board:

.. code-block:: console

   west build -b holyiot_25008/nrf54l15/cpuapp samples/sensor/lis2dh_fifo -- -DEXTRA_CONF_FILE=debug.conf

The same configuration can be used with board ``xiao_ble/nrf52840``.
Alternatively, set ``CONFIG_LOG=y`` and ``CONFIG_SENSOR_LOG_LEVEL_DBG=y``
in the application configuration. The additional messages report:

* SPI controller, requested clock frequency, mode, word size, bit order and CS;
  or I2C controller and target address.
* The actual SPI WHO_AM_I command and received byte, not buffer addresses.
* A successful chip-ID check, or the received and expected IDs on mismatch.
* FIFO watermark, sample period and INT1 routing at start, followed by readback
  of CTRL1 through CTRL5, FIFO_CTRL and FIFO_SRC.
* Start rollback and stop status.

For example, the identification messages on SPI contain:

.. code-block:: text

   WHO_AM_I SPI TX command=0x8f, RX length=1
   WHO_AM_I SPI RX=0x33
   chip ID OK: WHO_AM_I[0x0f]=0x33

The FIFO dump is best-effort: a failed diagnostic read is logged but does not
turn a successful start into a failure. It uses two extra control/status reads
only in builds with sensor DEBUG logging, under the driver mutex. It neither
consumes XYZ samples nor reads the HP-filter reference register. These are
register snapshots, not an automatic configuration self-test.

No per-sample logging or FIFO payload hexdumps are added to the driver.
Nevertheless, logging and readback add startup latency. Disable DEBUG for
throughput/IRQ-latency measurements; software logs cannot validate physical
SPI timing or replace a logic analyzer.

Sample Output
*************

.. code-block:: console

   LIS2DH Sensor Async FIFO stream started
   123456789 ns: (0.010763, -0.004785, 9.801000) m/s^2
   133456789 ns: (0.015548, -0.009570, 9.796215) m/s^2
