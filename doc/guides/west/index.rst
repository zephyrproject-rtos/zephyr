.. _west:

West (Zephyr's meta-tool)
#########################

The Zephyr project includes a swiss-army knife command line tool
named ``west`` (Zephyr is an English name for the Latin
`Zephyrus <https://en.wiktionary.org/wiki/Zephyrus>`_, the ancient Greek god
of the west wind).

West is used upstream to obtain the source code for the Zephyr project and can
also be used to build, debug, and flash applications. It is developed in its
own `repository on GitHub`_. The source code retrieval features include a
multiple repository management system with features inspired by Google's Repo
tool and Git submodules.

West is also pluggable: you can write your own west "extension commands" to add
additional features. Extension commands can be in any directory in your
installation; they don't have to be defined in the zephyr or west repositories.

Like :program:`git` and :program:`docker`, the top-level :program:`west`
command takes some options, a sub-command to run, and then options specific to
that sub-command::

  west [common-opts] <command-name> [command-opts] [<command-args>]

After you've :ref:`created a Zephyr installation using west <getting_started>`,
you can run ``west --help`` (or ``west -h`` for short) to get top-level help on
west's built-in commands along with any extension commands available in your
installation.

The following pages describe how to use west, and provide additional context
about the tool.

.. toctree::
   :maxdepth: 1

   repo-tool.rst
   build-flash-debug.rst
   why.rst
   without-west.rst
   planned.rst

For details on west's Python APIs (including APIs provided by extensions in the
zephyr), see :ref:`west-apis`.

.. _repository on GitHub:
   https://github.com/zephyrproject-rtos/west
