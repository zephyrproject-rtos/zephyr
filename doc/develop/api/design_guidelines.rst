.. _design_guidelines:

API Design Guidelines
#####################

Zephyr development and evolution is a group effort, and to simplify
maintenance and enhancements there are some general policies that should
be followed when developing a new capability or interface.

.. _doxygen_documentation_guidelines:

API Documentation
*****************

The Zephyr Project uses `Doxygen`_ to generate documentation from source code comments. A high-level
summary of the Doxygen guidelines for all public API is as follows:

- Each public header file, function declaration, type definition, typedef, define, macro and global
  variable declaration SHALL be fully documented.

- Each public header file, function declaration, type definition, typedef, define, macro and global
  variable declaration SHALL belong to at least one Doxygen group. See
  :ref:`doxygen_guidelines_groups` for more details.

- Each public header file SHALL have an ``@file`` block at the top of the file after the SPDX
  License Identifier. See :ref:`doxygen_guidelines_files` for more details.

- Each construct that is defined in a public header file but only meant for internal use SHALL be
  enclosed in ``@cond INTERNAL_HIDDEN`` / ``@endcond`` sections.
  See :ref:`doxygen_guidelines_internals` for more details.

- All documentation SHALL use grammatically correct sentences with proper punctuation.

.. _Doxygen: https://www.doxygen.nl/
.. _Doxygen commands: https://www.doxygen.nl/manual/commands.html

General Doxygen considerations
==============================

- Using the ``@brief`` command is optional. If omitted, the first sentence of the comment block
  (ending with a period) is treated as the brief description.

.. _doxygen_guidelines_groups:

Groups
======

- Doxygen group names shall use `snake case <https://en.wikipedia.org/wiki/Snake_case>`_.

- Use ``@defgroup`` and ``@addtogroup`` with ``@{`` and ``@}`` brackets to add members to a group.

- A group shall be defined at most once.

- Each group shall be documented with a title, brief description and an optional detailed
  description.

  - The title is a short, descriptive name for the group. As groups inherently collapse various
    interfaces/API together, do _not_ use "API" or "Interface" (or any other variant of these) terms
    in the title.
  - The brief description is a short, one-sentence description of the group that should not
    paraphrase the title.
  - The detailed description is a longer, more detailed description of the group.

- A group must be contained in at least one group, so that it correctly appears in the documentation
  hierarchy. Use the ``@ingroup`` command to specify the parent group(s) a group belongs to.

  .. note::

     There are situations where it makes sense for a group to be contained in multiple groups. For
     example, Zephyr device driver emulators appear both in the ``io_emulators`` group (in the
     Testing section of the documentation), as well as in the group corresponding to the emulated
     driver class (e.g. ``adc_interface``).

Example:

.. code-block:: c

   /**
    * @defgroup lora_api LoRa
    * @brief Set of APIs for interacting with LoRa radio.
    * @ingroup drivers
    *
    * @{
    */

    /* here the fully documented contents of the LoRa header file */

    /** @} */


.. _doxygen_guidelines_files:

Files
=====

- ``@file`` blocks shall have a brief description and an optional detailed description.

  - The brief description should concisely state what the file provides, e.g.:

    - This header file provides the API of the ABC subsystem.
    - This file defines helper macros for the XYZ module.

  - The optional detailed description should explain the context or grouping logic of the file
    contents.

- Avoid repeating content that should be in group or individual symbol documentation.

Type Definitions
================

- Each type (``typedef``, ``struct``, ``enum``) and type member shall be documented with a brief
  description and an optional detailed description.

- For the brief description of types, use phrases like this:

  - This type represents ... and so on.
  - This structure represents ... and so on.
  - This structure provides ... and so on.
  - This enumeration represents ... and so on.
  - The XYZ represents ... and so on.

- For the brief description of type members, use phrases like this:

  - This member represents ... and so on.
  - This member contains ... and so on.
  - This member references ... and so on.
  - The XYZ lock protects ... and so on.

Function Declarations
=====================

- Each function declaration or function-like macro in a header file shall be documented with a brief
  description and an optional detailed description.

- For the brief description, use descriptive-style, for example "Creates a thread." or "Sends the
  events to the thread." or "Obtains the semaphore.".

Parameters
----------

- Each parameter shall be documented with an ``@param`` entry. List the ``@param`` entries in the
  order of the function parameters.

- For non-``const`` pointer parameters:

  - Use ``@param[out]``, if the function *writes* to the pointed data.
  - Use ``@param[in, out]``, if the function both *reads* and *writes* to it.

- Do not use ``[in]``, ``[out]`` or ``[in, out]`` specifiers for const pointers or scalars.

- Parameter descriptions should clarify unit, constraints, and purpose.

Return Values
-------------

- Return values shall be documented with ``@retval`` (for distinctive values) and ``@return``
  (for non-distinctive values) paragraphs.

- Place ``@retval`` descriptions before the ``@return`` description, starting with the most common
  return value.

- For functions returning a boolean value, use ``@return`` and a phrase like this: "Returns true,
  if some condition is satisfied, otherwise false."

.. _doxygen_guidelines_internals:

Hiding internals
================

- Use ``@cond INTERNAL_HIDDEN`` / ``@endcond`` sections to hide internal details from the generated
  documentation.

