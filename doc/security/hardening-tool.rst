.. _hardening:

Hardening Tool
##############

Zephyr contains several optional features that make the overall system
more secure. As we take advantage of hardware features, many of these
options are platform specific and besides it, some of them are unknown
by developers.

To address this problem, Zephyr provides a tool that helps to check an
application configuration option list against a list of hardening
preferences defined by the **Security Group**. The tool can identify the build
target and based on that provides suggestions and recommendations on how to
optimize the configuration for security.

Usage
*****

After configure of your application, change directory to the build folder and:

.. code-block:: console

   # ninja build system:
   $ ninja hardenconfig
   # make build system:
   $ make hardenconfig

The output should be similar to the one bellow:

.. code-block:: console


                          name                       |   current   |    recommended     ||        check result
   ===================================================================================================================
   CONFIG_HW_STACK_PROTECTION                        |      n      |         y          ||            FAIL
   CONFIG_BOOT_BANNER                                |      y      |         n          ||            FAIL
   CONFIG_PRINTK                                     |      y      |         n          ||            FAIL
   CONFIG_EARLY_CONSOLE                              |      y      |         n          ||            FAIL
   CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT             |      n      |         y          ||            FAIL
   CONFIG_DEBUG_INFO                                 |      y      |         n          ||            FAIL
   CONFIG_TEST_RANDOM_GENERATOR                      |      y      |         n          ||            FAIL
   CONFIG_BUILD_OUTPUT_STRIPPED                      |      n      |         y          ||            FAIL
   CONFIG_STACK_SENTINEL                             |      n      |         y          ||            FAIL
