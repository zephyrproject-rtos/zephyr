Event Controller Power Management
#################################

Introduction
************

Event Controller (EVTC) Power Management (PM) minimizes EVTC's power
usage to meet hardware event (EVT) latencies requested by the system.

EVT
***

Hardware events originate from and are routed to hardware peripherals.

Examples of EVTs:

* GPIO state changed.
* Interrupt triggered.

Events can be predictable and unpredictable:

* Predictable events originate at predictable times.
* Unpredictable events originate at unpredictable times.

EVT latency
***********

The EVT latency is the time from a hardware event originating until
it reaches its destination.

Examples of EVT latencies:

* From GPIO state changed to interrupt being triggered.
* From interrupt triggered to interrupt being serviced.

PM EVTC
*******

PM EVTCs manage EVT latencies.

Examples of PM EVTCs:

* Nordic power device (:dtcompatible:`nordic,nrf-power`)
* Nordic GPIOTE device (:dtcompatible:`nordic,nrf-gpiote`)
* Nordic DPPIC device (:dtcompatible:`nordic,nrf-dppic`)

Predictable EVT latency management
**********************************

Purpose
-------

To minimize latency just in time for an EVT to originate, and
maximize latency right after the EVT has been serviced.

Usage
-----

1. User configures EVT originator to originate EVT at predictable
   time.
2. User schedules PM EVT with PM EVTC using
   :c:func:`pm_evtc_schedule_evt()`, specifying:
   * A time shortly before the EVT is predicted to originate.
   * The maximum acceptable latency of the EVT.
3. PM EVTC minimizes EVT latency just in time for it to be in
   effect before the event is predicted to originate.
4. EVT originates
5. EVT is handled by the user
6. User releases scheduled PM EVT using
   :c:func:`pm_evtc_release_evt()`
7. PM EVTC maximizes EVT latency.

The time spent with minimized latency, and thus higher energy
consumption, is minimized to just before the event to just after
the event has been handled, as notified by the user.

A PM EVT can be additionally be rescheduled using
:c:func:`pm_evtc_reschedule_evt()` which implicitly and efficiently
releases the PM EVT followed by scheduling it.

Unpredictable EVT latency management
************************************

Purpose
-------

To ensure a maximum EVT latency while maximizing EVT latency.

Usage
-----

1. User configures EVT originator to originate EVTs
2. User requests maximum EVT latency with PM EVTC using
   :c:func:`pm_evtc_request_evt()`.
3. PM EVTC minimizes EVT latency immediately to be in effect as "soon
   as possible".

While the EVT latency is in effect, it is not neccesarily constant,
but will not exceed the requested maximum.

4. EVT originates.
5. EVT is handled by the user.

The above steps are repeated until the user no longer requires a
maximum EVT latency.

6. User releases EVT using :c:func:`pm_evtc_release_evt()`.
7. PM EVTC maximizes EVT latency.

PM EVTC implementation
**********************

Devicetree
----------

Event controllers which support power management must include the base
binding ``evtc.yaml``:

.. code-block:: yaml

   include:
     - "evtc.yaml"

which includes the required properties ``evtc-request-latency-ns`` and
``evtc-latencies-ns`` which must be defined in the devicetree:

.. code-block:: devicetree

   evtc0: evtc0 {
           /* Latency for request to take effect in nanoseconds */
           evtc-request-latency-ns = <3500>;
           /* Supported event latencies in nanoseconds */
           evtc-latencies-ns = <200 150 50 20>;
   };

Device driver
-------------

PM EVTC device drivers must implement the API
:c:func:`pm_evtc_request()` to which the following contract applies:

* If :kconfig:option:`CONFIG_PM_EVTC` ``=y`` the highest supported
  event latency will be requested on init.
* If :kconfig:option:`CONFIG_PM_EVTC` ``=n`` the lowest supported
  event latency will be requested on init.
* Only supported latencies will be requested as defined in the
  devicetree property ``evtc-latencies-ns``
* Only latencies different from the current latency will be requested.
* Request intervals are limited by the devicetree property
  ``evtc-request-latency-ns``.
* The request will be performed from ISR context, and is not reentrant.

The PM EVTC instance which implements the :c:func:`pm_evtc_request()`
API is defined using the :c:macro:`PM_EVTC_DT_DEFINE()`:

.. code-block:: c

   struct evtc_dev_config {
           const struct pm_evtc *pm_evtc;
   };

   /* Implementation of pm_evtc_request() */
   static void evtc_dev_request(const struct device *dev, uint32_t latency_ns)
   {
           /*
            * No need to validate latency_ns here nor check if its
            * already applied.
            */
   }

   static int evtc_dev_init(const struct device *dev)
   {
           const struct evtc_dev_config *config = dev->config;

           pm_evtc_init(config->pm_evtc);
           return 0;
   }

   static const struct evtc_driver_config config0 = {
           .pm_evtc = PM_EVTC_DT_GET(DT_NODELABEL(evtc0)),
   };

   PM_EVTC_DT_DEFINE(DT_NODELABEL(evtc0), evtc_dev_request);

   /* Device passed to implementation of pm_evtc_request() */
   DEVICE_DT_DEFINE(DT_NODELABEL(evtc0), evtc_dev_init, ..., &config0, ...);

:c:func:`pm_evtc_init()` calls the implementation of :c:func:`pm_evtc_request()`
regardless of :kconfig:option:`CONFIG_PM_EVTC`.