- It is good practice to still document internal symbols for developer understanding.

Example:

.. code-block:: c

   /**
    * @brief This structure represents a foo.
    *
    * Opaque structure that holds the state of a foo.
    */
    struct foo {
      /**
       * @cond INTERNAL_HIDDEN
       */
      int bar;
      /**
       * @endcond
       */
    };

Zephyr-specific Doxygen commands
================================

Indicating Kconfig dependencies
-------------------------------

Some APIs might be available to the user only when one or more Kconfig options are enabled. The
``@kconfig_dep`` command can be used to convey this information to the user in a consistent way.

The command can be used with one, two or three Kconfig options.
For example, :samp:`@kconfig_dep{CONFIG_FOO,CONFIG_BAR}` will automatically add a note to the
generated documentation stating "Available only when the following Kconfig options are enabled:
``CONFIG_FOO``, ``CONFIG_BAR``.".

You can check an example of ``@kconfig_dep`` usage in the documentation for
:c:func:`bt_cap_handover_unicast_to_broadcast()`.

Using Callbacks
***************

Many APIs involve passing a callback as a parameter or as a member of a
configuration structure.  The following policies should be followed when
specifying the signature of a callback:

* The first parameter should be a pointer to the object most closely
  associated with the callback.  In the case of device drivers this
  would be ``const struct device *dev``.  For library functions it may be a
  pointer to another object that was referenced when the callback was
  provided.

* The next parameter(s) should be additional information specific to the
  callback invocation, such as a channel identifier, new status value,
  and/or a message pointer followed by the message length.

* The final parameter should be a ``void *user_data`` pointer carrying
  context that allows a shared callback function to locate additional
  material necessary to process the callback.

An exception to providing ``user_data`` as the last parameter may be
allowed when the callback itself was provided through a structure that
will be embedded in another structure.  An example of such a case is
:c:struct:`gpio_callback`, normally defined within a data structure
specific to the code that also defines the callback function.  In those
cases further context can accessed by the callback indirectly by
:c:macro:`CONTAINER_OF`.

Examples
========

* The requirements of :c:type:`k_timer_expiry_t` invoked when a system
  timer alarm fires are satisfied by::

    void handle_timeout(struct k_timer *timer)
    { ... }

  The assumption here, as with :c:struct:`gpio_callback`, is that the
  timer is embedded in a structure reachable from
  :c:macro:`CONTAINER_OF` that can provide additional context to the
  callback.

* The requirements of :c:type:`counter_alarm_callback_t` invoked when a
  counter device alarm fires are satisfied by::

    void handle_alarm(const struct device *dev,
                      uint8_t chan_id,
		      uint32_t ticks,
		      void *user_data)
    { ... }

  This provides more complete useful information, including which
  counter channel timed-out and the counter value at which the timeout
  occurred, as well as user context which may or may not be the
  :c:struct:`counter_alarm_cfg` used to register the callback, depending
  on user needs.

Conditional Data and APIs
*************************

APIs and libraries may provide features that are expensive in RAM or
code size but are optional in the sense that some applications can be
implemented without them.  Examples of such feature include
:kconfig:option:`capturing a timestamp <CONFIG_CAN_RX_TIMESTAMP>` or
:kconfig:option:`providing an alternative interface <CONFIG_SPI_ASYNC>`.  The
developer in coordination with the community must determine whether
enabling the features is to be controllable through a Kconfig option.

In the case where a feature is determined to be optional the following
practices should be followed.

* Any data that is accessed only when the feature is enabled should be
  conditionally included via ``#ifdef CONFIG_MYFEATURE`` in the
  structure or union declaration.  This reduces memory use for
  applications that don't need the capability.
* Function declarations that are available only when the option is
  enabled should be provided unconditionally.  Add a note in the
  description that the function is available only when the specified
  feature is enabled, referencing the required Kconfig symbol by name.
  In the cases where the function is used but not enabled the definition
  of the function shall be excluded from compilation, so references to
  the unsupported API will result in a link-time error.
* Where code specific to the feature is isolated in a source file that
  has no other content that file should be conditionally included in
  ``CMakeLists.txt``::

    zephyr_sources_ifdef(CONFIG_MYFEATURE foo_funcs.c)
* Where code specific to the feature is part of a source file that has
  other content the feature-specific code should be conditionally
  processed using ``#ifdef CONFIG_MYFEATURE``.

The Kconfig flag used to enable the feature should be added to the
``PREDEFINED`` variable in :file:`doc/zephyr.doxyfile.in` to ensure the
conditional API and functions appear in generated documentation.

Return Codes
************

Implementations of an API, for example an API for accessing a peripheral might
implement only a subset of the functions that is required for minimal operation.
A distinction is needed between APIs that are not supported and those that are
not implemented or optional:

- APIs that are supported but not implemented shall return ``-ENOSYS``.

- Optional APIs that are not supported by the hardware should be implemented and
  the return code in this case shall be ``-ENOTSUP``.

- When an API is implemented, but the particular combination of options
  requested in the call cannot be satisfied by the implementation the call shall
  return ``-ENOTSUP``. (For example, a request for a level-triggered GPIO interrupt on
  hardware that supports only edge-triggered interrupts)
