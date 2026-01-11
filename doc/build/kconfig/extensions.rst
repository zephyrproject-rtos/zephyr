.. _kconfig_extensions:

Kconfig extensions
##################

Zephyr uses the `Kconfiglib <https://github.com/zephyrproject-rtos/Kconfiglib>`__
implementation of `Kconfig
<https://docs.kernel.org/kbuild/kconfig-language.html>`__,
which includes some Kconfig extensions:

- Default values can be applied to existing symbols without
  :ref:`weakening <multiple_symbol_definitions>` the symbols dependencies
  through the use of ``configdefault``.

  .. code-block:: none

      config FOO
          bool "FOO"
          depends on BAR

      configdefault FOO
          default y if FIZZ

  The statement above is equivalent to:

  .. code-block:: none

      config FOO
          bool "Foo"
          default y if FIZZ
          depends on BAR

  ``configdefault`` symbols cannot contain any fields other than ``default``,
  however they can be wrapped in ``if`` statements. The two statements below
  are equivalent:

  .. code-block:: none

      configdefault FOO
          default y if BAR

      if BAR
      configdefault FOO
          default y
      endif # BAR

- Environment variables in ``source`` statements are expanded directly, meaning
  no "bounce" symbols with ``option env="ENV_VAR"`` need to be defined.

  .. note::

     ``option env`` has been removed from the C tools as of Linux 4.18 as well.

  The recommended syntax for referencing environment variables is ``$(FOO)``
  rather than ``$FOO``. This uses the new `Kconfig preprocessor
  <https://docs.kernel.org/kbuild/kconfig-macro-language.html>`__.
  The ``$FOO`` syntax for expanding environment variables is only supported for
  backwards compatibility.

- The ``source`` statement supports glob patterns and includes each matching
  file. A pattern is required to match at least one file.

  Consider the following example:

  .. code-block:: kconfig

      source "foo/bar/*/Kconfig"

  If the pattern ``foo/bar/*/Kconfig`` matches the files
  :file:`foo/bar/baz/Kconfig` and :file:`foo/bar/qaz/Kconfig`, the statement
  above is equivalent to the following two ``source`` statements:

  .. code-block:: kconfig

      source "foo/bar/baz/Kconfig"
      source "foo/bar/qaz/Kconfig"

  If no files match the pattern, an error is generated.

  The wildcard patterns accepted are the same as for the Python `glob
  <https://docs.python.org/3/library/glob.html>`__ module.

  For cases where it's okay for a pattern to match no files (or for a plain
  filename to not exist), a separate ``osource`` (*optional source*) statement
  is available. ``osource`` is a no-op if no file matches.

  .. note::

      ``source`` and ``osource`` are analogous to ``include`` and
      ``-include`` in Make.

- An ``rsource`` statement is available for including files specified with a
  relative path. The path is relative to the directory of the :file:`Kconfig`
  file that contains the ``rsource`` statement.

  As an example, assume that :file:`foo/Kconfig` is the top-level
  :file:`Kconfig` file, and that :file:`foo/bar/Kconfig` has the following
  statements:

  .. code-block:: kconfig

      source "qaz/Kconfig1"
      rsource "qaz/Kconfig2"

  This will include the two files :file:`foo/qaz/Kconfig1` and
  :file:`foo/bar/qaz/Kconfig2`.

  ``rsource`` can be used to create :file:`Kconfig` "subtrees" that can be
  moved around freely.

  ``rsource`` also supports glob patterns.

  A drawback of ``rsource`` is that it can make it harder to figure out where a
  file gets included, so only use it if you need it.

- An ``orsource`` statement is available that combines ``osource`` and
  ``rsource``.

  For example, the following statement will include :file:`Kconfig1` and
  :file:`Kconfig2` from the current directory (if they exist):

  .. code-block:: kconfig

      orsource "Kconfig[12]"

- ``def_int``, ``def_hex``, and ``def_string`` keywords are available,
  analogous to ``def_bool``. These set the type and add a ``default`` at the
  same time.

- A symbol name can be associated to ``choice`` groups using the ``choice <symbol>`` syntax.
  Such choices, called *named choices*, can be modified from places other than their initial
  definition. For example, the following statements define the named choice ``FOOBAR``:

  .. code-block:: kconfig

    choice FOOBAR
	    prompt "Example choice"
	    default BAR

    config FOO
	    bool "Foo"

    config BAR
	    bool "Bar"

    endchoice

  The following statements could then be used, in a ``Kconfig.defconfig`` file for example,
  to override the default option of choice ``FOOBAR``:

  .. code-block:: kconfig

    # Note how "prompt" is not present here
    choice FOOBAR
        default FOO
    endchoice

  .. note::
    The *named choices* feature originates from Linux, but it is no longer supported in Linux
    since kernel release 6.9, and has thus become a Kconfiglib language extension.
