.. _sysbuild_images:

Sysbuild images
###############

Sysbuild can be used to add additional images to builds, these can be added by projects or boards
though at present must be Zephyr applications.

Methods of adding images
************************

Images can be added to project or projects in various ways, multiple ways can be used
simultaneously, they can be added in the following ways:

Applications
============

Applications can add sysbuild images using the ``sysbuild.cmake`` file in the application
directory, the inclusion of images can be controlled with a ``Kconfig.sysbuild`` file in the
application directory.

Boards
======

Boards can add sysbuild images by using the ``sysbuild.cmake`` file in the board directory, the
inclusion of images can be controlled with a ``Kconfig.sysbuild`` file in the board directory.

SoCs
====

SoCs can add sysbuild images by using the ``sysbuild.cmake`` file in the soc directory.

Modules
=======

:ref:`modules` can add sysbuild images with the ``sysbuild-cmake`` and ``sysbuild-kconfig``
options in a ``module.yml`` file, see :ref:`sysbuild_module_integration` for details.

Adding images
*************

Images can be added in one of two ways:

Single unchangeable image
=========================

With this setup, the image to be added is fixed to a specific application and cannot be changed
(although it cannot be changed to another image, the version of the image itself can be changed by
use of a west manifest bringing in a different version of the application repo or from an
alternate source, assuming that the image is in it's own repository).

.. note::

   This method should only be used if the image is locked to a specific interface and has no
   extensibility.

An example of how to create such an image, this assumes that a
:ref:`Zephyr application has been created <application>`:

.. tabs::

   .. group-tab:: ``Kconfig.sysbuild``

      .. code-block:: kconfig

         config MY_IMAGE
                 bool "Include my amazing image"
                 help
                   If enabled, will include my amazing image in the build which does...

      .. note::

         Remember to have ``source "share/sysbuild/Kconfig"`` in the file if this
         is being applied in an application ``Kconfig.sysbuild`` file.


   .. group-tab:: ``sysbuild.cmake``

      .. code-block:: cmake

         if(SB_CONFIG_MY_IMAGE)
           ExternalZephyrProject_Add(
             APPLICATION my_image
             SOURCE_DIR ${ZEPHYR_MY_IMAGE_MODULE_DIR}/path/to/my_image
           )
         endif()

Additional dependency ordering can be set here if needed, see
:ref:`sysbuild_zephyr_application_dependencies` for details, image configuration can also be set
here, see :ref:`sysbuild_images_config` for details.

This image can be enabled when building with west like so:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: <app>
   :board: nrf52840dk/nrf52840
   :goals: build
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_MY_IMAGE=y
   :compact:

Extensible changeable image
===========================

With this setup, the image to be added can be one of any number of possible applications which the
user has selection over, this list of selections can also be extended downstream to add additional
options for out-of-tree specific applications that are not available in upstream Zephyr. This is
more complex to create but is the preferred method for adding images to upstream Zephyr.

.. tabs::

   .. group-tab:: ``Kconfig.sysbuild``

      .. code-block:: kconfig

         config SUPPORT_OTHER_APP
                 bool
                 # Conditions can be placed here if this application type is only usable on certain platforms
                 default y

         config SUPPORT_OTHER_APP_MY_IMAGE
                 bool
                 # Conditions can be placed here if this image is only usable on certain platforms
                 default y

         choice OTHER_APP
                 prompt "Other app image"
                 # Defaults can be specified here if a default image should be loaded if e.g. it is supported
                 default OTHER_APP_NONE
                 depends on SUPPORT_OTHER_APP

         config OTHER_APP_IMAGE_NONE
                 bool "None"
                 help
                   Do not Include an other app image in the build.

         config OTHER_APP_IMAGE_MY_IMAGE
                 bool "my_image"
                 depends on SUPPORT_OTHER_APP_MY_IMAGE
                 help
                   Include my amazing image as the other app image to use, which does...

         endchoice

         config OTHER_APP_IMAGE_NAME
                 string
                 default "my_image" if OTHER_APP_IMAGE_MY_IMAGE
                 help
                   Name of other app image.

         config OTHER_APP_IMAGE_PATH
                 string
                 default "$(ZEPHYR_MY_IMAGE_MODULE_DIR)/path/to/my_image" if OTHER_APP_IMAGE_MY_IMAGE
                 help
                   Source directory of other app image.

      .. note::

         Remember to have ``source "$(ZEPHYR_BASE)/share/sysbuild/Kconfig"`` in the file if this
         is being applied in an application ``Kconfig.sysbuild`` file.

   .. group-tab:: ``sysbuild.cmake``

      .. code-block:: cmake

         if(SB_CONFIG_OTHER_APP_IMAGE_PATH)
           ExternalZephyrProject_Add(
             APPLICATION ${SB_CONFIG_OTHER_APP_IMAGE_NAME}
             SOURCE_DIR ${SB_CONFIG_OTHER_APP_IMAGE_PATH}
           )
         endif()

Additional dependency ordering can be set here if needed, see
:ref:`sysbuild_zephyr_application_dependencies` for details, image configuration can also be set
here, see :ref:`sysbuild_images_config` for details.

This secondary image can be enabled when building with west like so:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: <app>
   :board: nrf52840dk/nrf52840
   :goals: build
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_MY_IMAGE=y
   :compact:

This can then be extended by :ref:`modules` like so:

.. tabs::

   .. group-tab:: ``Kconfig.sysbuild``

      .. code-block:: kconfig

         config SUPPORT_OTHER_APP_MY_SECOND_IMAGE
                 bool
                 default y

         choice OTHER_APP

         config OTHER_APP_IMAGE_MY_SECOND_IMAGE
                 bool "my_second_image"
                 depends on SUPPORT_OTHER_APP_MY_SECOND_IMAGE
                 help
                   Include my other amazing image as the other app image to use, which does...

         endchoice

         config OTHER_APP_IMAGE_NAME
                 default "my_second_image" if OTHER_APP_IMAGE_MY_SECOND_IMAGE

         config OTHER_APP_IMAGE_PATH
                 default "$(ZEPHYR_MY_SECOND_IMAGE_MODULE_DIR)/path/to/my_second_image" if OTHER_APP_IMAGE_MY_SECOND_IMAGE

As can be seen, no additional CMake changes are needed to add an alternative image as the base
CMake code will add the replacement image instead of the original image, if selected.

This alternative secondary image can be enabled when building with west like so:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: <app>
   :board: nrf52840dk/nrf52840
   :goals: build
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_MY_SECOND_IMAGE=y
   :compact:

.. _sysbuild_images_config:

Image configuration
*******************

Sysbuild supports being able to set image configuration (Kconfig options) and also supports
reading the output of image configuration (Kconfig) which can be used to allow an
option to be added to sysbuild itself and then to configure it globally or selectively.

Setting image configuration
===========================

Kconfig
-------

Sysbuild can be used to set image configuration **before the CMake configuration of images takes
place**. An important note about setting Kconfig options in images is that these are persistent
and cannot be changed by the images. The following functions can be used to set configuration on
an image:

.. code-block:: cmake

   set_config_bool(<image> CONFIG_<setting> <value>)
   set_config_string(<image> CONFIG_<setting> <value>)
   set_config_int(<image> CONFIG_<setting> <value>)

For example, to change the default image to output a hex file:

.. code-block:: cmake

   set_config_bool(${DEFAULT_IMAGE} CONFIG_BUILD_OUTPUT_HEX y)

These can safely be used in an application, board or SoC ``sysbuild.cmake`` file, as that file is
included before the image CMake processes are invoked. When extending
:ref:`sysbuild using modules <sysbuild_module_integration>` the pre-CMake hook should be used
instead, for example:

.. code-block:: cmake

   function(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)
     cmake_parse_arguments(PRE_CMAKE "" "" "IMAGES" ${ARGN})

     foreach(image ${PRE_CMAKE_IMAGES})
       set_config_bool(${image} CONFIG_BUILD_OUTPUT_HEX y)
     endforeach()
   endfunction()

Image configuration script
==========================

An image configuration script is a CMake file that can be used to configure an image with common
configuration values, multiple can be used per image, the configuration should be transferrable to
different images to correctly configure them based upon options set in sysbuild. MCUboot
configuration options are configured in both the application the MCUboot image using this method
which allows sysbuild to be the central location for things like the signing key which are then
kept in-sync in the main application bootloader images.

Inside image configuration scripts, the ``ZCMAKE_APPLICATION`` variable is set to the name of the
application being configured, the ``set_config_*`` sysbuild CMake functions can be used to set
configuration and the sysbuild Kconfig can be read, for example:

.. code-block:: cmake

   if(SB_CONFIG_BOOTLOADER_MCUBOOT AND "${SB_CONFIG_SIGNATURE_TYPE}" STREQUAL "NONE")
     set_config_bool(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE y)
   endif()

Image configuration script (module/application)
-----------------------------------------------

Module/application image configuration scripts can be set from module or application code, this
must be done in a ``sysbuild.cmake`` file for an application. This can be used to add an image
configuration script as follows:

.. tabs::

   .. group-tab:: ``sysbuild.cmake``

      .. code-block:: cmake

         # This applies the image configuration script to the default image only
         get_property(tmp_conf_scripts TARGET ${DEFAULT_IMAGE} PROPERTY IMAGE_CONF_SCRIPT)
         list(APPEND tmp_conf_scripts "${CMAKE_SOURCE_DIR}/image_configurations/MY_CUSTOM_TYPE_image_default.cmake")
         set_target_properties(${DEFAULT_IMAGE} PROPERTIES IMAGE_CONF_SCRIPT "${tmp_conf_scripts}")


   .. group-tab:: Module CMake

      .. code-block:: cmake

         function(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)
           cmake_parse_arguments(PRE_CMAKE "" "" "IMAGES" ${ARGN})

           # This applies the image configuration script to all images
           foreach(image ${PRE_CMAKE_IMAGES})
             get_property(tmp_conf_scripts TARGET ${image} PROPERTY IMAGE_CONF_SCRIPT)
             list(APPEND tmp_conf_scripts "${CMAKE_SOURCE_DIR}/image_configurations/MY_CUSTOM_TYPE_image_default.cmake")
             set_target_properties(${image} PROPERTIES IMAGE_CONF_SCRIPT "${tmp_conf_scripts}")
           endforeach()
         endfunction(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)

Image configuration script (Zephyr-wide)
----------------------------------------

Global Zephyr provided image configuration scripts, which allow specifying the type when using
``ExternalZephyrProject_Add()`` require changes to sysbuild code in Zephyr. This should only be
added to when adding a new type that any project should be able to select, generally this should
only be needed for upstream Zephyr though forked versions of Zephyr might use this to add
additional types without restriction.

Image configuration has an allow-list of names which must be set in the Zephyr file
:zephyr_file:`share/sysbuild/cmake/modules/sysbuild_extensions.cmake` in the
``ExternalZephyrProject_Add`` function. After adding a new type, it can be used when adding a
sysbuild image, for example:

.. tabs::

   .. group-tab:: ``sysbuild_extensions.cmake``

      Full file path: ``share/sysbuild/cmake/modules/sysbuild_extensions.cmake``

      .. code-block:: cmake

         # ...
         # Usage:
         #   ExternalZephyrProject_Add(APPLICATION <name>
         #                             SOURCE_DIR <dir>
         #                             [BOARD <board> [BOARD_REVISION <revision>]]
         #                             [APP_TYPE <MAIN|BOOTLOADER|MY_CUSTOM_TYPE>]
         #   )
         # ...
         # APP_TYPE <MAIN|BOOTLOADER|MY_CUSTOM_TYPE>: Application type.
         #                                            MAIN indicates this application is the main application
         #                                            and where user defined settings should be passed on as-is
         #                                            except for multi image build flags.
         #                                            For example, -DCONF_FILES=<files> will be passed on to the
         #                                            MAIN_APP unmodified.
         #                                            BOOTLOADER indicates this app is a bootloader
         #                                            MY_CUSTOM_TYPE indicates this app is...
         # ...
         function(ExternalZephyrProject_Add)
           set(app_types MAIN BOOTLOADER MY_CUSTOM_TYPE)
         # ...

   .. group-tab:: ``sysbuild.cmake``

      .. code-block:: cmake

         if(SB_CONFIG_OTHER_APP_IMAGE_PATH)
           ExternalZephyrProject_Add(
             APPLICATION ${SB_CONFIG_OTHER_APP_IMAGE_NAME}
             SOURCE_DIR ${SB_CONFIG_OTHER_APP_IMAGE_PATH}
             APP_TYPE MY_CUSTOM_TYPE
           )
         endif()


   .. group-tab:: ``MY_CUSTOM_TYPE_image_default.cmake``

      Full file path: ``share/sysbuild/image_configurations/MY_CUSTOM_TYPE_image_default.cmake``

      .. code-block:: cmake

         # Here, the ZCMAKE_APPLICATION variable will be replaced with the image being configured
         set_config_bool(${ZCMAKE_APPLICATION} CONFIG_BUILD_OUTPUT_HEX y)

Reading image configuration
===========================

Kconfig
-------

Kconfig values from images can be read by sysbuild **after the CMake configuration of images has
taken place**. This can be used to check configuration or to adjust additional sysbuild tasks
depending upon configuration. The following function can be used for this purpose:

.. code-block:: cmake

   sysbuild_get(<variable> IMAGE <image> [VAR <image-variable>] KCONFIG)

This function can only be used when
:ref:`sysbuild is extended by a module <sysbuild_module_integration>` or inside of a ``sysbuild/CMakeLists.txt`` file after ``find_package(Sysbuild)`` has been used. An example of outputting
values from all images is shown:

.. tabs::

   .. group-tab:: Module CMake

      .. code-block:: cmake

         function(${SYSBUILD_CURRENT_MODULE_NAME}_post_cmake)
           cmake_parse_arguments(POST_CMAKE "" "" "IMAGES" ${ARGN})

           foreach(image ${POST_CMAKE_IMAGES})
             # Note that the variable to read in to must not be set before using the sysbuild_get() function
             set(tmp_val)
             sysbuild_get(tmp_val IMAGE ${image} VAR CONFIG_BUILD_OUTPUT_HEX KCONFIG)
             message(STATUS "Image ${image} build hex: ${tmp_val}")
           endforeach()
         endfunction()

   .. group-tab:: ``sysbuild/CMakeLists.txt``

      .. code-block:: cmake

         find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

         project(sysbuild LANGUAGES)

         sysbuild_get(tmp_val IMAGE ${DEFAULT_IMAGE} VAR CONFIG_BUILD_OUTPUT_HEX KCONFIG)
         message(STATUS "Image ${DEFAULT_IMAGE} build hex: ${tmp_val}")
