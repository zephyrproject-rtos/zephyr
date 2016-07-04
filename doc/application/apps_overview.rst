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

* **Kernel configuration files**: An application typically provides a configuration
  file (:file:`.conf`) that specifies values for one or more kernel configuration
  options. If omitted, the application's existing kernel configuration option values
  are used; if no existing values are provided, the kernel's default configuration
  values are used.

* **MDEF file**: A microkernel application typically provides a Microkernel
  DEFinitions file (:file:`.mdef`) that defines the public kernel objects
  used by the application and the kernel. If omitted, no public kernel objects
  are defined. This file is not used with a nanokernel-only application.

* **Makefile**: This file typically contains a handful of lines that tell the build
  system where to find the files mentioned above, as well as the desired target
  board configuration and kernel type (either microkernel or nanokernel).

Once the application has been defined, it can be built with a single command.
The results of the build process, including the final application image,
are located in a sub-directory called :file:`outdir`.
