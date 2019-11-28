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

.. py:class:: west.commands.WestCommand

   .. automethod:: __init__

   .. versionadded:: 0.6.0
      The *requires_installation* parameter.

   Methods:

   .. automethod:: run

   .. versionchanged:: 0.6.0
      The *topdir* argument was added.

   .. automethod:: add_parser

   All subclasses must provide the following abstract methods, which are used
   to implement the above:

   .. automethod:: do_add_parser

   .. automethod:: do_run

   Instance attributes:

   .. py:attribute:: name

      As passed to the constructor.

   .. py:attribute:: help

      As passed to the constructor.

   .. py:attribute:: description

      As passed to the constructor.

   .. py:attribute:: accepts_unknown_args

      As passed to the constructor.

   .. py:attribute:: requires_installation

      As passed to the constructor.

   .. py:attribute:: parser

      The argument parser created by calling ``WestCommand.add_parser()``.

   Instance properties:

   .. py:attribute:: manifest

      A read-only property which returns the :py:class:`west.manifest.Manifest`
      instance for the current manifest file or aborts the program if one was
      not provided. This is only safe to use from the ``do_run()`` method.

   .. versionadded:: 0.6.1

.. autoclass:: west.commands.CommandError
   :show-inheritance:

   .. py:attribute:: returncode

      Recommended program exit code for this error.

.. autoclass:: west.commands.CommandContextError
   :show-inheritance:

.. autoclass:: west.commands.ExtensionCommandError
   :show-inheritance:

   .. py:method:: ExtensionCommandError.__init__(hint=None, **kwargs)

      If *hint* is given, it is a string indicating the cause of the problem.
      All other kwargs are passed to the super constructor.

   .. py:attribute:: hint

      As passed to the constructor.

.. _west-apis-configuration:

west.configuration
******************

.. automodule:: west.configuration

.. autoclass:: west.configuration.ConfigFile

.. autofunction:: west.configuration.read_config

.. versionchanged:: 0.6.0
   Errors due to an inability to find a local configuration file are ignored.

.. autofunction:: west.configuration.update_config

.. py:data:: west.configuration.config

   Module-global ConfigParser instance for the current configuration. This
   should be initialized with :py:func:`west.configuration.read_config` before
   being read.

.. _west-apis-log:

west.log
********

.. automodule:: west.log
   :members: set_verbosity, VERBOSE_NONE, VERBOSE_NORMAL, VERBOSE_VERY, VERBOSE_EXTREME, dbg, inf, wrn, err, die, banner, small_banner

.. _west-apis-manifest:

west.manifest
*************

.. automodule:: west.manifest

.. autodata:: MANIFEST_PROJECT_INDEX

.. autodata:: MANIFEST_REV_BRANCH

.. autodata:: QUAL_MANIFEST_REV_BRANCH

.. autofunction:: west.manifest.manifest_path

.. autoclass:: west.manifest.Manifest

   .. automethod:: from_file

   .. automethod:: from_data

   .. automethod:: __init__

   .. automethod:: get_remote

   .. automethod:: get_projects

   .. versionadded:: 0.6.1

   .. automethod:: as_frozen_dict

.. autoclass:: west.manifest.Defaults
   :members:
   :member-order: groupwise

.. autoclass:: west.manifest.Remote
   :members:
   :member-order: groupwise

.. autoclass:: west.manifest.Project

   .. automethod:: __init__

   .. automethod:: as_dict

   .. automethod:: format

   .. automethod:: git

   .. versionchanged:: 0.6.1
      The ``capture_stderr`` kwarg.

   .. automethod:: sha

   .. automethod:: is_ancestor_of

   .. automethod:: is_cloned

   .. versionadded:: 0.6.1

   .. automethod:: is_up_to_date_with

   .. automethod:: is_up_to_date

.. autoclass:: west.manifest.ManifestProject
   :members:
   :member-order: groupwise

.. versionadded:: 0.6.0

.. autoclass:: west.manifest.MalformedManifest
   :show-inheritance:

.. autoclass:: west.manifest.MalformedConfig
   :show-inheritance:

.. _west-apis-util:

west.util
*********

.. canon_path(), escapes_directory(), etc. intentionally not documented here.

.. automodule:: west.util

.. autofunction:: west.util.west_dir

.. autofunction:: west.util.west_topdir

.. autoclass:: west.util.WestNotFound
   :show-inheritance:

.. _west #38:
   https://github.com/zephyrproject-rtos/west/issues/38
