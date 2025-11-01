.. _external_module_arduino_core_api:

Arduino Core API
################

Introduction
************

The Arduino-Core-Zephyr module started as a `Google Summer of Code 2022 project`_
to provide Arduino-style APIs for Zephyr RTOS applications. This module acts as an abstraction
layer, allowing developers familiar with Arduino programming to leverage Zephyr's capabilities
without having to learn entirely new APIs and libraries.

Understanding the Components
============================

This module consists of two key components that work together:

**1. ArduinoCore-API (Common Arduino API Definition)**

The `ArduinoCore-API <https://github.com/arduino/ArduinoCore-API>`_ is Arduino's official
hardware abstraction layer that defines the common Arduino API. It contains the abstract API
definitions **and** implementations for hardware-independent functionality.

Key characteristics:

* Contains both API definitions (headers) and hardware-agnostic implementations
* Provides classes like ``String``, ``Print``, ``Stream``, ``IPAddress`` with full implementations
* Defines interfaces for hardware-specific classes (e.g., ``HardwareSerial``, ``HardwareSPI``)
* Shared across all modern Arduino platforms for consistency
* See the `ArduinoCore-API README <https://github.com/arduino/ArduinoCore-API#arduinocore-api>`_ for implementation details

**2. Arduino-Core-Zephyr (Zephyr-Specific Implementation)**

The `Arduino-Core-Zephyr <https://github.com/zephyrproject-rtos/arduino-core-zephyr>`_ module
provides the **Zephyr-specific implementation** of the Arduino API. This is where hardware-dependent
Arduino functions are implemented using Zephyr's native APIs and drivers.

Key characteristics:

* Contains the ``cores/arduino`` directory with Zephyr implementations (``zephyrCommon.cpp``, ``zephyrSerial.cpp``, etc.)
* Provides board-specific variants with pin mappings and Device Tree overlays
* Includes Zephyr build system integration (CMake, Kconfig, west.yml)
* Links to ArduinoCore-API to inherit the common implementations
* See the `project documentation <https://github.com/zephyrproject-rtos/arduino-core-zephyr/tree/main/documentation>`_ for implementation details

Features Provided
=================

Together, these components provide:

* Standard Arduino API functions like ``pinMode()``, ``digitalWrite()``, ``analogRead()``, etc.
* Support for Arduino-style ``setup()`` and ``loop()`` functions
* Pin mapping between Arduino pin numbers and Zephyr's GPIO definitions
* Support for common Arduino communication protocols (Serial, I2C, SPI)
* Compatibility with existing Arduino libraries
* Board variant support for various hardware platforms already present in Zephyr

By bringing Arduino-style programming to Zephyr, this module provides a gentler learning curve
for those transitioning from Arduino to Zephyr while still benefiting from Zephyr's advanced
features, scalability, and broad hardware support.

Usage with Zephyr
*****************

Adding the Arduino Core API to a Zephyr Project
===============================================

#. To pull in the Arduino Core for Zephyr as a Zephyr module, either add it as
   a West project in the west.yaml file or pull it in by adding a submanifest
   (e.g. ``zephyr/submanifests/arduino-core.yaml``) file with the following content
   and run west update:

   .. code-block:: yaml

      # Arduino API repository
      - name: Arduino-Core-Zephyr
        path: modules/lib/arduinocore-zephyr
        revision: main
        url: https://github.com/zephyrproject-rtos/arduino-core-zephyr

#. Run the following command to update your project:

   .. code-block:: bash

      west update

#. For Linux users, there's an ``install.sh`` script in the module that will automatically
   link the ArduinoCore-API. If you can't use this script, follow the manual steps below.

#. Complete the core setup by linking the API folder from the ArduinoCore-API repository into
   the arduinocore-zephyr folder:

   .. code-block:: bash

      git clone https://github.com/arduino/ArduinoCore-API # Any location
      cd /<path>/<to>/<zephyrproject>/modules/lib/arduinocore-zephyr
      ln -s /<your>/<location>/arduinocore-zephyr/api cores/arduino/.

   The ``cores`` folder is located inside ``<zephyr-project-path>/modules/lib/arduinocore-zephyr/cores``.

Using Arduino Core API in Your Application
==========================================

#. In your application's ``prj.conf`` file, enable the Arduino API:

   .. code-block:: cfg

      CONFIG_GPIO=y
      CONFIG_ARDUINO_API=y

#. Create your application using Arduino-style code:

   .. code-block:: cpp

      #include <Arduino.h>

      void setup() {
        pinMode(LED_BUILTIN, OUTPUT);
      }

      void loop() {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
      }

#. Build your application with the target board:

   .. code-block:: bash

      west build -b <board_name> path/to/your/app

Supported Boards
================

The Arduino Core API module currently has variants for these boards:

* :zephyr:board:`arduino_mkrzero`
* :zephyr:board:`arduino_nano_33_ble` (including Sense variant)
* :zephyr:board:`arduino_nano_33_iot`
* :zephyr:board:`beagleconnect_freedom`
* :zephyr:board:`cc3220sf_launchxl`
* :zephyr:board:`nrf52840dk`
* :zephyr:board:`nrf9160dk`
* :zephyr:board:`rpi_pico`

Adding Custom Board Support
===========================

To add support for a custom board:

#. Create a new folder in the ``variants/`` directory with your board's name
#. Add an overlay file and a pinmap header file that match the board name
#. Add your new headerfile to an ``#ifdef`` statement in the ``variant.h`` file

For detailed instructions on adding board variants, refer to the `board variants documentation`_.

Using External Arduino Libraries
================================

To use external Arduino libraries with your Zephyr project:

#. Add your library's source files (e.g., ``MyLibrary.h`` and ``MyLibrary.cpp``) to your project's ``src`` folder
#. Update your application's ``CMakeLists.txt`` to include these files:

   .. code-block:: cmake

      target_sources(app PRIVATE src/MyLibrary.cpp)

#. Include the library in your source code:

   .. code-block:: cpp

      #include "MyLibrary.h"

For more details on using external libraries, see the `Arduino libraries documentation`_.

References
**********

#. `Arduino-Core-Zephyr GitHub Repository`_
#. `ArduinoCore-API Repository`_
#. `Golioth Article: Zephyr + Arduino: a Google Summer of Code story`_

.. target-notes::

.. _Arduino Core API: https://github.com/zephyrproject-rtos/arduino-core-zephyr
.. _board variants documentation: https://github.com/zephyrproject-rtos/arduino-core-zephyr/blob/main/documentation/variants.md
.. _Arduino libraries documentation: https://github.com/zephyrproject-rtos/arduino-core-zephyr/blob/main/documentation/arduino_libs.md
.. _Arduino-Core-Zephyr GitHub Repository: https://github.com/zephyrproject-rtos/arduino-core-zephyr
.. _ArduinoCore-API Repository: https://github.com/arduino/ArduinoCore-API
.. _Google Summer of Code 2022 project: https://dhruvag2000.github.io/Blog-GSoC22/
.. _Golioth Article\: Zephyr + Arduino\: a Google Summer of Code story: https://blog.golioth.io/zephyr-arduino-a-google-summer-of-code-story/
