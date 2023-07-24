.. _rtio_api:

Real Time I/O (RTIO)
####################

.. contents::
  :local:
  :depth: 2

.. image:: rings.png
  :width: 800
  :alt: Submissions and Completion Ring Queues

RTIO provides a framework for doing asynchronous operation chains with event
driven I/O. This section covers the RTIO API, queues, executor, iodev,
and common usage patterns with peripheral devices.

RTIO takes a lot of inspiration from Linux's io_uring in its operations and API
as that API matches up well with hardware transfer queues and descriptions such as
DMA transfer lists.

Problem
*******

An application wishing to do complex DMA or interrupt driven operations today
in Zephyr requires direct knowledge of the hardware and how it works. There is
no understanding in the DMA API of other Zephyr devices and how they relate.

This means doing complex audio, video, or sensor streaming requires direct
hardware knowledge or leaky abstractions over DMA controllers. Neither is ideal.

To enable asynchronous operations, especially with DMA, a description of what
to do rather than direct operations through C and callbacks is needed. Enabling
DMA features such as channels with priority, and sequences of transfers requires
more than a simple list of descriptions.

Using DMA and/or interrupt driven I/O shouldn't dictate whether or not the
call is blocking or not.

Inspiration, introducing io_uring
*********************************

It's better not to reinvent the wheel (or ring in this case) and io_uring as an
API from the Linux kernel provides a winning model. In io_uring there are two
lock-free ring buffers acting as queues shared between the kernel and a userland
application. One queue for submission entries which may be chained and flushed to
create concurrent sequential requests. A second queue for completion queue events.
Only a single syscall is actually required to execute many operations, the
io_uring_submit call. This call may block the caller when a number of
operations to wait on is given.

This model maps well to DMA and interrupt driven transfers. A request to do a
sequence of operations in an asynchronous way directly relates
to the way hardware typically works with interrupt driven state machines
potentially involving multiple peripheral IPs like bus and DMA controllers.

Submission Queue
****************

The submission queue (sq), is the description of the operations
to perform in concurrent chains.

For example imagine a typical SPI transfer where you wish to write a
register address to then read from. So the sequence of operations might be...

   1. Chip Select
   2. Clock Enable
   3. Write register address into SPI transmit register
   4. Read from the SPI receive register into a buffer
   5. Disable clock
   6. Disable Chip Select

If anything in this chain of operations fails give up. Some of those operations
can be embodied in a device abstraction that understands a read or write
implicitly means setup the clock and chip select. The transactional nature of
the request also needs to be embodied in some manner. Of the operations above
perhaps the read could be done using DMA as its large enough make sense. That
requires an understanding of how to setup the device's particular DMA to do so.

The above sequence of operations is embodied in RTIO as chain of
submission queue entries (sqe). Chaining is done by setting a bitflag in
an sqe to signify the next sqe must wait on the current one.

Because the chip select and clocking is common to a particular SPI controller
and device on the bus it is embodied in what RTIO calls an iodev.

Multiple operations against the same iodev are done in the order provided as
soon as possible. If two operation chains have varying points using the same
device its possible one chain will have to wait for another to complete.

Completion Queue
****************

In order to know when a sqe has completed there is a completion
queue (cq) with completion queue events (cqe). A sqe once completed results in
a cqe being pushed into the cq. The ordering of cqe may not be the same order of
sqe. A chain of sqe will however ensure ordering and failure cascading.

Other potential schemes are possible but a completion queue is a well trod
idea with io_uring and other similar operating system APIs.

Executor
********

The RTIO executor is a low overhead concurrent I/O task scheduler. It ensures
certain request flags provide the expected behavior. It takes a list of
submissions working through them in order. Various flags allow for changing the
behavior of how submissions are worked through. Flags to form in order chains of
submissions, transactional sets of submissions, or create multi-shot
(continuously producing) requests are all possible!

IO Device
*********

Turning submission queue entries (sqe) into completion queue events (cqe) is the
job of objects implementing the iodev (IO device) API. This API accepts requests
in the form of the iodev submit API call. It is the io devices job to work
through its internal queue of submissions and convert them into completions. In
effect every io device can be viewed as an independent, event driven actor like
object, that accepts a never ending queue of I/O like requests. How the iodev
does this work is up to the author of the iodev, perhaps the entire queue of
operations can be converted to a set of DMA transfer descriptors, meaning the
hardware does almost all of the real work.

