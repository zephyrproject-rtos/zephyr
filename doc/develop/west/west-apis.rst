:orphan:

.. _west-apis:
.. _west-apis-west:

West APIs
#########

This page documents the Python APIs provided by :ref:`west <west>`, as well as
some additional APIs used by the :ref:`west extensions <west-extensions>` in
the zephyr repository.

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

.. autoclass:: west.commands.WestCommand

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

   .. py:attribute:: config

      A settable property which returns the
      :py:class:`west.configuration.Configuration` instance or aborts the
      program if one was not provided. This is only safe to use from the
      ``do_run()`` method.

   .. versionadded:: 0.13.0

   .. py:attribute:: has_config

      True if reading the config property will succeed instead of erroring
      out.

   .. versionadded:: 0.13.0

   .. py:attribute:: git_version_info

      A tuple of Git version information.

   .. versionadded:: 0.11.0

   .. py:attribute:: color_ui

      True if the west configuration permits colorized output,
      False otherwise.

   .. versionadded:: 1.0.0

   Constructor:

   .. automethod:: __init__

   .. versionadded:: 0.6.0
      The *requires_installation* parameter (removed in v0.13.0).
   .. versionadded:: 0.7.0
      The *requires_workspace* parameter.
   .. versionchanged:: 0.8.0
      The *topdir* parameter can now be any ``os.PathLike``.
   .. versionchanged:: 0.13.0
      The deprecated *requires_installation* parameter was removed.
   .. versionadded:: 1.0.0
      The *verbosity* parameter.

   Methods:

   .. automethod:: run

   .. versionchanged:: 0.6.0
      The *topdir* argument was added.

   .. automethod:: add_parser

   .. automethod:: add_pre_run_hook
   .. versionadded:: 1.0.0

   .. automethod:: check_call

   .. versionchanged:: 0.11.0

   .. automethod:: check_output

   .. versionchanged:: 0.11.0

   All subclasses must provide the following abstract methods, which are used
   to implement the above:

   .. automethod:: do_add_parser

   .. automethod:: do_run

   The following methods should be used when the command needs to print output.
   These were introduced to enable a transition from the deprecated
   ``west.log`` module to a per-command interface that will allow for a global
   "quiet" mode for west commands in a future release:

   .. automethod:: dbg
   .. versionadded:: 1.0.0

   .. automethod:: inf
   .. versionadded:: 1.0.0

   .. automethod:: wrn
   .. versionadded:: 1.0.0

   .. automethod:: err
   .. versionadded:: 1.0.0

   .. automethod:: die
   .. versionadded:: 1.0.0

   .. automethod:: banner
   .. versionadded:: 1.0.0

   .. automethod:: small_banner
   .. versionadded:: 1.0.0

.. _west-apis-commands-output:

Verbosity
=========

Since west v1.0, west commands should print output using methods like
west.commands.WestCommand.dbg(), west.commands.WestCommand.inf(), etc. (see
above). This section documents a related enum used to declare verbosity levels.

.. autoclass:: west.commands.Verbosity

   .. autoattribute:: QUIET
   .. autoattribute:: ERR
   .. autoattribute:: WRN
   .. autoattribute:: INF
   .. autoattribute:: DBG
   .. autoattribute:: DBG_MORE
   .. autoattribute:: DBG_EXTREME

.. versionadded:: 1.0.0

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

Since west v0.13, the recommended class for reading this is
:py:class:`west.configuration.Configuration`.

Note that if you are writing a :ref:`west extension <west-extensions>`, you can
access the current ``Configuration`` object as ``self.config``. See
:py:class:`west.commands.WestCommand`.

Configuration API
=================

This is the recommended API to use since west v0.13.

.. autoclass:: west.configuration.ConfigFile

.. autoclass:: west.configuration.Configuration
   :members:

   .. versionadded:: 0.13.0

Deprecated APIs
===============

