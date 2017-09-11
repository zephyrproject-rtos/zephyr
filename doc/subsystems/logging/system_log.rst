.. _system_log:

System Logging
##############

The system log API provides a common interface to process messages issued by
developers. These messages are currently printed on the terminal but the API is
defined in a generic way.

This API can be deactivated through the Kconfig options, see
:ref:`global_kconfig`.
This approach prevents impacting image size and execution time when the system
log is not needed.

Each of the four ``SYS_LOG_X`` macros correspond to a different logging level,
The logging macros activate when their logging level or higher is set.

There are two configuration categories: configurations per module and global
configurations. When logging is enabled globally, it works for modules. However,
modules can disable logging locally. Every module can specify its own logging
level. The module must define the :c:macro:`SYS_LOG_LEVEL` macro before
including the :file:`include/logging/sys_log.h` header file to do so. Unless a global
override is set, the module logging level will be honored. The global override
can only increase the logging level. It cannot be used to lower module logging
levels that were previously set higher.

You can set a local domain to differentiate messages. When no domain is set,
then the ``[general]`` domain appears before the message. Define the
:c:macro:`SYS_LOG_DOMAIN` macro before including the :file:`include/logging/sys_log.h`
header file to set the domain.

When several macros are active, the printed messages can be differentiated in
two ways: by a tag printed before the message or by ANSI colors. See the
:option:`CONFIG_SYS_LOG_SHOW_TAGS` and :option:`CONFIG_SYS_LOG_SHOW_COLOR`
Kconfig options for more information.

Define the :c:macro:`SYS_LOG_NO_NEWLINE` macro before including the
:file:`include/logging/sys_log.h` header file to prevent macros appending a new line at the
end of the logging message.

.. _global_kconfig:

Global Kconfig Options
**********************

These options can be found in the following path :file:`subsys/logging/Kconfig`.

:option:`CONFIG_SYS_LOG`: Global switch, turns on/off all system logging.

:option:`CONFIG_SYS_LOG_DEFAULT_LEVEL`: Default level, sets the logging level
used by modules that are not setting their own logging level.

:option:`CONFIG_SYS_LOG_SHOW_TAGS`: Globally sets whether level tags will be
shown on log or not.

:option:`CONFIG_SYS_LOG_SHOW_COLOR`: Globally sets whether ANSI colors will be
used by the system log.

:option:`CONFIG_SYS_LOG_OVERRIDE_LEVEL`: It overrides module logging level when
it is not set or set lower than the override value.

Example
*******

The following macro:

    .. code-block:: c

     SYS_LOG_WRN("hi!");

Will produce:

    .. code-block:: console

     [general] [WRN] main: Hi!

For the above example to work at least one of the following settings must be
true:

- The :option:`CONFIG_SYS_LOG_DEFAULT_LEVEL` is set to 2 or above and module
  configuration is not set.
- The module configuration is set to 2 or above.
- The :option:`CONFIG_SYS_LOG_OVERRIDE_LEVEL` is set to 2 or above.


APIs
****

.. doxygengroup:: system_log
   :project: Zephyr
