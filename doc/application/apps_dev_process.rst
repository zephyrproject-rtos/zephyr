.. _apps_dev_process:

Application Development Workflow
################################

The application development workflow identifies procedures needed to create, build, and
run a Zephyr microkernel or nanokernel application.

Before you build
----------------

* Check that your Linux host meets the minimum requirements specified in the
  :ref:`quick_start`.

* Check that environment variables have been configured correctly as outlined
  in :ref:`apps_common_procedures`.

Workflow
--------

1. Create a directory structure for your Zephyr application.

   a) :ref:`create_directory_structure`

2. Add a Makefile

   b) :ref:`create_src_makefile`

3. Define the application's default kernel configuration using
   :ref:`define_default_kernel_conf`.

4. Define kernel configuration override options for the application
   using :ref:`override_kernel_conf`.

5. For a microkernel application, define objects as you develop code
   using :ref:`create_mdef`.

6. For all applications, define nanokernel objects as you need them in
   code.

7. Develop source code and add source code files to the src directory.

   * :ref:`naming_conventions`
   * :ref:`src_makefiles_reqs`
   * :ref:`src_files_directories`

8. Build an application image.

   * :ref:`apps_build`


9. To test the application image's functionality on simulated hardware
   with QEMU, see :ref:`apps_run`.

10. To load an application image on a target hardware, see using
    :ref:`board` documentation.
