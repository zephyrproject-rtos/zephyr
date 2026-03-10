.. _doxygen_style:

Doxygen Style Guidelines
########################

The Zephyr Project uses `Doxygen`_ to generate API documentation from source code comments. This
guide defines the conventions for documenting Zephyr public APIs consistently.

All the `Doxygen commands`_ are available for use even if they don't explicitly appear in this
document.

.. _Doxygen: https://www.doxygen.nl/
.. _Doxygen commands: https://www.doxygen.nl/manual/commands.html

To build and preview Zephyr's Doxygen documentation locally, refer to :ref:`zephyr_doc`.

General Rules
*************

All :term:`public header files and their public API symbols <public API>` (functions, structs,
enums, unions, typedefs, macros, and global variables):

- Must be fully documented (see :ref:`doxygen_internals` for exceptions)
- Must belong to at least one Doxygen group (see :ref:`doxygen_groups`)

You must use the following syntax when documenting public APIs:

- Use ``/**`` to start a block comment and ``*/`` to end it.
- Use ``/**<`` for trailing comments on the same line as a symbol.
- Use ``@`` (not ``\``) for Doxygen commands (e.g., ``@param`` not ``\param``).
- The ``@brief`` command is optional. If not used, the first sentence (ending with a period) is
  treated as the brief description.

Constructs meant only for internal use must be hidden from the public documentation, as described
in :ref:`doxygen_internals`.

What to document
================

For any API element that accepts, returns, or stores a value (function parameters, return values,
struct/union members, enum values, typedefs, and macros), their documentation must describe the
following items when applicable:

Semantics
  What this element represents and how callers/users should interpret it. Don't restate the
  identifier or the C type.

  Example:

  - Avoid: ``@param timeout Timeout value in ms.``
  - Prefer: ``@param timeout Maximum time to wait before returning, in milliseconds.``

Valid values
  What values are accepted, and how they are interpreted. Specify one or more of the following:

  - Range: minimum/maximum values (for example, the type is ``uint8_t`` but only 0–100 are valid).
  - Discrete set: permitted values, when only a subset is allowed.
  - Enum: whether the value must be a valid member of a given enum (and whether all enumerators are
    allowed or only a subset).
  - Flags/bitmasks: which bits are valid and whether combinations are allowed.
  - Nullability: whether ``NULL`` is allowed for pointer values, and what it means.

Units
  When the value represents a quantity, specify the unit and reference (for example, milliseconds vs
  ticks, Hertz, bytes). Use SI unit symbols when applicable, and write a space between the number
  and the unit symbol (for example, ``10 ms``).

Representation
  Any non-obvious encoding or scaling (for example, fixed-point scale, Q format, split integer and
  fractional fields, endianness requirements).

Ownership and lifetime
  For pointers/buffers, state who allocates/frees and how long the memory must remain valid. Note
  required size/alignment when relevant.

Writing style
=============

Brief descriptions should use the imperative mood (a verb phrase) rather than third-person prose.
This keeps API summaries consistent and scannable.

- Avoid:  "Transmits data through a pipe.", "This function gets the device state.".
- Prefer: "Transmit data through a pipe.", "Get the device state.", "Initialize the subsystem.".

Parameter and member descriptions may be sentence fragments, but they should remain descriptive and
avoid repeating the parameter/member name.


.. _doxygen_groups:

Groups
******

Groups organize related symbols into a hierarchy that helps users navigate the API.

- Define each group once with ``@defgroup``.

  - The provided title should be a short, descriptive name for the group.
    As groups inherently collapse various interfaces/API together, do *not* use "API" or "Interface"
    (or any other variant of these) terms in the title, as this would be redundant.
  - The brief description of the group should not paraphrase its title.

- Group names use `snake_case <https://en.wikipedia.org/wiki/Snake_case>`_.
- Use ``@ingroup`` to specify the parent group(s) a group belongs to.

Example:

.. code-block:: c
   :emphasize-lines: 2,3

   /**
    * @defgroup mqtt_socket MQTT Client library
    * @ingroup networking
    * @since 1.14
    * @version 0.8.0
    * @{
    */

    /* documented contents of the MQTT header file */

    /** @} */

.. note::

   A group may belong to multiple parent groups, so you may have multiple ``@ingroup`` commands.
   For example, device driver emulators typically appear both in "Emulator Interfaces" and "Device
   drivers" groups. See :c:group:`i2c_emul_interface` for an example.

.. _doxygen_api_versioning:

API Versioning
==============

Use ``@since`` and ``@version`` on group definitions (``@defgroup``) to document API history
and maturity:

- ``@since`` indicates the Zephyr release when the API was introduced (e.g., ``@since 3.7``)
- ``@version`` indicates the current API version, following semantic versioning

The version number reflects the API's maturity as defined in :ref:`api_overview`.

Example:

.. code-block:: c
   :caption: Example for an API introduced in Zephyr 1.14, with current version 0.8.0 (unstable).
   :emphasize-lines: 4,5

   /**
    * @defgroup mqtt_socket MQTT Client library
    * @ingroup networking
    * @since 1.14
    * @version 0.8.0
    * @{
    */

    /* documented contents of the MQTT header file */

    /** @} */

Files
*****

Each public header file must have an ``@file`` block at the top, appearing after the SPDX license
and copyright notice.

The ``@file`` block must also belong to a Doxygen group, typically the one defined in the same
header file. This allows users browsing by file to easily navigate to the associated group.

.. code-block:: c
   :emphasize-lines: 2,4

   /**
    * @file
    * @brief Public API for the GPIO driver.
    * @ingroup gpio_interface
    */

