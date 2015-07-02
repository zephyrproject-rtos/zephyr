.. _architecture_apps:

The Application Model
#####################

The |codename| provides developers with a development environment to create
applications with a very small footprint without sacrificing functionality.
The hardware resources that you use determine many of the things you need to
consider while developing your applications. This section provides you the
information common to all supported platforms and explains the way that the
|codename| works with your application.

Large platforms allow your applications to use the microkernel. Smaller
platforms might only allow nanokernel applications. All applications are
built together with the kernel providing a single binary for each platform.
Applications can use any combination of the three execution contexts
available: ISRs, fibers and tasks. Nanokernel only applications can only use
a single task whereas microkernel applications can make use of multiple
tasks.

The application calls the different execution contexts it needs in order to
perform its processes. It is then within a execution context that the
appropriate kernel objects are called and used via their APIs. Keep in mind
that some objects can only be used within a certain context. For more
information regarding execution contexts, microkernel objects and nanokernel
objects please see the


Microkernel Applications
************************

Microkernel applications are C functions that can use the |codename|'s API
to accomplish their purpose. These functions are executed within one of
three contexts: tasks, fibers and ISRs. Some of the API is only available in
specific execution contexts but, typically, microkernel applications are
coded for execution within multiple tasks.

Microkernel applications require three components to work properly:

* C source code: The source contains all the instructions that the
application needs to fulfill its purpose.

* MDEF file: This file contains all the definitions of the microkernel
objects used in the source code.

* Makefiles and .conf files: These files add your application to the
|codename|'s build system. They allow the application to be compiled and
built as part of the kernel.

These are the general steps of the development of a microkernel application:

#. Create the MDEF file for the application with the microkernel objects
that the application will use.

#. Write the microkernel application including the proper header files.

#. Create the application's Makefiles.

#. Create the application's .conf files.

#. Build your application with the |codename|.

How to perform each of these steps can be found in
:ref:`microkernel_app`.

Keep in mind that the application will be built with the kernel. You can
expand the functionality of the kernel as needed. For information regarding
the creation of drivers refer to :ref:`architecture_drivers`.

Nanokernel Only Applications
****************************