The following APIs also use :py:class:`west.configuration.ConfigFile`, but they
operate by default on a global object which stores the current workspace
configuration. This has proven to be a bad design decision since west's APIs
can be used from multiple workspaces. They were deprecated in west v0.13.0.

These APIs are preserved for compatibility with older extensions. They should
not be used in new code when west v0.13.0 or later may be assumed.

.. autofunction:: west.configuration.read_config

.. versionchanged:: 0.8.0
   The deprecated *read_config* parameter was removed.

.. versionchanged:: 0.6.0
   Errors due to an inability to find a local configuration file are ignored.

.. autofunction:: west.configuration.update_config

.. py:data:: west.configuration.config

   Module-global ConfigParser instance for the current configuration. This
   should be initialized with :py:func:`west.configuration.read_config` before
   being read.

.. _west-apis-log:

west.log (deprecated)
*********************

.. automodule:: west.log

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

The main classes are :py:class:`Manifest` and :py:class:`Project`. These
represent the contents of a :ref:`manifest file <west-manifests>`. The
recommended method for parsing west manifests is
:py:meth:`Manifest.from_topdir`.

Constants and functions
=======================

.. autodata:: MANIFEST_PROJECT_INDEX
.. autodata:: MANIFEST_REV_BRANCH
.. autodata:: QUAL_MANIFEST_REV_BRANCH
.. autodata:: QUAL_REFS_WEST
.. autodata:: SCHEMA_VERSION

.. autofunction:: west.manifest.manifest_path

.. autofunction:: west.manifest.validate

.. versionchanged:: 0.13.0
   This returns the validated dict containing the parsed YAML data.

Manifest and sub-objects
========================

.. autoclass:: west.manifest.Manifest

   .. automethod:: __init__
   .. versionchanged:: 0.7.0
      The *importer* and *import_flags* keyword arguments.
   .. versionchanged:: 0.13.0
      All arguments were made keyword-only. The *source_file* argument was
      removed (use *topdir* instead). The function no longer raises
      ``WestNotFound``.
   .. versionadded:: 0.13.0
      The *config* argument.
   .. versionadded:: 0.13.0
      The *abspath*, *posixpath*, *relative_path*, *yaml_path*, *repo_path*,
      *repo_posixpath*, and *userdata* attributes.

   .. automethod:: from_topdir
   .. versionadded:: 0.13.0

   .. automethod:: from_file
   .. versionchanged:: 0.7.0
      ``**kwargs`` added.
   .. versionchanged:: 0.8.0
      The *source_file*, *manifest_path*, and *topdir* arguments
      can now be any ``os.PathLike``.
   .. versionchanged:: 0.13.0
      The *manifest_path* and *topdir* arguments were removed.

   .. automethod:: from_data
   .. versionchanged:: 0.7.0
      ``**kwargs`` added, and *source_data* may be a ``str``.
   .. versionchanged:: 0.13.0
      The *manifest_path* and *topdir* arguments were removed.

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
   .. automethod:: is_active
   .. versionadded:: 0.9.0
   .. versionchanged:: 1.1.0
      This respects the ``manifest.project-filter`` configuration
      option. See :ref:`west-config-index`.

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

   .. versionadded:: 0.9.0
      The *group_filter* and *submodules* attributes.

   .. versionadded:: 0.12.0
      The *userdata* attribute.

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

.. autoclass:: west.manifest.Submodule

.. versionadded:: 0.9.0

Exceptions
==========

.. autoclass:: west.configuration.MalformedConfig
   :show-inheritance:

.. autoclass:: west.manifest.MalformedManifest
   :show-inheritance:

.. autoclass:: west.manifest.ManifestVersionError
   :show-inheritance:

   .. versionchanged:: 0.8.0
      The *file* argument can now be any ``os.PathLike``.

.. autoclass:: west.manifest.ManifestImportFailed
   :show-inheritance:

   .. versionchanged:: 0.8.0
      The *filename* argument can now be any ``os.PathLike``.

   .. versionchanged:: 0.13.0
      The *filename* argument was renamed *imp*, and can now take any value.

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