Type Definitions
****************

Document ``struct``, ``enum``, ``union``, and ``typedef`` definitions with a brief description.

Document each member (struct field, enum value, etc.) as well. It is recommended to use ``/**<``
trailing comments style when only a brief description is provided and it can fit on a single line.

.. code-block:: c
   :caption: Examples of fully documented enum, struct, and typedef.

   /** @brief Parity modes */
   enum uart_config_parity {
        UART_CFG_PARITY_NONE,   /**< No parity */
        UART_CFG_PARITY_ODD,    /**< Odd parity */
        UART_CFG_PARITY_EVEN,   /**< Even parity */
        UART_CFG_PARITY_MARK,   /**< Mark parity */
        UART_CFG_PARITY_SPACE,  /**< Space parity */
   };

   /**
    * GPIO pin configuration.
    *
    * Specifies direction, pull resistor, and interrupt settings.
    */
   struct gpio_config {
       uint32_t flags;    /**< Pin configuration flags. */
       gpio_pin_t pin;    /**< Pin number within the port. */
   };

   /**
    * @brief Identifies a set of pins associated with a port.
    *
    * The pin with index n is present in the set if and only if the bit
    * identified by (1U << n) is set.
    */
   typedef uint32_t gpio_port_pins_t;

Functions
*********

Parameters
==========

- Document parameters with ``@param`` in declaration order
- Use ``@param[out]`` for pointers the function writes to.
- Use ``@param[in,out]`` for pointers the function both reads and writes.
- You may omit direction specifiers for const pointers and scalars (implicitly input-only).

Return Values
=============

- Use ``@return <description>`` for general descriptions (e.g., boolean or computed values).
- Use ``@retval <value> <description>`` for specific, discrete return values (typically error
  codes), starting with the success case (when applicable). The ``<value>`` must be provided as the
  first word, followed by the description (e.g., ``@retval -EINVAL Invalid arguments``, not
  ``@retval -EINVAL if invalid arguments``).

  .. note::

     Since there might be multiple reasons for the same discrete value to be returned, it is
     allowed to use the same value in multiple ``@retval`` statements.

Example:

.. code-block:: c
   :caption: Examples of fully documented functions.

   /**
    * @brief Transmit data through a pipe
    *
    * @param[in] pipe Pipe to transmit through
    * @param buf Data to transmit
    * @param size Number of bytes to transmit
    *
    * @return Number of bytes placed in @p pipe
    * @retval -EPERM Pipe is closed
    * @retval -errno Negative error code on error
    */
   int modem_pipe_transmit(struct modem_pipe *pipe, const uint8_t *buf, size_t size);

   /**
    * @brief Helper function for converting struct sensor_value to float.
    *
    * @param val A pointer to a sensor_value struct.
    * @return The converted value.
    */
   static inline float sensor_value_to_float(const struct sensor_value *val)
   {
       return (float)val->val1 + (float)val->val2 / 1000000;
   }


Macros
******

For function-like macros, document parameters like you would for functions.

.. code-block:: c
   :caption: Example of a fully documented function-like macro.

   /**
    * @brief Get a node's (only) register block size
    *
    * Equivalent to DT_REG_SIZE_BY_IDX(node_id, 0).
    *
    * @param node_id node identifier
    * @return node's only register block's size
    */
   #define DT_REG_SIZE(node_id) DT_REG_SIZE_BY_IDX(node_id, 0)

.. _doxygen_internals:

Hiding internal details
***********************

Use ``@cond INTERNAL_HIDDEN`` / ``@endcond`` to hide internal details from generated documentation.

You may still optionally document these internal symbols to provide a useful reference to internal
users.

.. code-block:: c
   :emphasize-lines: 7,11

   /** Timer structure.
    *
    * Opaque type for a timer object. All the fields in this structure are internal and should not
    * be accessed outside of kernel code.
    */
   struct k_timer {
       /** @cond INTERNAL_HIDDEN */

       /* ... internal members ... */

       /** @endcond */
   };

.. _doxygen_conditional_code:

Conditional code
****************

To ensure conditionally-compiled code appears in documentation, use either of the following
approaches:

1. Add the Kconfig macro (``CONFIG_...``) that is gating the code to the ``PREDEFINED`` list in
   :zephyr_file:`doc/zephyr.doxyfile.in`. This makes the code visible to Doxygen during
   documentation generation.

#. Alternatively, you can rely on the ``__DOXYGEN__`` macro being set to make the conditional code
   visible to Doxygen, as this macro is automatically defined by Doxygen.

.. code-block:: c
   :emphasize-lines: 3,9

   struct coap_packet {
       uint8_t *data;  /**< User allocated buffer. */
   #if defined(CONFIG_COAP_KEEP_USER_DATA) || defined(__DOXYGEN__)
       /**
        * Application-specific user data.
        * @kconfig_dep{CONFIG_COAP_KEEP_USER_DATA}
        */
       void *user_data;
   #endif
   };

.. _doxygen_kconfig_dep:

Kconfig Dependencies
====================

The ``@kconfig_dep`` command can be used to document the Kconfig option(s) that are required in
order for an API symbol to be available to the user. The command can be used with one, two or three
Kconfig options.

For example, adding ``@kconfig_dep{CONFIG_PM,CONFIG_SMP}`` to the Doxygen documentation of an API
symbol will cause the generated documentation to include a note reading: "Available only when the
following Kconfig options are enabled: ``CONFIG_PM``, ``CONFIG_SMP``.".

You can see an example of ``@kconfig_dep`` usage in the documentation of :c:struct:`coap_packet`.
