.. zephyr:code-sample:: power-domain-tree
   :name: Power domain tree
   :relevant-api: subsys_pm_device subsys_pm_device_runtime

   Power a nested tree of domains on demand, including devices that depend on
   more than one power domain.

Overview
********

This sample builds a small tree of software power domains and a few consumer
devices, then turns the consumers on and off with the device runtime power
management API. It shows two things that a single-domain setup cannot:

* A device can depend on **more than one power domain** at the same time
  (``dev_b`` and ``dev_d``). Getting such a device powers up *every* domain it
  needs; putting it releases them all.
* A power domain can itself depend on **several parent domains** (``pd_shared``).
  Because every domain is reference counted, a parent stays powered as long as
  *any* path still needs it, and only powers off once the last user is gone.

Every node in the tree is instantiated from the same tiny driver
(:file:`src/power_domain_log.c`), which simply logs each power transition
(``resumed`` / ``suspended`` for the runtime state, ``on`` / ``off`` for the
power a parent applies or removes) and forwards notifications to the devices
that depend on it.

The dependency tree
*******************

.. mermaid::
   :caption: Power domain dependency tree used by the sample. Arrows point from
       a domain to the nodes it powers.
   :alt: pd_root powers pd_a and pd_b; pd_a powers pd_a1, pd_a2 and pd_shared;
       pd_b powers pd_b1 and pd_shared; pd_shared powers pd_deep. dev_a is on
       pd_a1, dev_b on pd_a2 and pd_b1, dev_c on pd_deep, dev_d on pd_a1 and
       pd_deep.

   graph TD
       pd_root["pd_root"] --> pd_a["pd_a"]
       pd_root --> pd_b["pd_b"]
       pd_a --> pd_a1["pd_a1"]
       pd_a --> pd_a2["pd_a2"]
       pd_b --> pd_b1["pd_b1"]
       pd_a --> pd_shared["pd_shared"]
       pd_b --> pd_shared
       pd_shared --> pd_deep["pd_deep"]

       pd_a1 --> dev_a("dev_a")
       pd_a2 --> dev_b("dev_b")
       pd_b1 --> dev_b
       pd_deep --> dev_c("dev_c")
       pd_a1 --> dev_d("dev_d")
       pd_deep --> dev_d

Consumer devices and the domains they depend on:

===========  ======================
Device       ``power-domains``
===========  ======================
``dev_a``    ``pd_a1``
``dev_b``    ``pd_a2``, ``pd_b1``
``dev_c``    ``pd_deep``
``dev_d``    ``pd_a1``, ``pd_deep``
===========  ======================

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/pm/power_domain_tree
   :board: native_sim
   :goals: build run
   :compact:

Sample Output
*************

Runtime PM is enabled automatically for every node (each sets
``zephyr,pm-device-runtime-auto``), so the consumers start powered ``off`` and
only ``pd_root`` — the always-available root rail — sits ``suspended``. The
application runs a fixed get/put sequence and, after each step, prints the
domain tree with every domain's runtime state. The per-node power transitions
are logged at debug level; raise the ``power_domain_log`` module to ``DBG`` to
see them.

.. code-block:: console

   === power domain tree demo ===
   initial state:
   pd_root           (suspended)
   ├─ pd_a           (off)
   │  ├─ pd_a1       (off)
   │  ├─ pd_a2       (off)
   │  └─ pd_shared   (off)   [also powered by pd_b]
   │     └─ pd_deep  (off)
   └─ pd_b           (off)
      └─ pd_b1       (off)

   > get dev_a
   pd_root           (active)
   ├─ pd_a           (active)
   │  ├─ pd_a1       (active)
   │  ├─ pd_a2       (suspended)
   │  └─ pd_shared   (suspended)   [also powered by pd_b]
   │     └─ pd_deep  (off)
   └─ pd_b           (suspended)
      └─ pd_b1       (off)

   > get dev_b
   pd_root           (active)
   ├─ pd_a           (active)
   │  ├─ pd_a1       (active)
   │  ├─ pd_a2       (active)
   │  └─ pd_shared   (suspended)   [also powered by pd_b]
   │     └─ pd_deep  (off)
   └─ pd_b           (active)
      └─ pd_b1       (active)

   > get dev_c
   pd_root           (active)
   ├─ pd_a           (active)
   │  ├─ pd_a1       (active)
   │  ├─ pd_a2       (active)
   │  └─ pd_shared   (active)   [also powered by pd_b]
   │     └─ pd_deep  (active)
   └─ pd_b           (active)
      └─ pd_b1       (active)

   > put dev_a
   pd_root           (active)
   ├─ pd_a           (active)
   │  ├─ pd_a1       (suspended)
   │  ├─ pd_a2       (active)
   │  └─ pd_shared   (active)   [also powered by pd_b]
   │     └─ pd_deep  (active)
   └─ pd_b           (active)
      └─ pd_b1       (active)

   > put dev_b
   pd_root           (active)
   ├─ pd_a           (active)
   │  ├─ pd_a1       (suspended)
   │  ├─ pd_a2       (suspended)
   │  └─ pd_shared   (active)   [also powered by pd_b]
   │     └─ pd_deep  (active)
   └─ pd_b           (active)
      └─ pd_b1       (suspended)

   > put dev_c
   pd_root           (suspended)
   ├─ pd_a           (off)
   │  ├─ pd_a1       (off)
   │  ├─ pd_a2       (off)
   │  └─ pd_shared   (off)   [also powered by pd_b]
   │     └─ pd_deep  (off)
   └─ pd_b           (off)
      └─ pd_b1       (off)

   === demo complete ===

Notice that releasing ``dev_a`` does not power ``pd_a`` down (``dev_b`` still
needs it), and releasing ``dev_b`` still keeps ``pd_a`` and ``pd_b`` up, because
``pd_shared`` — needed by ``dev_c`` — depends on both. Only when the last
consumer is released does the whole tree collapse. ``pd_root`` ends
``suspended`` rather than ``off`` because, as the root of the tree, no parent
ever removes its power.

Exploring with the shell
************************

After the scripted sequence the ``pd`` shell command is available to drive any
device or domain by hand:

.. code-block:: console

   uart:~$ pd get dev_d
       pd_root=active  pd_a=active  ...  pd_deep=active
   uart:~$ pd status
       pd_root=active  ...
   uart:~$ pd put dev_d
       pd_root=suspended  ...

``dev_d`` is a second multi-domain device (on ``pd_a1`` and ``pd_deep``); use it
to see two independent branches of the tree power up for a single consumer.
