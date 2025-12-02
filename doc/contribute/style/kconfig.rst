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

.. literalinclude:: kconfig_demo_simple.txt
   :language: kconfig
   :start-after: start-after-here

.. literalinclude:: kconfig_demo_complex.txt
   :language: kconfig
   :start-after: start-after-here


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
  (e.g. *Driver Type* -> *Driver Name*).

* The prompt for an enabling symbol should use the same logic as the symbol
  name itself but use the keywords in reversed order.

   * Adhering to this style makes searching for symbols easier in the UI
     because one can filter by a scope keyword.

* When the enabling symbol is dependent on a devicetree node, consider
  depending on the automatically created ``DT_HAS_<node>_ENABLED`` symbol.

The specific formats by subtree:

* **Drivers (/drivers)**: Use the format ``{Driver Type}_{Driver Name}`` for
  symbols and ``{Driver Name} {Driver Type} driver`` for prompts.

* **Sensors (/drivers/sensors)**: Use the format ``SENSOR_{Sensor Name}`` for symbols
  and ``{Sensor Name} {Sensor Type} sensor driver`` for prompts.

* **Architecture (/arch)**: Many symbols are shared across architectures. Before
  creating new symbols, check if similar ones already exist in other
  architectures.

Examples
========

.. note::

   The following examples only show the symbol and prompt lines for brevity.

**Driver Examples:**

.. literalinclude:: kconfig_example_driver.txt
   :language: kconfig
   :start-after: start-after-here

**Sensor Examples:**

.. literalinclude:: kconfig_example_sensor.txt
   :language: kconfig
   :start-after: start-after-here

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
