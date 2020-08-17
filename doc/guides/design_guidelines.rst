.. _design_guidelines:

API Design Guidelines
#####################

Zephyr development and evolution is a group effort, and to simplify
maintenance and enhancements there are some general policies that should
be followed when developing a new capability or interface.

Using Callbacks
***************

Many APIs involve passing a callback as a parameter or as a member of a
configuration structure.  The following policies should be followed when
specifying the signature of a callback:

* The first parameter should be a pointer to the object most closely
  associated with the callback.  In the case of device drivers this
  would be ``struct device *dev``.  For library functions it may be a
  pointer to another object that was referenced when the callback was
  provided.

* The next parameter(s) should be additional information specific to the
  callback invocation, such as a channel identifier, new status value,
  and/or a message pointer followed by the message length.

* The final parameter should be a ``void *user_data`` pointer carrying
  context that allows a shared callback function to locate additional
  material necessary to process the callback.

An exception to providing ``user_data`` as the last parameter may be
allowed when the callback itself was provided through a structure that
will be embedded in another structure.  An example of such a case is
:c:type:`gpio_callback`, normally defined within a data structure
specific to the code that also defines the callback function.  In those
cases further context can accessed by the callback indirectly by
:c:macro:`CONTAINER_OF`.

Examples
========

* The requirements of :c:type:`k_timer_expiry_t` invoked when a system
  timer alarm fires are satisfied by::

    void handle_timeout(struct k_timer *timer)
    { ... }

  The assumption here, as with :c:type:`gpio_callback`, is that the
  timer is embedded in a structure reachable from
  :c:macro:`CONTAINER_OF` that can provide additional context to the
  callback.

* The requirements of :c:type:`counter_alarm_callback_t` invoked when a
  counter device alarm fires are satisfied by::

    void handle_alarm(struct device *dev,
                      uint8_t chan_id,
		      uint32_t ticks,
		      void *user_data)
    { ... }

  This provides more complete useful information, including which
  counter channel timed-out and the counter value at which the timeout
  occurred, as well as user context which may or may not be the
  :c:type:`counter_alarm_cfg` used to register the callback, depending
  on user needs.
