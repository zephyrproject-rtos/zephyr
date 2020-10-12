:orphan:

.. _west-apis:
.. _west-apis-west:

West APIs
#########

This page documents the Python APIs provided by :ref:`west <west>`, as well as
some additional APIs used by the :ref:`west extensions <west-extensions>` in
the zephyr repository.

.. warning::

   These APIs should be considered unstable until west version 1.0 (see `west
   #38`_).

.. _west #38:
   https://github.com/zephyrproject-rtos/west/issues/38

**Contents**:

.. contents::
   :local:

.. NOTE: documentation authors:

   1. keep these sorted by package/module name.
   2. if you add a :ref: target here, add it to west-not-found.rst too.

.. _west-apis-commands:

west.commands
*************

.. module:: west.commands

All built-in and extension commands are implemented as subclasses of the
:py:class:`WestCommand` class defined here. Some exception types are also
provided.

WestCommand
===========

.. py:class:: west.commands.WestCommand

   Instance attributes:

   .. py:attribute:: name

      As passed to the constructor.

   .. py:attribute:: help

      As passed to the constructor.

   .. py:attribute:: description

      As passed to the constructor.

   .. py:attribute:: accepts_unknown_args

      As passed to the constructor.

   .. py:attribute:: requires_workspace

      As passed to the constructor.

   .. versionadded:: 0.7.0

   .. py:attribute:: parser

      The argument parser created by calling ``WestCommand.add_parser()``.

   Instance properties:

   .. py:attribute:: manifest

      A property which returns the :py:class:`west.manifest.Manifest`
      instance for the current manifest file or aborts the program if one was
      not provided. This is only safe to use from the ``do_run()`` method.

   .. versionadded:: 0.6.1
   .. versionchanged:: 0.7.0
      This is now settable.

   .. py:attribute:: has_manifest

      True if reading the manifest property will succeed instead of erroring
      out.

   Constructor:

   .. automethod:: __init__

   .. versionchanged:: 0.8.0
      The *topdir* parameter can now be any ``os.PathLike``.
   .. versionadded:: 0.6.0
      The *requires_installation* parameter.
   .. versionadded:: 0.7.0
      The *requires_workspace* parameter.

   Methods:

   .. automethod:: run

   .. versionchanged:: 0.6.0
      The *topdir* argument was added.

   .. automethod:: add_parser

   All subclasses must provide the following abstract methods, which are used
   to implement the above:

   .. automethod:: do_add_parser

   .. automethod:: do_run

Exceptions
==========

.. autoclass:: west.commands.CommandError
   :show-inheritance:

   .. py:attribute:: returncode

      Recommended program exit code for this error.

.. autoclass:: west.commands.CommandContextError
   :show-inheritance:

.. _west-apis-configuration:

west.configuration
******************

.. automodule:: west.configuration

This provides API access to west configuration files and data.

Reading and writing options
===========================

.. autofunction:: west.configuration.read_config

.. versionchanged:: 0.8.0
   The deprecated *read_config* parameter was removed.

.. versionchanged:: 0.6.0
   Errors due to an inability to find a local configuration file are ignored.

.. autofunction:: west.configuration.update_config

.. autoclass:: west.configuration.ConfigFile

Global configuration instance
=============================

.. py:data:: west.configuration.config

   Module-global ConfigParser instance for the current configuration. This
   should be initialized with :py:func:`west.configuration.read_config` before
   being read.

.. _west-apis-log:

west.log
********

.. automodule:: west.log

This module's functions are used whenever a running west command needs to print
to standard out or error streams.

This is safe to use from extension commands if you want output that mirrors
that of west itself.

Verbosity control
=================

To set the global verbosity level, use ``set_verbosity()``.

.. autofunction:: set_verbosity

These verbosity levels are defined.

.. autodata:: VERBOSE_NONE
.. autodata:: VERBOSE_NORMAL
.. autodata:: VERBOSE_VERY
.. autodata:: VERBOSE_EXTREME

Output functions
================

The main functions are ``dbg()``, ``inf()``, ``wrn()``, ``err()``, and
``die()``. Two special cases of ``inf()``, ``banner()`` and ``small_banner()``,
are also available for grouping output into "sections".

.. autofunction:: dbg
.. autofunction:: inf
.. autofunction:: wrn
.. autofunction:: err
.. autofunction:: die

.. autofunction:: banner
.. autofunction:: small_banner

.. _west-apis-manifest:

west.manifest
*************

.. automodule:: west.manifest

The main classes are `Manifest` and `Project`. These represent the contents of
a :ref:`manifest file <west-manifests>`. The recommended methods for parsing
west manifests are `Manifest.from_file` and `Manifest.from_data`.

Constants and functions
=======================

.. autodata:: MANIFEST_PROJECT_INDEX
.. autodata:: MANIFEST_REV_BRANCH
.. autodata:: QUAL_MANIFEST_REV_BRANCH
.. autodata:: QUAL_REFS_WEST
.. autodata:: SCHEMA_VERSION