Cancellation
************

Canceling an already queued operation is possible but not guaranteed. If the
SQE has not yet started, it's likely that a call to :c:func:`rtio_sqe_cancel`
will remove the SQE and never run it. If, however, the SQE already started
running, the cancel request will be ignored.

Memory pools
************

In some cases requests to read may not know how much data will be produced.
Alternatively, a reader might be handling data from multiple io devices where
the frequency of the data is unpredictable. In these cases it may be wasteful
to bind memory to in flight read requests. Instead with memory pools the memory
to read into is left to the iodev to allocate from a memory pool associated with
the RTIO context that the read was associated with. To create such an RTIO
context the :c:macro:`RTIO_DEFINE_WITH_MEMPOOL` can be used. It allows creating
an RTIO context with a dedicated pool of "memory blocks" which can be consumed by
the iodev. Below is a snippet setting up the RTIO context with a memory pool.
The memory pool has 128 blocks, each block has the size of 16 bytes, and the data
is 4 byte aligned.

.. code-block:: C

  #include <zephyr/rtio/rtio.h>

  #define SQ_SIZE       4
  #define CQ_SIZE       4
  #define MEM_BLK_COUNT 128
  #define MEM_BLK_SIZE  16
  #define MEM_BLK_ALIGN 4

  RTIO_DEFINE_WITH_MEMPOOL(rtio_context,
      SQ_SIZE, CQ_SIZE, MEM_BLK_COUNT, MEM_BLK_SIZE, MEM_BLK_ALIGN);

When a read is needed, the caller simply needs to replace the call
:c:func:`rtio_sqe_prep_read` (which takes a pointer to a buffer and a length)
with a call to :c:func:`rtio_sqe_prep_read_with_pool`. The iodev requires
only a small change which works with both pre-allocated data buffers as well as
the mempool. When the read is ready, instead of getting the buffers directly
from the :c:struct:`rtio_iodev_sqe`, the iodev should get the buffer and count
by calling :c:func:`rtio_sqe_rx_buf` like so:

.. code-block:: C

  uint8_t *buf;
  uint32_t buf_len;
  int rc = rtio_sqe_rx_buff(iodev_sqe, MIN_BUF_LEN, DESIRED_BUF_LEN, &buf, &buf_len);

  if (rc != 0) {
    LOG_ERR("Failed to get buffer of at least %u bytes", MIN_BUF_LEN);
    return;
  }

Finally, the consumer will be able to access the allocated buffer via
c:func:`rtio_cqe_get_mempool_buffer`.

.. code-block:: C

  uint8_t *buf;
  uint32_t buf_len;
  int rc = rtio_cqe_get_mempool_buffer(&rtio_context, &cqe, &buf, &buf_len);

  if (rc != 0) {
    LOG_ERR("Failed to get mempool buffer");
    return rc;
  }

  /* Release the cqe events (note that the buffer is not released yet */
  rtio_cqe_release_all(&rtio_context);

  /* Do something with the memory */

  /* Release the mempool buffer */
  rtio_release_buffer(&rtio_context, buf);

When to Use
***********

RTIO is useful in cases where concurrent or batch like I/O flows are useful.

From the driver/hardware perspective the API enables batching of I/O requests, potentially in an optimal way.
Many requests to the same SPI peripheral for example might be translated to hardware command queues or DMA transfer
descriptors entirely. Meaning the hardware can potentially do more than ever.

There is a small cost to each RTIO context and iodev. This cost could be weighed
against using a thread for each concurrent I/O operation or custom queues and
threads per peripheral. RTIO is much lower cost than that.

Examples
********

Examples speak loudly about the intended uses and goals of an API. So several key
examples are presented below. Some are entirely plausible today without a
big leap. Others (the sensor example) would require additional work in other
APIs outside of RTIO as a sub system and are theoretical.

Chained Blocking Requests
=========================

A common scenario is needing to write the register address to then read from.
This can be accomplished by chaining a write into a read operation.

The transaction on i2c is implicit for each operation chain.

