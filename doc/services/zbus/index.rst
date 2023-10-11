.. _zbus:

Zephyr message bus (zbus)
#########################

The :dfn:`Zephyr message bus - Zbus` is a lightweight and flexible message bus enabling a simple way for threads to talk to one another.

.. contents::
    :local:
    :depth: 2

Concepts
********

Threads can broadcast messages to all interested observers using zbus. Many-to-many communication is possible. The bus implements message-passing and publish/subscribe communication paradigms that enable threads to communicate synchronously or asynchronously through shared memory. The communication through zbus is channel-based, where threads publish and read to and from using messages. Additionally, threads can observe channels and receive notifications from the bus when the channels are modified. :numref:`zbus common` shows an example of a typical application using zbus in which the application logic (hardware independent) talks to other threads via message bus. Note that the threads are decoupled from each other because they only use zbus' channels and do not need to know each other to talk.


.. _zbus common:
.. figure:: images/zbus_overview.svg
    :alt: zbus usage overview
    :width: 75%

    A typical zbus application architecture.

:numref:`zbus anatomy` illustrates zbus' anatomy. The bus comprises:

* Set of channels that consists of a unique identifier, its control metadata information, and the message itself;
* :dfn:`Virtual distributed event dispatcher` (VDED), the bus logic responsible for sending notifications to the observers. The VDED logic runs inside the publishing action in the same thread context, giving the bus an idea of a distributed execution. When a thread publishes to a channel, it also propagates the notifications to the observers;
* Threads (subscribers) and callbacks (listeners) publishing, reading, and receiving notifications from the bus.

.. _zbus anatomy:
.. figure:: images/zbus_anatomy.svg
    :alt: Zbus anatomy
    :width: 70%

    Zbus internals details.

The bus makes the publish, read, and subscribe actions available over channels. Publishing and reading are available in all RTOS contexts except inside an Interrupt Service Routine (ISR). The publish and read operations were designed to be simple and fast; the procedure is a mutex locking followed by a memory copy to and from a shared memory region. Zbus observers' registration can be:

* Static, defined in compile time. It is not possible to remove at runtime, but it is possible to suppress it by calling the :c:func:`zbus_obs_set_enable`;
* Dynamic, it can be added and removed to and from a channel at runtime.


For illustration purposes, suppose a usual sensor-based solution in :numref:`zbus operations`. When the timer is triggered, it pushes an action to a workqueue that publishes to the ``Start trigger`` channel. As the sensor thread subscribed to the ``Start trigger`` channel, it starts to fetch the sensor data. Notice the event dispatcher executes the blink callback because it also listens to the ``Start trigger`` channel. When the sensor data is ready, the sensor thread publishes it to the ``Sensor data`` channel. The core thread as a ``Sensor data`` channel subscriber process the sensor data and stores it in a internal sample buffer. It repeats until the sample buffer is full; when it happens, the core thread aggregates the sample buffer information, prepares a package, and publishes that to the ``Payload`` channel. The Lora thread receives that because it is a ``Payload`` channel subscriber and sends the payload to the cloud. When the transmission is completed, the Lora thread publishes to the ``Transmission done`` channel. The blink callback will be executed again since it listens to the ``Transmission done`` channel.



.. _zbus operations:
.. figure:: images/zbus_operations.svg
    :alt: Zbus sensor-based application
    :width: 80%

    Zbus sensor-based application.

This way of implementing the solution gives us certain flexibility enabling us to change things independently. For example, suppose we would like to change the trigger from a timer to a button press. We can do that, and the change does not affect other parts of the system. Suppose, again, we would like to change the communication interface from LoRa to Bluetooth; for that, we only need to change the LoRa thread. No other change is needed to make that work. Thus, the developer would do that for every block of the image. Based on that, there is a sign zbus promotes decoupling in the system architecture.

Another important aspect of using zbus is the reuse of system modules. If a module, code portion with a set of well-defined behaviors, only uses zbus channels and not hardware interfaces, it can easily be reused in other solutions. For that, the new solution must implement the interfaces (set of channels) the module needs to work. That indicates zbus could improve the module reuse.

The last important note is the zbus solution reach. We can count on many different ways of using zbus to enable the developer to be as free as possible to create what they need with it. Messages can be dynamic or static allocated, notifications can be synchronous or asynchronous, the developer can control the channel in so many different ways claiming the channel, developers can add their metadata information to a channel by using the user-data field, the discretionary use of a validator enables the systems to be accurate over message format, and so on. Those characteristics increase the solutions that can be done with zbus and make it a good fit as an open-source community tool.

Limitations
===========

