README file for Contiki's IPv6 multicast core

Author: George Oikonomou

What does it do
===============
These files, alongside some core modifications, add support for IPv6 multicast
to contiki's uIPv6 engine.

Currently, two modes are supported:

* 'Stateless Multicast RPL Forwarding' (SMRF)
    RPL in MOP 3 handles group management as per the RPL docs,
    SMRF is a lightweight engine which handles datagram forwarding.
    SMRF is documented here:
    http://dx.doi.org/10.1007/s11277-013-1250-5
    and here:
    http://dx.doi.org/10.1109/PerComW.2012.6197494
* 'Multicast Forwarding with Trickle' according to the algorithm described
    in the internet draft:
    http://tools.ietf.org/html/draft-ietf-roll-trickle-mcast
    The version of this draft that's currently implementated is documented
    in `roll-tm.h`

More engines can (and hopefully will) be added in the future. The first
addition is most likely going to be an updated implementation of MPL

The Big Gotcha
==============
Currently we only support traffic originating and destined inside a single 6LoWPAN
To be able to send multicast traffic from the internet to 6LoWPAN nodes or the other
way round, we need border routers or other gateway devices to be able to achieve
the following:

* Add/Remove Trickle Multicast, RPL or other HBHO headers as necessary for datagrams
  entering / exiting the 6LoWPAN
* Advertise multicast group membership to the internet (e.g. with MLD)

These are currently not implemented and are in the ToDo list. Contributions welcome.

Where to Start
==============
The best place in `examples/ipv6/multicast`

There is a cooja example demonstrating basic functionality

How to Use
==========
Look in `core/net/ipv6/multicast/uip-mcast6-engines.h` for a list of supported
multicast engines.

To turn on multicast support, add this line in your `project-` or `contiki-conf.h`

        #define UIP_MCAST6_CONF_ENGINE xyz

  where xyz is a value from `uip-mcast6-engines.h`

To disable:

        #define UIP_MCAST6_CONF_ENGINE 0

You also need to make sure the multicast code gets built. Your example's
(or platform's) Makefile should include this:

        MODULES += core/net/ipv6/multicast

How to extend
=============
Let's assume you want to write an engine called foo.
The multicast API defines a multicast engine driver in a fashion similar to
the various NETSTACK layer drivers. This API defines functions for basic
multicast operations (init, in, out).
In order to extend multicast with a new engine, perform the following steps:

- Open `uip-mcast6-engines.h` and assign a unique integer code to your engine

        #define UIP_MCAST6_ENGINE_FOO        xyz

  - Include your engine's `foo.h`

- In `foo.c`, implement:
  * `init()`
  * `in()`
  * `out()`
  * Define your driver like so:

        `const struct uip_mcast6_driver foo_driver = { ... }`

- If you want to maintain stats:
  * Standard multicast stats are maintained in `uip_mcast6_stats`. Don't access
    this struct directly, use the macros provided in `uip-mcast6-stats.h` instead
  * You can add your own stats extensions. To do so, declare your own stats
    struct in your engine's module, e.g `struct foo_stats`
  * When you initialise the stats module with `UIP_MCAST6_STATS_INIT`, pass
    a pointer to your stats variable as the macro's argument.
    An example of how to extend multicast stats, look at the ROLL TM engine

- Open `uip-mcast6.h` and add a section in the `#if` spree. This aims to
  configure the uIPv6 core. More specifically, you need to:
  * Specify if you want to put RPL in MOP3 by defining
      `RPL_CONF_MULTICAST`: 1: MOP 3, 0: non-multicast MOP
  * Define your engine details

            #define UIP_MCAST6             foo_driver
            #define UIP_MCAST6_STATS       foo_stats
            typedef struct foo_stats uip_mcast6_stats_t;

  * Optionally, add a configuration check block to stop builds when the
    configuration is not sane.

If you need your engine to perform operations not supported by the generic
UIP_MCAST6 API, you will have to hook those in the uip core manually. As an
example, see how the core is modified so that it can deliver ICMPv6 datagrams
to the ROLL TM engine.
