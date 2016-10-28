.. _apps_overview:

Application Overview
####################

A Zephyr application is a collection of user-supplied files that can be built
into an application image that executes on a board.

Each application resides in a distinct directory created by the developer.
The directory has the following structure.

* **Application source code files**: An application typically provides one
  or more application-specific files, written in C or assembly language. These
  files are usually located in a sub-directory called :file:`src`.

* **Kernel configuration files**: An application typically provides
  a configuration file (:file:`.conf`) that specifies values for one or more
  kernel configuration options. If omitted, the application's existing kernel
  configuration option values are used; if no existing values are provided,
  the kernel's default configuration values are used.

* **Makefile**: This file typically contains a handful of lines that tell
  the build system where to find the files mentioned above, as well as
  the desired target board configuration.

Once the application has been defined, it can be built with a single command.
The results of the build process are located in a sub-directory
called :file:`outdir/BOARD`. This directory contains the files generated
by the build process, the most notable of which are listed below.

* The :file:`.config` file that contains the configuration settings
  used to build the application.

* The various object files (:file:`.o` files and :file:`.a` files) containing
  custom-built kernel and application-specific code.

* The :file:`zephyr.elf` file that contains the final application image.