Based on the fact that developers can use zbus to solve many different problems, some challenges arise. Zbus will not solve every problem, so it is necessary to analyze the situation to be sure zbus is applicable. For instance, based on the zbus benchmark, it would not be well suited to a high-speed stream of bytes between threads. The `Pipe` kernel object solves this kind of need.

Delivery guarantees
-------------------

Zbus always delivers the messages to the listeners. However, there are no message delivery guarantees for subscribers because zbus only sends the notification, but the message reading depends on the subscriber's implementation. It is possible to increase the delivery rate by following design tips:

* Keep the listeners quick-as-possible (deal with them as ISRs). If some processing is needed, consider submitting a work to a work-queue;
* Try to give producers a high priority to avoid losses;
* Leave spare CPU for observers to consume data produced;
* Consider using message queues or pipes for intensive byte transfers.


Message delivery sequence
-------------------------

The listeners (synchronous observers) will follow the channel definition sequence as the notification and message consumption sequence. However, the subscribers, as they have an asynchronous nature, all will receive the notification as the channel definition sequence but only will consume the data when they execute again, so the delivery respects the order, but the priority assigned to the subscribers will define the reaction sequence. All the listeners (static o dynamic) will receive the message before subscribers receive the notification. The sequence of delivery is: (i) static listeners; (ii) runtime listeners; (iii) static subscribers; at last (iv) runtime subscribers.

Implementation
**************

Zbus operation depends on channels and observers. Therefore, it is necessary to determine its message and observers list during the channel definition. A message is a regular C struct; the observer can be a subscriber (asynchronous) or a listener (synchronous). Channels can have a ``validator function`` that enables a channel to accept only valid messages.

The following code defines and initializes a regular channel and its dependencies. This channel exchanges accelerometer data, for example.

.. code-block:: c

    struct acc_msg {
            int x;
            int y;
            int z;
    };

    ZBUS_CHAN_DEFINE(acc_chan,				     /* Name */
             struct acc_msg,			     /* Message type */

             NULL,					     /* Validator */
             NULL,					     /* User Data */
             ZBUS_OBSERVERS(my_listener, my_subscriber), /* observers */
             ZBUS_MSG_INIT(.x = 0, .y = 0, .z = 0)	     /* Initial value {0} */
    );

    void listener_callback_example(const struct zbus_channel *chan)
    {
            const struct acc_msg *acc;
            if (&acc_chan == chan) {
                    acc = zbus_chan_const_msg(chan); // Direct message access
                    LOG_DBG("From listener -> Acc x=%d, y=%d, z=%d", acc->x, acc->y, acc->z);
            }
    }

    ZBUS_LISTENER_DEFINE(my_listener, listener_callback_example);

    ZBUS_SUBSCRIBER_DEFINE(my_subscriber, 4);
    void subscriber_task(void)
    {
            const struct zbus_channel *chan;

            while (!zbus_sub_wait(&my_subscriber, &chan, K_FOREVER)) {
                    struct acc_msg acc = {0};

                    if (&acc_chan == chan) {
                            // Indirect message access
                            zbus_chan_read(&acc_chan, &acc, K_NO_WAIT);
                            LOG_DBG("From subscriber -> Acc x=%d, y=%d, z=%d", acc.x, acc.y, acc.z);
                    }
            }
    }
    K_THREAD_DEFINE(subscriber_task_id, 512, subscriber_task, NULL, NULL, NULL, 3, 0, 0);


.. note::
    It is unnecessary to claim/lock a channel before accessing the message inside the listener since the event dispatcher calls listeners with the notifying channel already locked. Subscribers, however, must claim/lock that or use regular read operations to access the message after being notified.

The following code defines and initializes a :dfn:`hard channel` and its dependencies. Only valid messages can be published to a :dfn:`hard channel`. It is possible because a ``Validator function`` passed to the channel's definition. In this example, only messages with ``move`` equal to 0, -1, and 1 are valid. Publish function will discard all other values to ``move``.

.. code-block:: c

    struct control_msg {
            int move;
    };

    bool control_validator(const void* msg, size_t msg_size) {
            const struct control_msg* cm = msg;
            bool is_valid = (cm->move == -1) || (cm->move == 0) || (cm->move == 1);
            return is_valid;
    }

    static int message_count = 0;

    ZBUS_CHAN_DEFINE(control_chan,     /* Name */
             struct control_msg,      /* Message type */

             control_validator,       /* Validator */
             &message_count,          /* User data */
             ZBUS_OBSERVERS_EMPTY,    /* observers */
             ZBUS_MSG_INIT(.move = 0) /* Initial value {.move=0} */
    );

