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
as that API matches up well with hardware DMA transfer queues and descriptions.

A quick sales pitch on why RTIO works well in many scenarios:

1. API is DMA and interrupt friendly
2. No buffer copying
3. No callbacks
4. Blocking or non-blocking operation

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

Submission Queue and Chaining
*****************************

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

Executor and IODev
******************

Turning submission queue entries (sqe) into completion queue events (cqe) is the
job of objects implementing the executor and iodev APIs. These APIs enable
coordination between themselves to enable things like DMA transfers.

The end result of these APIs should be a method to resolve the request by
deciding some of the following questions with heuristic/constraint
based decision making.

* Polling, Interrupt, or DMA transfer?
* If DMA, are the requirements met (peripheral supported by DMAC, etc).

The executor is meant to provide policy for when to use each transfer
type, and provide the common code for walking through submission queue
chains by providing calls the iodev may use to signal completion,
error, or a need to suspend and wait.

Outstanding Questions
*********************

RTIO is not a complete API and solution, and is currently evolving to best
fit the nature of an RTOS. The general ideas behind a pair of queues to
describe requests and completions seems sound and has been proven out in
other contexts. Questions remain though.

Timeouts and Deadlines
======================

Timeouts and deadlines are key to being Real-Time. Real-Time in Zephyr means
being able to do things when an application wants them done. That could mean
different things from a deadline with best effort attempts or a timeout and
failure.

These features would surely be useful in many cases, but would likely add some
significant complexities. It's something to decide upon, and even if enabled
would likely be a compile time optional feature leading to complex testing.

Cancellation
============

Canceling an already queued operation could be possible with a small
API addition to perhaps take both the RTIO context and a pointer to the
submission queue entry. However, cancellation as an API induces many potential
complexities that might not be appropriate. It's something to be decided upon.

Userspace Support
=================

RTIO with userspace is certainly plausible but would require the equivalent of
a memory map call to map the shared ringbuffers and also potentially dma buffers.

Additionally a DMA buffer interface would likely need to be provided for
coherence and MMU usage.

IODev and Executor API
======================

Lastly the API between an executor and iodev is incomplete.

There are certain interactions that should be supported. Perhaps things like
expanding a submission queue entry into multiple submission queue entries in
order to split up work that can be done by a device and work that can be done
by a DMA controller.

In some SoCs only specific DMA channels may be used with specific devices. In
others there are requirements around needing a DMA handshake or specific
triggering setups to tell the DMA when to start its operation.

None of that, from the outward facing API, is an issue.

It is however an unresolved task and issue from an internal API between the
executor and iodev. This requires some SoC specifics and enabling those
generically isn't likely possible. That's ok, an iodev and dma executor should
be vendor specific, but an API needs to be there between them that is not!


Special Hardware: Intel HDA
===========================

In some cases there's a need to always do things in a specific order
with a specific buffer allocation strategy. Consider a DMA that *requires*
the usage of a circular buffer segmented into blocks that may only be
transferred one after another. This is the case of the Intel HDA stream for
audio.

In this scenario the above API can still work, but would require an additional
buffer allocator to work with fixed sized segments.

When to Use
***********

It's important to understand when DMA like transfers are useful and when they
are not. It's a poor idea to assume that something made for high throughput will
work for you. There is a computational, memory, and latency cost to setup the
description of transfers.

Polling at 1Hz an air sensor will almost certainly result in a net negative
result compared to ad-hoc sensor (i2c/spi) requests to get the sample.

Continuous transfers, driven by timer or interrupt, of data from a peripheral's
on board FIFO over I2C, I3C, SPI, MIPI, I2S, etc... maybe, but not always!

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

RTIO API
========

.. doxygengroup:: rtio_api

RTIO SPSC API
=============

.. doxygengroup:: rtio_spsc