.. code-block:: C

	RTIO_I2C_IODEV(i2c_dev, I2C_DT_SPEC_INST(n));
	RTIO_DEFINE(ez_io, 4, 4);
	static uint16_t reg_addr;
	static uint8_t buf[32];

	int do_some_io(void)
	{
		struct rtio_sqe *write_sqe = rtio_spsc_acquire(ez_io.sq);
		struct rtio_sqe *read_sqe = rtio_spsc_acquire(ez_io.sq);

		rtio_sqe_prep_write(write_sqe, i2c_dev, RTIO_PRIO_LOW, &reg_addr, 2);
		write_sqe->flags = RTIO_SQE_CHAINED; /* the next item in the queue will wait on this one */

		rtio_sqe_prep_read(read_sqe, i2c_dev, RTIO_PRIO_LOW, buf, 32);

		rtio_submit(rtio_inplace_executor, &ez_io, 2);

		struct rtio_cqe *read_cqe = rtio_spsc_consume(ez_io.cq);
		struct rtio_cqe *write_cqe = rtio_spsc_consume(ez_io.cq);

		if(read_cqe->result < 0) {
			LOG_ERR("read failed!");
		}

		if(write_cqe->result < 0) {
			LOG_ERR("write failed!");
		}

		rtio_spsc_release(ez_io.cq);
		rtio_spsc_release(ez_io.cq);
	}

Non blocking device to device
=============================

Imagine wishing to read from one device on an I2C bus and then write the same
buffer  to a device on a SPI bus without blocking the thread or setting up
callbacks or other IPC notification mechanisms.

Perhaps an I2C temperature sensor and a SPI lowrawan module. The following is a
simplified version of that potential operation chain.

.. code-block:: C

	RTIO_I2C_IODEV(i2c_dev, I2C_DT_SPEC_INST(n));
	RTIO_SPI_IODEV(spi_dev, SPI_DT_SPEC_INST(m));

	RTIO_DEFINE(ez_io, 4, 4);
	static uint8_t buf[32];

	int do_some_io(void)
	{
		uint32_t read, write;
		struct rtio_sqe *read_sqe = rtio_spsc_acquire(ez_io.sq);
		rtio_sqe_prep_read(read_sqe, i2c_dev, RTIO_PRIO_LOW, buf, 32);
		read_sqe->flags = RTIO_SQE_CHAINED; /* the next item in the queue will wait on this one */

		/* Safe to do as the chained operation *ensures* that if one fails all subsequent ops fail */
		struct rtio_sqe *write_sqe = rtio_spsc_acquire(ez_io.sq);
		rtio_sqe_prep_write(write_sqe, spi_dev, RTIO_PRIO_LOW, buf, 32);

		/* call will return immediately without blocking if possible */
		rtio_submit(rtio_inplace_executor, &ez_io, 0);

		/* These calls might return NULL if the operations have not yet completed! */
		for (int i = 0; i < 2; i++) {
			struct rtio_cqe *cqe = rtio_spsc_consume(ez_io.cq);
			while(cqe == NULL) {
				cqe = rtio_spsc_consume(ez_io.cq);
				k_yield();
			}
			if(cqe->userdata == &read && cqe->result < 0) {
				LOG_ERR("read from i2c failed!");
			}
			if(cqe->userdata == &write && cqe->result < 0) {
				LOG_ERR("write to spi failed!");
			}
			/* Must release the completion queue event after consume */
			rtio_spsc_release(ez_io.cq);
		}
	}

Nested iodevs for Devices on Buses (Sensors), Theoretical
=========================================================

Consider a device like a sensor or audio codec sitting on a bus.

Its useful to consider that the sensor driver can use RTIO to do I/O on the SPI
bus, while also being an RTIO device itself. The sensor iodev can set aside a
small portion of the buffer in front or in back to store some metadata describing
the format of the data. This metadata could then be used in creating a sensor
readings iterator which lazily lets you map over each reading, doing
calculations such as FIR/IIR filtering, or perhaps translating the readings into
other numerical formats with useful measurement units such as SI. RTIO is a
common movement API and allows for such uses while not deciding the mechanism.

This same sort of setup could be done for other data streams such as audio or
video.