The following sections describe in detail how to use zbus features.


Publishing to a channel
=======================

Messages are published to a channel in zbus by calling :c:func:`zbus_chan_pub`. For example, the following code builds on the examples above and publishes to channel ``acc_chan``. The code is trying to publish the message ``acc1`` to channel ``acc_chan``, and it will wait up to one second for the message to be published. Otherwise, the operation fails.

.. code-block:: c

	struct acc_msg acc1 = {.x = 1, .y = 1, .z = 1};
	zbus_chan_pub(&acc_chan, &acc1, K_SECONDS(1));

.. warning::
    Do not use this function inside an ISR.

Reading from a channel
======================

Messages are read from a channel in zbus by calling :c:func:`zbus_chan_read`. So, for example, the following code tries to read the channel ``acc_chan``, which will wait up to 500 milliseconds to read the message. Otherwise, the operation fails.

.. code-block:: c

    struct acc_msg acc = {0};
    zbus_chan_read(&acc_chan, &acc, K_MSEC(500));

.. warning::
    Do not use this function inside an ISR.

Forcing channel notification
============================

It is possible to force zbus to notify a channel's observers by calling :c:func:`zbus_chan_notify`. For example, the following code builds on the examples above and forces a notification for the channel ``acc_chan``. Note this can send events with no message, which does not require any data exchange.

.. code-block:: c

    zbus_chan_notify(&acc_chan, K_NO_WAIT);

.. warning::
    Do not use this function inside an ISR.

Declaring channels and observers
================================

For accessing channels or observers from files other than its defining files, it is necessary to declare them by calling :c:macro:`ZBUS_CHAN_DECLARE` and :c:macro:`ZBUS_OBS_DECLARE`. It is possible to declare more than one channel or observer at the same call. The following code builds on the examples above and displays the defined channels and observers.

.. code-block:: c

    ZBUS_OBS_DECLARE(my_listener, my_subscriber);
    ZBUS_CHAN_DECLARE(acc_chan, version_chan);


Iterating over channels and observers
=====================================

There is an iterator mechanism in zbus that enables the developer to execute some procedure per channel and observer. The sequence executed is sorted by channel or observer name.

.. code-block:: c

    int count;

    bool print_channel_data_iterator(const struct zbus_channel *chan)
    {
            LOG_DBG("%d - Channel %s:", count, zbus_chan_name(chan));
            LOG_DBG("      Message size: %d", zbus_chan_msg_size(chan));
            ++count;
            LOG_DBG("      Observers:");
            for (struct zbus_observer **obs = chan->observers; *obs != NULL; ++obs) {
                LOG_DBG("      - %s", zbus_obs_name(*obs));
            }
            return true;
    }

    bool print_observer_data_iterator(const struct zbus_observer *obs)
    {
            LOG_DBG("%d - %s %s", count, ((obs->queue != NULL) ? "Subscriber" : "Listener"), zbus_obs_name(obs));
            ++count;
            return true;
    }
    void main(void)
    {
            LOG_DBG("Channel list:");
            count = 0;
            zbus_iterate_over_channels(print_channel_data_iterator);

            LOG_DBG("Observers list:");
            count = 0;
            zbus_iterate_over_observers(print_observer_data_iterator);
    }

The code will log the following output:

.. code-block:: console

    D: Channel list:
    D: 0 - Channel acc_chan:
    D:       Message size: 12
    D:       Observers:
    D:       - my_listener
    D:       - my_subscriber
    D: 1 - Channel version_chan:
    D:       Message size: 4
    D:       Observers:
    D: Observers list:
    D: 0 - Listener my_listener
    D: 1 - Subscriber my_subscriber


Advanced channel control
========================

Zbus was designed to be as flexible and extensible as possible. Thus there are some features designed to provide some control and extensibility to the bus.

Listeners message access
------------------------

For performance purposes, listeners can access the receiving channel message directly since they already have the mutex lock for it. To access the channel's message, the listener should use the ``zbus_chan_const_msg`` because the channel passed as an argument to the listener function is a constant pointer to the channel. The const pointer ensures that the message will be kept unchanged during the notification process.

.. code-block:: c

    void listener_callback_example(const struct zbus_channel *chan)
    {
            const struct acc_msg *acc;
            if (&acc_chan == chan) {
                    acc = zbus_chan_const_msg(chan); // Use this
                    // instead of zbus_chan_read(chan, &acc, K_MSEC(200))
                    // or zbus_chan_msg(chan)

                    LOG_DBG("From listener -> Acc x=%d, y=%d, z=%d", acc->x, acc->y, acc->z);
            }
    }

