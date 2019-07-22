:orphan:

.. _west-apis:

West and Extension APIs
#######################

This page documents the Python APIs provided by :ref:`west <west>`, as well as
some additional APIs used by the :ref:`west extensions <west-extensions>` in
the zephyr repository.

**Contents**:

.. contents::
   :local:

.. _west-apis-west:

West
****

This section contains documentation for west's APIs.

.. warning::

   These APIs should be considered unstable until west version 1.0. Further,
   until `west #38`_ is closed, these modules can only be imported from
   extension command files (and within west itself, of course).

.. NOTE: documentation authors:

   1. keep these sorted by package/module name.
   2. if you add a :ref: target here, add it to west-not-found.rst too.

.. _west-apis-commands:

west.commands
=============

.. automodule:: west.commands
   :members: WestCommand, CommandError, CommandContextError, ExtensionCommandError

.. _west-apis-configuration:

west.configuration
==================

.. automodule:: west.configuration
   :members: ConfigFile, read_config, update_config

.. _west-apis-log:

west.log
========

.. automodule:: west.log
   :members: set_verbosity, VERBOSE_NONE, VERBOSE_NORMAL, VERBOSE_VERY, VERBOSE_EXTREME, dbg, inf, wrn, err, die

.. _west-apis-manifest:

west.manifest
=============

.. automodule:: west.manifest
   :members: manifest_path, Manifest, Defaults, Remote, Project, SpecialProject, MalformedManifest, MalformedConfig, MANIFEST_SECTIONS, MANIFEST_PROJECT_INDEX, MANIFEST_REV_BRANCH, QUAL_MANIFEST_REV_BRANCH

.. _west-apis-util:

west.util
=========

.. automodule:: west.util
   :members: west_dir, west_topdir, WestNotFound

.. _west #38:
   https://github.com/zephyrproject-rtos/west/issues/38