.. code-block:: C

	/* Note that the sensor device itself can use RTIO to get data over I2C/SPI
	 * potentially with DMA, but we don't need to worry about that here
	 * All we need to know is the device tree node_id and that it can be an iodev
	 */
	RTIO_SENSOR_IODEV(sensor_dev, DEVICE_DT_GET(DT_NODE(super6axis));

	RTIO_DEFINE(ez_io, 4, 4);


	/* The sensor driver decides the minimum buffer size for us, we decide how
	 * many bufs. This could be a typical multiple of a fifo packet the sensor
	 * produces, ICM42688 for example produces a FIFO packet of 20 bytes in
	 * 20bit mode at 32KHz so perhaps we'd like to get 4 buffers of 4ms of data
	 * each in this setup to process on. and its already been defined here for us.
	 */
	#include <sensors/icm42688_p.h>
	static uint8_t bufs[4][ICM42688_RTIO_BUF_SIZE];

	int do_some_sensors(void) {
		/* Obtain a dmac executor from the DMA device */
		struct device *dma = DEVICE_DT_GET(DT_NODE(dma0));
		const struct rtio_executor *rtio_dma_exec =
				dma_rtio_executor(dma);

		/*
		 * Set the executor for our queue context
		 */
		 rtio_set_executor(ez_io, rtio_dma_exec);

		/* Mostly we want to feed the sensor driver enough buffers to fill while
		 * we wait and process! Small enough to process quickly with low latency,
		 * big enough to not spend all the time setting transfers up.
		 *
		 * It's assumed here that the sensor has been configured already
		 * and each FIFO watermark interrupt that occurs it attempts
		 * to pull from the queue, fill the buffer with a small metadata
		 * offset using its own rtio request to the SPI bus using DMA.
		 */
		for(int i = 0; i < 4; i++) {
			struct rtio_sqe *read_sqe = rtio_spsc_acquire(ez_io.sq);

			rtio_sqe_prep_read(read_sqe, sensor_dev, RTIO_PRIO_HIGH, bufs[i], ICM42688_RTIO_BUF_SIZE);
		}
		struct device *sensor = DEVICE_DT_GET(DT_NODE(super6axis));
		struct sensor_reader reader;
		struct sensor_channels channels[4] = {
			SENSOR_TIMESTAMP_CHANNEL,
			SENSOR_CHANNEL(int32_t, SENSOR_ACC_X, 0, SENSOR_RAW),
			SENSOR_CHANNEL(int32_t SENSOR_ACC_Y, 0, SENSOR_RAW),
			SENSOR_CHANNEL(int32_t, SENSOR_ACC_Z, 0, SENSOR_RAW),
		};
		while (true) {
			/* call will wait for one completion event */
			rtio_submit(ez_io, 1);
			struct rtio_cqe *cqe = rtio_spsc_consume(ez_io.cq);
			if(cqe->result < 0) {
				LOG_ERR("read failed!");
				goto next;
			}

			/* Bytes read into the buffer */
			int32_t bytes_read = cqe->result;

			/* Retrieve soon to be reusable buffer pointer from completion */
			uint8_t *buf = cqe->userdata;


			/* Get an iterator (reader) that obtains sensor readings in integer
			 * form, 16 bit signed values in the native sensor reading format
			 */
			res = sensor_reader(sensor, buf, cqe->result, &reader, channels,
							    sizeof(channels));
			__ASSERT(res == 0);
			while(sensor_reader_next(&reader)) {
				printf("time(raw): %d, acc (x,y,z): (%d, %d, %d)\n",
				channels[0].value.u32, channels[1].value.i32,
				channels[2].value.i32, channels[3].value.i32);
			}

	next:
			/* Release completion queue event */
			rtio_spsc_release(ez_io.cq);

			/* resubmit a read request with the newly freed buffer to the sensor */
			struct rtio_sqe *read_sqe = rtio_spsc_acquire(ez_io.sq);
			rtio_sqe_prep_read(read_sqe, sensor_dev, RTIO_PRIO_HIGH, buf, ICM20649_RTIO_BUF_SIZE);
		}
	}

API Reference
*************

.. doxygengroup:: rtio

MPSC Lock-free Queue API
========================

.. doxygengroup:: rtio_mpsc

SPSC Lock-free Queue API
========================

.. doxygengroup:: rtio_spsc
