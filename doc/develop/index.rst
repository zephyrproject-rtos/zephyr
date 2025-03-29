.. _developing_with_zephyr:

Developing with Zephyr
######################

Whether you're a newcomer or an experienced developer, this page provides resources and guidance to
help you navigate Zephyr's development environment and best practices.

Getting Started
===============

.. toctree::
   :maxdepth: 1
   :hidden:

   getting_started/index.rst
   beyond-GSG.rst

:ref:`getting_started`
   Begin your Zephyr journey with this comprehensive guide that helps you set up your environment
   and build your first application.

:ref:`beyond-gsg`
   Explore additional resources and recommendations for developers who have completed the Getting
   Started Guide.

Application Development
=======================

.. toctree::
   :maxdepth: 1
   :hidden:

   application/index.rst
   debug/index.rst
   flash_debug/index
   modules.rst

:ref:`application`
   Dive into application development, including building, running, and structuring your Zephyr projects.

:ref:`debugging`
   Master debugging techniques to diagnose and fix issues in your applications.

:ref:`flashing_and_debugging`
   Learn about hardware debugging and flashing applications onto physical devices.

:ref:`modules`
   Integrate external modules and projects into your Zephyr environment.

Development Environment
=======================

.. toctree::
   :maxdepth: 1
   :hidden:

   env_vars.rst
   toolchains/index
   tools/index
   west/index

:ref:`env_vars`
   Learn about the essential environment variables required for Zephyr development.

:ref:`toolchains`
   Understand the supported toolchains and their configurations for compiling Zephyr applications.

:ref:`dev_tools`
   Discover tools and integrated development environments (IDEs) that simplify Zephyr development.

:ref:`West <west>`
   West is Zephyr's meta-tool that helps you manage the code base, dependencies, and build process.

Testing and Optimization
========================

.. toctree::
   :maxdepth: 1
   :hidden:

   test/index
   sca/index
   optimizations/index

:ref:`testing`
   Understand Zephyr's testing framework and how to write and execute tests for your applications.

:ref:`sca`
   Learn how to enable and use static code analysis tools such as CodeChecker, ECLAIR, and more.

:ref:`optimizations`
   Optimize your applications for performance, memory usage, and power consumption.

APIs and Language Support
=========================

.. toctree::
   :maxdepth: 1
   :hidden:

   api/index.rst
   languages/index.rst

:ref:`api_status_and_guidelines`
   Explore the current status of Zephyr APIs and the guidelines for their usage and development.

:ref:`language_support`
   Learn about the supported programming languages and their integration with Zephyr.
