Snippets Design
###############

This page documents design goals for the snippets feature.
Further information can be found in `Issue #51834`_.

.. _Issue #51834: https://github.com/zephyrproject-rtos/zephyr/issues/51834

- **extensible**: for example, it is possible to add board support for an
  existing built-in snippet without modifying the zephyr repository

- **composable**: it is possible to use multiple snippets at once, for example
  using:

  .. code-block:: console

     west build -S <snippet1> -S <snippet2> ...

- **able to combine multiple types of configuration**: snippets make it possible
  to store multiple different types of build system settings in one place, and
  apply them all together

- **specializable**: for example, it is possible to customize a snippet's
  behavior for a particular board, or board revision

- **future-proof and backwards-compatible**: arbitrary future changes to the
  snippets feature will be possible without breaking backwards compatibility
  for older snippets

- **applicable to purely "software" changes**: unlike the shields feature,
  snippets do not assume the presence of a "daughterboard", "shield", "hat", or
  any other type of external assembly which is connected to the main board

- **DRY** (don't repeat yourself): snippets allow you to skip unnecessary
  repetition; for example, you can apply the same board-specific configuration
  to boards ``foo`` and ``bar`` by specifying ``/(foo|bar)/`` as a regular
  expression for the settings, which will then apply to both boards
