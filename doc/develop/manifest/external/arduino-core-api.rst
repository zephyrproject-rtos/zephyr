.. _external_module_arduino_core_api:

Arduino Core API
###############

Introduction
***********

The `Arduino Core API`_ module for Zephyr started as a Google Summer of Code 2022 project
to provide Arduino-style APIs for Zephyr RTOS applications. This module acts as an abstraction
layer, allowing developers familiar with Arduino programming to leverage Zephyr's capabilities
without having to learn entirely new APIs and libraries.

The Arduino Core API module provides:

* Standard Arduino API functions like ``pinMode()``, ``digitalWrite()``, ``analogRead()``, etc.
* Support for Arduino-style ``setup()`` and ``loop()`` functions
* Pin mapping between Arduino pin numbers and Zephyr's GPIO definitions
* Support for common Arduino communication protocols (Serial, I2C, SPI)
* Compatibility with existing Arduino libraries
* Board variant support for various hardware platforms that are already present in zephyr.

By bringing Arduino-style programming to Zephyr, this module provides a gentler learning curve
for those transitioning from Arduino to Zephyr while still benefiting from Zephyr's advanced
features, scalability, and broad hardware support.

Usage with Zephyr
****************

Adding the Arduino Core API to a Zephyr Project
==============================================

1. To include the Arduino Core API in your Zephyr project, add the following
   entry to the ``manifest/projects`` subtree in your ``west.yml`` file:

   .. code-block:: yaml

      # Arduino API repository
      - name: Arduino-Core-Zephyr
        path: modules/lib/Arduino-Zephyr-API
        revision: main
        url: https://github.com/zephyrproject-rtos/arduino-core-zephyr

2. Run the following command to update your project:

   .. code-block:: bash

      west update

3. For Linux users, there's an ``install.sh`` script in the module that will automatically
   link the ArduinoCore-API. If you can't use this script, follow the manual steps below.

4. Complete the core setup by linking the API folder from the ArduinoCore-API repository:

   .. code-block:: bash

      git clone https://github.com/arduino/ArduinoCore-API # Any location
      ln -s /<your>/<location>/ArduinoCore-API/api cores/arduino/.

   The ``cores`` folder can be found at ``<zephyr-project-path>/modules/lib/Arduino-Zephyr-API/cores``.

Using Arduino Core API in Your Application
========================================

1. In your application's ``prj.conf`` file, enable the Arduino API:

   .. code-block:: text

      CONFIG_GPIO=y
      CONFIG_ARDUINO_API=y

2. Create your application using Arduino-style code:

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

3. Build your application with the target board:

   .. code-block:: bash

      west build -b <board_name> path/to/your/app

Supported Boards
==============

The Arduino Core API module currently has variants for these boards:
* Arduino Nano BLE Sense 33
* Arduino Nano BLE 33
* Arduino Nano 33 IoT
* Beagleconnect Freedom
* Raspberry Pi Pico (RP2040)

Adding Custom Board Support
========================

To add support for a custom board:

1. Create a new folder in the ``variants/`` directory with your board's name
2. Add an overlay file and a pinmap header file that match the board name
3. Add your new headerfile to an ``#ifdef`` statement in the ``variant.h`` file

For detailed instructions on adding board variants, refer to the `board variants documentation`_.

Using External Arduino Libraries
============================

To use external Arduino libraries with your Zephyr project:

1. Add your library's source files (e.g., ``MyLibrary.h`` and ``MyLibrary.cpp``) to your project's ``src`` folder
2. Update your application's ``CMakeLists.txt`` to include these files:

   .. code-block:: cmake

      target_sources(app PRIVATE src/MyLibrary.cpp)

3. Include the library in your source code:

   .. code-block:: cpp

      #include "MyLibrary.h"

For more details on using external libraries, see the `Arduino libraries documentation`_.

References
*********

* `Arduino Core API GitHub Repository`_
* `Arduino Core API Project Blog`_
* `Golioth Article: Zephyr + Arduino: a Google Summer of Code story`_
* `ArduinoCore-API Repository`_

.. target-notes::

.. _Arduino Core API: https://github.com/zephyrproject-rtos/gsoc-2022-arduino-core
.. _board variants documentation: https://github.com/zephyrproject-rtos/gsoc-2022-arduino-core/blob/main/documentation/variants.md
.. _Arduino libraries documentation: https://github.com/zephyrproject-rtos/gsoc-2022-arduino-core/blob/main/documentation/arduino_libs.md
.. _Arduino Core API GitHub Repository: https://github.com/zephyrproject-rtos/gsoc-2022-arduino-core
.. _Arduino Core API Project Blog: https://dhruvag2000.github.io/Blog-GSoC22/
.. _Golioth Article\: Zephyr + Arduino\: a Google Summer of Code story: https://blog.golioth.io/zephyr-arduino-a-google-summer-of-code-story/
.. _ArduinoCore-API Repository: https://github.com/arduino/ArduinoCore-API
