.. _external_module_clutgen:

CLUTGen
#######

Introduction
************

`CLUTGen <clutgen-zephyr_>`_ automates the creation of **Look-Up Tables** for embedded systems,
converting raw ADC readings into calibrated physical units such as temperature, pressure, or distance.

Given a set of calibration samples, CLUTGen fits an interpolation curve and generates a
production-ready ``.c``/``.h`` pair with the full LUT precomputed for every possible ADC reading.
At runtime, conversion is a single array index operation, so you can trade costly RAM operations
from ``math.h`` and other similar libraries for a constant and predictable ROM cost.

This Zephyr module integrates CLUTGen directly into the west build system. LUT generation runs
at CMake configure time and the generated files are automatically linked into the application.


Usage with Zephyr
*****************

Declare the module in your workspace manifest, or pull it in via a submanifest.

For example, create ``zephyrproject/zephyr/submanifests/clutgen.yaml`` with the following content:

.. code-block:: yaml

   manifest:
     projects:
       - name: clutgen
         url: https://github.com/wkhadgar/clutgen-zephyr
         revision: zephyr
         path: modules/clutgen
         submodules: true

Then update the workspace and install Python dependencies into the west venv:

.. code-block:: sh

   west update
   west packages pip --install


Reference
*********

- `CLUTGen Zephyr module <clutgen-zephyr_>`_
- `CLUTGen CLI tool <clutgen-cli_>`_


.. _clutgen-zephyr: https://github.com/wkhadgar/clutgen-zephyr

.. _clutgen-cli: https://github.com/wkhadgar/clutgen
