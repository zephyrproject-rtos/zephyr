.. _app-version-details:

Application version management
******************************

Zephyr supports an application version management system for applications which is built around the
system that Zephyr uses for its own version system management. This allows applications to define a
version file and have application (or module) code include the auto-generated file and be able to
access it, just as they can with the kernel version. This version information is available from
multiple scopes, including:

* Code (C/C++)
* Kconfig
* CMake
* Shell

which makes it a very versatile system for lifecycle management of applications. In addition, it
can be used when building applications which target supported bootloaders (e.g. MCUboot) allowing
images to be signed with correct version of the application automatically - no manual signing
steps are required.

VERSION file
============

Application version information is set on a per-application basis in a file named :file:`VERSION`,
which must be placed at the base directory of the application, where the CMakeLists.txt file is
located. This is a simple text file which contains the various version information fields, each on
a newline. The basic ``VERSION`` file has the following structure:

.. code-block:: cfg

   VERSION_MAJOR =
   VERSION_MINOR =
   PATCHLEVEL =
   VERSION_TWEAK =
   EXTRAVERSION =

Each field and the values it supports is described below. Zephyr limits the value of each numeric
field to a single byte (note that there may be further restrictions depending upon what the version
is used for, e.g. bootloaders might only support some of these fields or might place limits on the
maximum values of fields):

+---------------+-------------------------------------------------------+
| Field         | Data type                                             |
+---------------+-------------------------------------------------------+
| VERSION_MAJOR | Numerical (0-255)                                     |
+---------------+-------------------------------------------------------+
| VERSION_MINOR | Numerical (0-255)                                     |
+---------------+-------------------------------------------------------+
| PATCHLEVEL    | Numerical (0-255)                                     |
+---------------+-------------------------------------------------------+
| VERSION_TWEAK | Numerical (0-255)                                     |
+---------------+-------------------------------------------------------+
| EXTRAVERSION  | Alphanumerical (Lowercase a-z and 0-9) and "." or "-" |
+---------------+-------------------------------------------------------+

When an application is configured using CMake, the version file will be automatically processed,
and will be checked automatically each time the version is changed, so CMake does not need to be
manually re-ran for changes to this file.

For the sections below, examples are provided for the following :file:`VERSION` file:

.. code-block:: cfg

   VERSION_MAJOR = 1
   VERSION_MINOR = 2
   PATCHLEVEL = 3
   VERSION_TWEAK = 4
   EXTRAVERSION = unstable.5

Use in code
===========

To use the version information in application code, the version file must be included, then the
fields can be freely used. The include file name is :file:`app_version.h` (no path is needed), the
following defines are available:

+-----------------------------+-------------------+------------------------------------------------------+---------------------------+
| Define                      | Type              | Field(s)                                             | Example                   |
+-----------------------------+-------------------+------------------------------------------------------+---------------------------+
| APPVERSION                  | Numerical         | ``VERSION_MAJOR`` (left shifted by 24 bits), |br|    | 0x1020304                 |
|                             |                   | ``VERSION_MINOR`` (left shifted by 16 bits), |br|    |                           |
|                             |                   | ``PATCHLEVEL`` (left shifted by 8 bits), |br|        |                           |
|                             |                   | ``VERSION_TWEAK``                                    |                           |
+-----------------------------+-------------------+------------------------------------------------------+---------------------------+
| APP_VERSION_NUMBER          | Numerical         | ``VERSION_MAJOR`` (left shifted by 16 bits), |br|    | 0x10203                   |
|                             |                   | ``VERSION_MINOR`` (left shifted by 8 bits), |br|     |                           |
|                             |                   | ``PATCHLEVEL``                                       |                           |
+-----------------------------+-------------------+------------------------------------------------------+---------------------------+
| APP_VERSION_MAJOR           | Numerical         | ``VERSION_MAJOR``                                    | 1                         |
+-----------------------------+-------------------+------------------------------------------------------+---------------------------+
| APP_VERSION_MINOR           | Numerical         | ``VERSION_MINOR``                                    | 2                         |
+-----------------------------+-------------------+------------------------------------------------------+---------------------------+
| APP_PATCHLEVEL              | Numerical         | ``PATCHLEVEL``                                       | 3                         |
+-----------------------------+-------------------+------------------------------------------------------+---------------------------+
| APP_TWEAK                   | Numerical         | ``VERSION_TWEAK``                                    | 4                         |
+-----------------------------+-------------------+------------------------------------------------------+---------------------------+
| APP_VERSION_STRING          | String (quoted)   | ``VERSION_MAJOR``, |br|                              | "1.2.3-unstable.5"        |
|                             |                   | ``VERSION_MINOR``, |br|                              |                           |
|                             |                   | ``PATCHLEVEL``, |br|                                 |                           |
|                             |                   | ``EXTRAVERSION`` |br|                                |                           |
+-----------------------------+-------------------+------------------------------------------------------+---------------------------+
| APP_VERSION_EXTENDED_STRING | String (quoted)   | ``VERSION_MAJOR``, |br|                              | "1.2.3-unstable.5+4"      |
|                             |                   | ``VERSION_MINOR``, |br|                              |                           |
|                             |                   | ``PATCHLEVEL``, |br|                                 |                           |
|                             |                   | ``EXTRAVERSION`` |br|                                |                           |
|                             |                   | ``VERSION_TWEAK`` |br|                               |                           |
+-----------------------------+-------------------+------------------------------------------------------+---------------------------+
| APP_VERSION_TWEAK_STRING    | String (quoted)   | ``VERSION_MAJOR``, |br|                              | "1.2.3+4"                 |
|                             |                   | ``VERSION_MINOR``, |br|                              |                           |
|                             |                   | ``PATCHLEVEL``, |br|                                 |                           |
|                             |                   | ``VERSION_TWEAK`` |br|                               |                           |
+-----------------------------+-------------------+------------------------------------------------------+---------------------------+
| APP_BUILD_VERSION           | String (unquoted) | None (value of ``git describe --abbrev=12 --always`` | v3.3.0-18-g2c85d9224fca   |
|                             |                   | from application repository)                         |                           |
+-----------------------------+-------------------+------------------------------------------------------+---------------------------+

Use in Kconfig
==============

The following variables are available for usage in Kconfig files:

+--------------------------------+-----------+--------------------------+--------------------+
| Variable                       | Type      | Field(s)                 | Example            |
+--------------------------------+-----------+--------------------------+--------------------+
| $(VERSION_MAJOR)               | Numerical | ``VERSION_MAJOR``        | 1                  |
+--------------------------------+-----------+--------------------------+--------------------+
| $(VERSION_MINOR)               | Numerical | ``VERSION_MINOR``        | 2                  |
+--------------------------------+-----------+--------------------------+--------------------+
| $(PATCHLEVEL)                  | Numerical | ``PATCHLEVEL``           | 3                  |
+--------------------------------+-----------+--------------------------+--------------------+
| $(VERSION_TWEAK)               | Numerical | ``VERSION_TWEAK``        | 4                  |
+--------------------------------+-----------+--------------------------+--------------------+
| $(APPVERSION)                  | String    | ``VERSION_MAJOR``, |br|  | 1.2.3-unstable.5   |
|                                |           | ``VERSION_MINOR``, |br|  |                    |
|                                |           | ``PATCHLEVEL``, |br|     |                    |
|                                |           | ``EXTRAVERSION``         |                    |
+--------------------------------+-----------+--------------------------+--------------------+
| $(APP_VERSION_EXTENDED_STRING) | String    | ``VERSION_MAJOR``, |br|  | 1.2.3-unstable.5+4 |
|                                |           | ``VERSION_MINOR``, |br|  |                    |
|                                |           | ``PATCHLEVEL``, |br|     |                    |
|                                |           | ``EXTRAVERSION``, |br|   |                    |
|                                |           | ``VERSION_TWEAK``        |                    |
+--------------------------------+-----------+--------------------------+--------------------+
| $(APP_VERSION_TWEAK_STRING)    | String    | ``VERSION_MAJOR``, |br|  | 1.2.3+4            |
|                                |           | ``VERSION_MINOR``, |br|  |                    |
|                                |           | ``PATCHLEVEL``, |br|     |                    |
|                                |           | ``VERSION_TWEAK``        |                    |
+--------------------------------+-----------+--------------------------+--------------------+

