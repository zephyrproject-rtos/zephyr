.. zephyr:code-sample:: userspace_prod_consumer
   :name: Producer/consumer

   Manipulate basic user mode concepts.

Overview
********

Consider a "sample driver" which gets incoming data from some unknown source
and generates interrupts with pointers to this data. The application needs
to perform some processing on this data and then write the processed data
back to the driver.

The goal here is to demonstrate:

 - Multiple logical applications, each with their own memory domain
 - Creation of a sys_heap and assignment to a memory partition
 - Use of APIs like ``k_queue_alloc_append()`` which require thread resource
   pools to be configured
 - Management of permissions for kernel objects and drivers
 - Show how application-specific system calls are defined
 - Show IPC between ISR and application (using ``k_msgq``) and
   application-to-application IPC (using ``k_queue``)
 - Show how to create application-specific system calls

In this example, we have an Application A whose job is to talk to the
driver, buffer incoming data, and write it back once processed by
Application B.

Application B simply processes the data. Let's pretend this data is
untrusted and possibly malicious, so Application B is sandboxed from
everything else, with just two queues for sending/receiving data items.

The control loop is as follows:

 - Sample driver issues interrupts, invoking its associated callback
   function with a fixed-sized data payload.
 - App A callback function, in supervisor mode, places the data payload
   into a message queue.
 - App A monitor thread in user mode waits for data in the message queue.
   When it wakes up, copy the data payload into a buffer allocated out
   of the shared memory pool, and enqueue this data into a ``k_queue`` being
   monitored by application B.
 - Application B processing thread waits on new items in the queue. It
   then processes the data in-place, and after it's finished it places
   the processed data into another queue to be written back to the driver.
 - Application A writeback thread monitors the outgoing data queue for
   new items containing processed data. As it gets them it will write
   such data back to the driver and free the buffer.

We also demonstrate application-defined system calls, in the form of
the ``magic_cookie()`` function.

Sample Output
*************

.. code-block:: console

    I:APP A partition: 0x00110000 4096
    I:Shared partition: 0x0010e000 4096
    I:sample_driver_foo_isr: param=0x00147078 count=0
    I:monitor thread got data payload #0
    I:sample_driver_foo_isr: param=0x00147078 count=1
    I:monitor thread got data payload #1
    I:sample_driver_foo_isr: param=0x00147078 count=2
    I:monitor thread got data payload #2
    I:sample_driver_foo_isr: param=0x00147078 count=3
    I:monitor thread got data payload #3
    I:sample_driver_foo_isr: param=0x00147078 count=4
    I:monitor thread got data payload #4
    I:processing payload #1 complete
    I:writing processed data blob back to the sample device
    I:sample_driver_foo_isr: param=0x00147078 count=5
    I:monitor thread got data payload #5
    I:processing payload #2 complete
    I:writing processed data blob back to the sample device
    I:sample_driver_foo_isr: param=0x00147078 count=6
    I:monitor thread got data payload #6
    I:processing payload #3 complete
    I:writing processed data blob back to the sample device
    I:sample_driver_foo_isr: param=0x00147078 count=7
    I:monitor thread got data payload #7
    I:processing payload #4 complete
    I:writing processed data blob back to the sample device
    I:sample_driver_foo_isr: param=0x00147078 count=8
    I:monitor thread got data payload #8
    I:processing payload #5 complete
    I:writing processed data blob back to the sample device
    I:sample_driver_foo_isr: param=0x00147078 count=9
    I:monitor thread got data payload #9
    I:processing payload #6 complete
    I:writing processed data blob back to the sample device
    I:processing payload #7 complete
    I:writing processed data blob back to the sample device
    I:processing payload #8 complete
    I:writing processed data blob back to the sample device
    I:processing payload #9 complete
    I:writing processed data blob back to the sample device
    I:processing payload #10 complete
    I:writing processed data blob back to the sample device
    I:SUCCESS