User Data
---------

There is a possibility of injecting data into the channel's metadata by passing the ``user_data`` pointer to the channel's definition macro. The ``user_data`` field enables others to access the data. Note that it needs to be set individually for each channel.

Claim and finish a channel
--------------------------

To take more control over channels, two function were added :c:func:`zbus_chan_claim` and :c:func:`zbus_chan_finish`. With these functions, it is possible to access the channel's metadata safely. When a channel is claimed, no actions are available to that channel. After finishing the channel, all the actions are available again.

.. warning::
    Never change the fields of the channel struct directly. It may cause zbus behavior inconsistencies and concurrency issues.

The following code builds on the examples above and claims the ``acc_chan`` to set the ``user_data`` to the channel. Suppose we would like to count how many times the channels exchange messages. We defined the ``user_data`` to have the 32 bits integer. This code could be added to the listener code described above.

.. code-block:: c

    if (!zbus_chan_claim(&acc_chan, K_MSEC(200))) {
            int *message_counting = (int *) zbus_chan_user_data(acc_chan);
            *message_counting += 1;
            zbus_chan_finish(&acc_chan);
    }

.. warning::
    Do not use these functions inside an ISR.


Runtime observer registration
-----------------------------

It is possible to add observers to channels in runtime. This feature uses the object pool pattern technique in which the dynamic nodes are pre-allocated and can be used and recycled. Therefore, it is necessary to set the pool size by changing the feature :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE` to enable this feature. Furthermore, it uses memory slabs. When necessary, turn on the :kconfig:option:`CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION` configuration to track the maximum usage of the pool. The following example illustrates the runtime registration usage.

.. code-block:: c

    ZBUS_LISTENER_DEFINE(my_listener, callback);
    // ...
    void thread_entry(void) {
            // ...
            /* Adding the observer to channel chan1 */
            zbus_chan_add_obs(&chan1, &my_listener);
            /* Removing the observer from channel chan1 */
            zbus_chan_rm_obs(&chan1, &my_listener);


Zbus can only use a limited number of dynamic observers. The configuration option :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE` represents the size of the runtime observers pool (memory slab). Change that to fit the solution needs. Use the :c:func:`k_mem_slab_num_used_get` to verify how many runtime observers slots are available. The function :c:func:`k_mem_slab_max_used_get` will provide information regarding the maximum number of used slots count reached during the execution. Use that to set the appropriate pool size avoiding waste. The following code illustrates how to use that.

.. code-block:: c

    extern struct k_mem_slab _zbus_runtime_obs_pool;
    uint32_t slots_available = k_mem_slab_num_free_get(&_zbus_runtime_obs_pool);
    uint32_t max_usage = k_mem_slab_max_used_get(&_zbus_runtime_obs_pool);


.. warning::
    Do not use ``_zbus_runtime_obs_pool`` memory slab directly. It may lead to inconsistencies.

Samples
*******

For a complete overview of zbus usage, take a look at the samples. There are the following samples available:

* :ref:`zbus-hello-world-sample` illustrates the code used above in action;
* :ref:`zbus-work-queue-sample` shows how to define and use different kinds of observers. Note there is an example of using a work queue instead of executing the listener as an execution option;
* :ref:`zbus-dyn-channel-sample` demonstrates how to use dynamically allocated exchanging data in zbus;
* :ref:`zbus-uart-bridge-sample` shows an example of sending the operation of the channel to a host via serial;
* :ref:`zbus-remote-mock-sample` illustrates how to implement an external mock (on the host) to send and receive messages to and from the bus.
* :ref:`zbus-runtime-obs-registration-sample` illustrates a way of using the runtime observer registration feature;
* :ref:`zbus-benchmark-sample` implements a benchmark with different combinations of inputs.

Suggested Uses
**************

Use zbus to transfer data (messages) between threads in one-to-one, one-to-many, and many-to-many synchronously or asynchronously.

.. note::
    Zbus can be used to transfer streams from the producer to the consumer. However, this can increase zbus' communication latency. So maybe consider a Pipe a good alternative for this communication topology.

Configuration Options
*********************

For enabling zbus, it is necessary to enable the :kconfig:option:`CONFIG_ZBUS` option.

Related configuration options:

* :kconfig:option:`CONFIG_ZBUS_CHANNEL_NAME`
* :kconfig:option:`CONFIG_ZBUS_OBSERVER_NAME`
* :kconfig:option:`CONFIG_ZBUS_STRUCTS_ITERABLE_ACCESS`
* :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE`

API Reference
*************

.. doxygengroup:: zbus_apis