Use in CMake
============

The following variable are available for usage in CMake files:

+-----------------------------+-----------------+---------------------------------------------------+--------------------+
| Variable                    | Type            | Field(s)                                          | Example            |
+-----------------------------+-----------------+---------------------------------------------------+--------------------+
| APPVERSION                  | Numerical (hex) | ``VERSION_MAJOR`` (left shifted by 24 bits), |br| | 0x1020304          |
|                             |                 | ``VERSION_MINOR`` (left shifted by 16 bits), |br| |                    |
|                             |                 | ``PATCHLEVEL`` (left shifted by 8 bits), |br|     |                    |
|                             |                 | ``VERSION_TWEAK``                                 |                    |
+-----------------------------+-----------------+---------------------------------------------------+--------------------+
| APP_VERSION_NUMBER          | Numerical (hex) | ``VERSION_MAJOR`` (left shifted by 16 bits), |br| | 0x10203            |
|                             |                 | ``VERSION_MINOR`` (left shifted by 8 bits), |br|  |                    |
|                             |                 | ``PATCHLEVEL``                                    |                    |
+-----------------------------+-----------------+---------------------------------------------------+--------------------+
| APP_VERSION_MAJOR           | Numerical       | ``VERSION_MAJOR``                                 | 1                  |
+-----------------------------+-----------------+---------------------------------------------------+--------------------+
| APP_VERSION_MINOR           | Numerical       | ``VERSION_MINOR``                                 | 2                  |
+-----------------------------+-----------------+---------------------------------------------------+--------------------+
| APP_PATCHLEVEL              | Numerical       | ``PATCHLEVEL``                                    | 3                  |
+-----------------------------+-----------------+---------------------------------------------------+--------------------+
| APP_VERSION_TWEAK           | Numerical       | ``VERSION_TWEAK``                                 | 4                  |
+-----------------------------+-----------------+---------------------------------------------------+--------------------+
| APP_VERSION_STRING          | String          | ``VERSION_MAJOR``, |br|                           | 1.2.3-unstable.5   |
|                             |                 | ``VERSION_MINOR``, |br|                           |                    |
|                             |                 | ``PATCHLEVEL``, |br|                              |                    |
|                             |                 | ``EXTRAVERSION``                                  |                    |
+-----------------------------+-----------------+---------------------------------------------------+--------------------+
| APP_VERSION_EXTENDED_STRING | String          | ``VERSION_MAJOR``, |br|                           | 1.2.3-unstable.5+4 |
|                             |                 | ``VERSION_MINOR``, |br|                           |                    |
|                             |                 | ``PATCHLEVEL``, |br|                              |                    |
|                             |                 | ``EXTRAVERSION``, |br|                            |                    |
|                             |                 | ``VERSION_TWEAK``                                 |                    |
+-----------------------------+-----------------+---------------------------------------------------+--------------------+
| APP_VERSION_TWEAK_STRING    | String          | ``VERSION_MAJOR``, |br|                           | 1.2.3+4            |
|                             |                 | ``VERSION_MINOR``, |br|                           |                    |
|                             |                 | ``PATCHLEVEL``, |br|                              |                    |
|                             |                 | ``VERSION_TWEAK``                                 |                    |
+-----------------------------+-----------------+---------------------------------------------------+--------------------+

Use in MCUboot-supported applications
=====================================

No additional configuration needs to be done to the target application so long as it is configured
to support MCUboot and a signed image is generated, the version information will be automatically
included in the image data.

Use in Shell
============

When a shell interface is configured, the following commands are available to retrieve application version information:

+----------------------+-----------------------------+-------------------------+
| Command              | Variable                    | Example                 |
+----------------------+-----------------------------+-------------------------+
| app version          | APP_VERSION_STRING          | 1.2.3-unstable.5        |
+----------------------+-----------------------------+-------------------------+
| app version-extended | APP_VERSION_EXTENDED_STRING | 1.2.3-unstable.5+4      |
+----------------------+-----------------------------+-------------------------+
| app build-version    | APP_BUILD_VERSION           | v3.3.0-18-g2c85d9224fca |
+----------------------+-----------------------------+-------------------------+
