.. _snippet-debug:

Debug build Snippet (debug)
#################################

Overview
********

This snippet enables most common debugging features in Zephyr, including:
- :kconfig:option:`CONFIG_DEBUG_OPTIMIZATIONS`
- :kconfig:option:`CONFIG_DEBUG_THREAD_INFO`

This snippet does not change the default logging level, as it might increase the size of the binary significantly.
