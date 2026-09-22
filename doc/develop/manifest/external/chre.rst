.. _external_module_chre:

Context Hub Runtime Environment (CHRE)
######################################

Introduction
************

The `Context Hub Runtime Environment`_ (CHRE) is Android's platform for always-on applications,
called *nanoapps*, which run on a low-power processor next to the application processor running
Android. Nanoapps are written against the CHRE API, which is standardized across platforms, and
are hosted by the CHRE framework, the AOSP reference implementation of that API.

The `zephyrproject-rtos/chre`_ repository is a fork of the AOSP framework with a Zephyr port under
``platform/zephyr``. The port runs the CHRE event loop in a dedicated thread and provides the
platform primitives (memory, timers, system time, logging and host link) the framework needs.
The Platform Abstraction Layers (PALs) for audio, GNSS, sensors, WiFi and WWAN are not implemented
for Zephyr.

CHRE is licensed under the Apache-2.0 license.

Usage with Zephyr
*****************

To pull in CHRE as a Zephyr :ref:`module <modules>`, either add it as a West project in the
``west.yaml`` file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/chre.yaml``)
file with the following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: chre
         url: https://github.com/zephyrproject-rtos/chre
         revision: zephyr
         path: modules/lib/chre # adjust the path as needed

Enable the framework with ``CONFIG_CHRE=y``. CHRE is written in C++ and selects
:kconfig:option:`CONFIG_REQUIRES_FULL_LIBCPP`, so the toolchain must provide a full C++ standard
library. The remaining ``CONFIG_CHRE_*`` options in the module's ``platform/zephyr/Kconfig`` size
the event loop thread and memory pool and enable the individual PAL frameworks.

The module contains a sample application under ``zephyr/sample`` which starts the event loop, loads
a nanoapp, delivers an event to it and shuts down. Build and run it on :zephyr:board:`native_sim`
with:

.. code-block:: console

   west build -b native_sim modules/lib/chre/zephyr/sample -t run

Reference
*********

.. target-notes::

.. _Context Hub Runtime Environment:
   https://source.android.com/docs/core/interaction/contexthub

.. _zephyrproject-rtos/chre:
   https://github.com/zephyrproject-rtos/chre
