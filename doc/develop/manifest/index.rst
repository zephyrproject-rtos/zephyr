:orphan:

.. _west_projects_index:

West Projects index
###################

See :ref:`external-contributions` for more information about
this contributing and review process for imported components.

Active Projects/Modules
+++++++++++++++++++++++

The projects below are enabled by default and will be downloaded when you
call `west update`. Many of the projects or modules listed below are
essential for building generic Zephyr application and include among others
hardware support for many of the platforms available in Zephyr.

To disable any of the active modules, for example a specific HAL, use the
following commands::

        west config manifest.project-filter -- -hal_FOO
        west update

.. manifest-projects-table::
   :filter: active

Inactive and Optional Projects/Modules
++++++++++++++++++++++++++++++++++++++


The projects below are optional and will not be downloaded when you
call `west update`. You can add any of the projects or modules listed below
and use them to write application code and extend your workspace with the added
functionality.

To enable any of the modules below, use the following commands::

        west config manifest.project-filter -- +nanopb
        west update

.. manifest-projects-table::
   :filter: inactive

External Projects/Modules
++++++++++++++++++++++++++


The projects listed below are external and are not directly imported into the
default manifest.
To use any of the projects below, you will need to define your own manifest
file which includes them.  See :ref:`west-manifest-import` for information on
recommended ways to do this while still inheriting the mandatory modules from
Zephyr's :file:`west.yml`.

Use the template :file:`doc/develop/manifest/external/external.rst.tmpl` to add
external modules to the list below:

.. toctree::
   :titlesonly:
   :maxdepth: 1
   :glob:

   external/*
