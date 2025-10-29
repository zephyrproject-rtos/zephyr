.. _external_module_zephelin:

Zephelin
########

Introduction
************

`Zephyr Profiling Library`_ (ZPL), or Zephelin for short, is a library which enables capturing
and reporting runtime performance metrics for profiling and detailed analysis of Zephyr
applications, with a special focus on applications running AI/ML inference workloads.

In addition to the above, Zephelin also simplifies the analysis of AI runtimes, such as `LiteRT`_
and `microTVM`_, allowing you to gain a better understanding of the underlying bottlenecks or
potential opportunities for optimization.

Zephelin features:

* Tracing execution of Zephyr applications on hardware
* Obtaining traces using such backends as UART, USB or debug adapter
* Delivering traces in CTF and TEF formats
* Scripts for capturing traces from device
* Collecting readings from:

  * Memory - stack, heaps, kernel heaps and memory slabs
  * Sensors - e.g. die temperature sensors
  * Thread analysis - CPU usage
  * AI runtimes - e.g. tensor arena usage in LiteRT

* Displaying details on executed neural network layers in the LiteRT or microTVM runtime:

  * Dimensions of inputs, outputs and weights
  * Parameters of layers
  * Time and resources spent on executing specific layers

* Compilation-level and runtime-level configuration of the library

* Ability to configure a profiling tier, controlling the subsystems and the amount of data collected

All of the above can be analyzed with `Zephelin Trace Viewer`_.

Usage With Zephyr
*****************

To use Zephelin as a Zephyr :ref:`module <modules>`, add the following entry:

.. code-block:: yaml

   manifest:
     projects:
       - name: zephelin
         url: https://github.com/antmicro/zephelin
         revision: main
         path: modules/zephelin # adjust the path as needed

to a Zephyr submanifest (e.g. ``zephyr/submanifests/zephelin.yaml``) and run ``west update``, or
add it as a West project in your project's ``west.yaml`` manifest.

Please consult the `Zephelin documentation`_ for more information.

References
**********

.. target-notes::

.. _Zephyr Profiling Library:
   https://github.com/antmicro/zephelin

.. _Zephelin documentation:
   https://antmicro.github.io/zephelin/

.. _Zephelin Trace Viewer:
   https://antmicro.github.io/zephelin-trace-viewer

.. _LiteRT:
   https://ai.google.dev/edge/litert

.. _microTVM:
   https://tvm.apache.org/docs/v0.9.0/topic/microtvm/index.html
