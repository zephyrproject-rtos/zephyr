.. _kconfig_style:

Kconfig Style Guidelines
########################

This document provides style guidelines for writing Kconfig files in the Zephyr
project. Following these guidelines ensures consistency and readability across
the codebase, making it easier for developers to understand and maintain
configuration options.

The following sections provide guidelines with examples to illustrate
proper Kconfig formatting and naming conventions.


Basic Formatting Rules
**********************

When writing Kconfig files, follow these basic formatting rules:

* **Line length**: Keep lines to 100 columns or fewer.
* **Indentation**: Use tabs for indentation, except for ``help`` entry text
  which should be placed at one tab plus two extra spaces.
* **Spacing**: Leave a single empty line between option declarations.
* **Comments**: Format comments as ``# Comment`` rather than ``#Comment``.
* **Conditional blocks**: Insert an empty line before/after each top-level
  ``if`` and ``endif`` statement.

For guidance on using statements like ``select``, see
:ref:`kconfig_tips_and_tricks` for more information.

Symbol Naming and Structure
***************************

The following examples demonstrate proper Kconfig symbol naming and structure:

.. code-block:: kconfig

   # symbol to enable/disable the feature = "enabling symbol"
   config ADC_ADI_AD405X
      bool "AD405X (ADI) ADC driver"
      depends on DT_HAS_ADI_AD4052_ADC_ENABLED || DT_HAS_ADI_AD4050_ADC_ENABLED
      select SPI
      default y
      help
        Enable ADC driver for ADI AD405X.

.. code-block:: kconfig

   # symbol to enable/disable the feature = "enabling symbol"
   menuconfig ADC_ADI_AD4114
      bool "AD4114 (ADI) ADC driver"
      default y
      depends on DT_HAS_ADI_AD4114_ADC_ENABLED
      select SPI
      help
        Enable the AD4114 ADC driver.

   if ADC_ADI_AD4114

   # symbol to configure the behavior of the feature = "config symbol"
   config ADC_ADI_AD4114_ACQUISITION_THREAD_STACK_SIZE
      int "Stack size for the data acquisition thread"
      default 512
      help
        Size of the stack used for the internal data acquisition
        thread.

   # symbol to configure the behavior of the feature = "config symbol"
   config ADC_ADI_AD4114_ACQUISITION_THREAD_PRIO
      int "Priority for the data acquisition thread"
      default 0
      help
        Priority level for the internal ADC data acquisition thread.

   endif # ADC_ADI_AD4114

Naming Conventions
******************

* As a general rule, symbols concerning the same component should be distinct
  from other symbols. This can usually be accomplished by using a common
  prefix. This prefix can be a simple keyword or, as in the case for drivers,
  several keywords for more precision.

* A common prefix usually indicates the subsystem or component the symbol
  belongs to.

* An enabling symbol name should consist of keywords to provide the context
  of the symbol in a scope from most general to most specific
  (e.g. *Driver Type* -> *Manufacturer* -> *Driver Name*).

* The prompt for an enabling symbol should use the same logic as the symbol
  name itself but use the keywords in reversed order.

   * Adhering to this style makes searching for symbols easier in the UI
     because one can filter by a scope keyword.

* When the enabling symbol is dependent on a devicetree node, consider
  depending on the automatically created ``DT_HAS_<node>_ENABLED`` symbol.

The specific formats by subtree:

* **Drivers (/drivers)**: Use the format ``{Driver Type}_{Manufacturer}_{Driver Name}`` for
  symbols and ``{Driver Name} ({Manufacturer}) {Driver Type} driver`` for prompts.

* **Sensors (/drivers/sensors)**: Use the format ``SENSOR_{Manufacturer}_{Sensor Name}`` for symbols
  and ``{Sensor Name} ({Manufacturer}) {Sensor Type} sensor driver`` for prompts.

* **Architecture (/arch)**: Many symbols are shared across architectures. Before
  creating new symbols, check if similar ones already exist in other
  architectures.

Examples
========

**Driver Examples:**

.. code-block:: kconfig

   config ADC_ADI_AD405X
      bool "AD405X (ADI) ADC driver"

   config CAN_ADI_MAX32
      bool "MAX32 (ADI) CAN driver"

   config I2C_EMUL
      bool "I2C emulator driver"

**Sensor Examples:**

.. code-block:: kconfig

   config SENSOR_BOSCH_BMP581
      bool "BMP581 (Bosch) barometric pressure sensor driver"

   config SENSOR_TI_TMP007
      bool "TMP007 (TI) infrared thermopile sensor driver"

   config SENSOR_LM75
      bool "LM75 temperature sensor driver"

.. note::
   For sensors like LM75 that have multiple manufacturers, the manufacturer
   can be omitted from the symbol name.

Configuration Symbol Organization
*********************************

When a feature uses config symbols to configure its behavior:

* Use ``menuconfig`` instead of ``config`` to define the enabling feature
  (even when the config symbol has no prompt).

* Encapsulate the config symbols in an ``if`` statement to declare their
  dependency on the enabling symbol (this automatically groups these
  symbols under the enabling symbol in the UI).

* Prefix the config symbol with the enabling symbol's name for scope and
  context.

* In the prompt of the config symbol, describe what the symbol configures
  without repeating the scope keywords, because the grouping in the UI
  provides this context.

File Organization
*****************

When organizing Kconfig files:

* Keep the Kconfig file close to the source files it configures.

* When dealing with large Kconfig files (e.g. with many config symbols),
  consider grouping (some of) them into a separate file and import it using the
  ``source`` directive to improve readability.

* Insert an empty line before/after each top-level ``if`` and ``endif``
  statement to improve readability.