.. autofunction:: west.manifest.manifest_path

.. autofunction:: west.manifest.validate

Manifest and sub-objects
========================

.. autoclass:: west.manifest.Manifest

   .. automethod:: __init__
   .. versionchanged:: 0.7.0
      The *importer* and *import_flags* keyword arguments.

   .. automethod:: from_file
   .. versionchanged:: 0.8.0
      The *source_file*, *manifest_path*, and *topdir* arguments
      can now be any ``os.PathLike``.
   .. versionchanged:: 0.7.0
      ``**kwargs`` added.

   .. automethod:: from_data
   .. versionchanged:: 0.7.0
      ``**kwargs`` added, and *source_data* may be a ``str``.

   Conveniences for accessing sub-objects by name or other identifier:

   .. automethod:: get_projects
   .. versionchanged:: 0.8.0
      The *project_ids* sequence can now contain any ``os.PathLike``.
   .. versionadded:: 0.6.1

   Additional methods:

   .. automethod:: as_dict
   .. versionadded:: 0.7.0
   .. automethod:: as_frozen_dict
   .. automethod:: as_yaml
   .. versionadded:: 0.7.0
   .. automethod:: as_frozen_yaml
   .. versionadded:: 0.7.0

.. autoclass:: west.manifest.ImportFlag
   :members:
   :member-order: bysource

.. autoclass:: west.manifest.Project

   .. (note: attributes are part of the class docstring)

   .. versionchanged:: 0.8.0
      The *west_commands* attribute is now always a list. In previous
      releases, it could be a string or ``None``.

   .. versionchanged:: 0.7.0
      The *remote* attribute was removed. Its semantics could no longer
      be preserved when support for manifest ``import`` keys was added.

   .. versionadded:: 0.7.0
      The *remote_name* and *name_and_path* attributes.

   Constructor:

   .. automethod:: __init__

   .. versionchanged:: 0.8.0
      The *path* and *topdir* parameters can now be any ``os.PathLike``.

   .. versionchanged:: 0.7.0
      The parameters were incompatibly changed from previous versions.

   Methods:

   .. automethod:: as_dict
   .. versionadded:: 0.7.0

   .. automethod:: git
   .. versionchanged:: 0.6.1
      The *capture_stderr* kwarg.
   .. versionchanged:: 0.7.0
      The (now removed) ``Project.format`` method is no longer called on
      arguments.

   .. automethod:: sha
   .. versionchanged:: 0.7.0
      Standard error is now captured.

   .. automethod:: is_ancestor_of
   .. versionchanged:: 0.8.0
      The *cwd* parameter can now be any ``os.PathLike``.

   .. automethod:: is_cloned
   .. versionchanged:: 0.8.0
      The *cwd* parameter can now be any ``os.PathLike``.
   .. versionadded:: 0.6.1

   .. automethod:: is_up_to_date_with
   .. versionchanged:: 0.8.0
      The *cwd* parameter can now be any ``os.PathLike``.

   .. automethod:: is_up_to_date
   .. versionchanged:: 0.8.0
      The *cwd* parameter can now be any ``os.PathLike``.

   .. automethod:: read_at
   .. versionchanged:: 0.8.0
      The *cwd* parameter can now be any ``os.PathLike``.
   .. versionadded:: 0.7.0

   .. automethod:: listdir_at
   .. versionchanged:: 0.8.0
      The *cwd* parameter can now be any ``os.PathLike``.
   .. versionadded:: 0.7.0

.. autoclass:: west.manifest.ManifestProject

   A limited subset of Project methods is supported.
   Results for calling others are not specified.

   .. versionchanged:: 0.8.0
      The *url* attribute is now the empty string instead of ``None``.
      The *abspath* attribute is created using ``os.path.abspath()``
      instead of ``os.path.realpath()``, improving support for symbolic links.

   .. automethod:: as_dict

.. versionadded:: 0.6.0

Exceptions
==========

.. autoclass:: west.manifest.MalformedManifest
   :show-inheritance:

.. autoclass:: west.manifest.MalformedConfig
   :show-inheritance:

.. autoclass:: west.manifest.ManifestVersionError
   :show-inheritance:

   .. versionchanged:: 0.8.0
      The *file* argument can now be any ``os.PathLike``.

.. autoclass:: west.manifest.ManifestImportFailed
   :show-inheritance:

   .. versionchanged:: 0.8.0
      The *filename* argument can now be any ``os.PathLike``.

.. _west-apis-util:

west.util
*********

.. canon_path(), escapes_directory(), etc. intentionally not documented here.

.. automodule:: west.util

Functions
=========

.. autofunction:: west.util.west_dir

   .. versionchanged:: 0.8.0
      The *start* parameter can be any ``os.PathLike``.

.. autofunction:: west.util.west_topdir

   .. versionchanged:: 0.8.0
      The *start* parameter can be any ``os.PathLike``.

Exceptions
==========

.. autoclass:: west.util.WestNotFound
   :show-inheritance:
