.. _pluginFilterDocu:

Plugin Filter to Twister
########################

Twister works only with static information found in files such as testcase.yaml and samples.yaml.
The Plugin Filter system gives to the developers the necessary tools for creating custom filters,
potentially reducing the CI load for tests and static code analysis.

Usage
*****

This section will explain how to use the plugin with filters that have already been implemented.
To understand how to create a filter from scratch, consult the corresponding documentation under
the :ref:`Filter Creation <pluginFilterCreation>` page.

TL;DR - Twister call
====================

JSON Structure
--------------

    Two out of the three plugin filter commands use a specific .json format.
    The basic rules for an error-free execution are the following:

    *   The outermost layer of the .json must be a list of dictionaries.
    *   Each dictionary must contain one to three values:
        *   *filter_file_path* - A string, representing the path to the filter.
        *   *args* - A list of strings, representing the positional arguments of the filter.
        *   *kwargs* - A dictionary with string key-value pairs, representing the keyword arguments
            of the filter.

    Example:

    .. code-block:: json

        [
            {
                "filter_file_path": "filter_id_regex",
                "args": [
                    ".*static"
                ]
            }
        ]

Plugin filter variants
----------------------

    Three plugin-filter variations can be called:

.. _pluginFilterCommandsPGJP:

    *   **--plugin-filter-json-path**, accepts a path pointing to a json containing filter data.

        Notation:

        .. code-block:: bash

            scripts/twister -T samples/philosophers --plugin-filter-json-path <path-to-json-file>

        Examples:

        .. code-block:: bash

            scripts/twister -T samples/philosophers --plugin-filter-json-path 'scripts/pylib/twister/plugin_filters/filters/filter_id_regex_example_conf.json'
            scripts/twister -T samples/philosophers --plugin-filter-json-path '/home/user/filter_id_regex_example_conf.json'

.. _pluginFilterCommandsPG:

    *   **--plugin-filter**, accepts a string containing filter data.
        This string must be made out of "blocks" separated by semicolons,
        where each block corresponds to a filter and its parameters.
        A block must always start with the path to the filter,
        followed by the filter's args and kwargs (if required).

        Notation:

        .. code-block:: bash

            scripts/twister -T samples/philosophers --plugin-filter <[subpath/]plugin-filter-name-1> <arg-1> <arg-N> <kwarg-key-1>=<kwarg-value-1> <kwarg-key-n>=<kwarg-value-n>; <[subpath/]plugin-filter-name-n> <arg-1> <arg-N> <kwarg-key-1>=<kwarg-value-1> <kwarg-key-n>=<kwarg-value-n>

        Examples:

        .. code-block:: bash

            scripts/twister -T samples/philosophers --plugin-filter filter_id_regex '.*static'
            scripts/twister -T samples/philosophers --plugin-filter filter_id_regex regex_pattern='.*static' exclude_reason='not from static group' include_reason='from static group'
            scripts/twister -T samples/philosophers --plugin-filter filter_id_regex '.*static'; filter_id_regex '.*twister'

.. _pluginFilterCommandsPGJS:

    *   **--plugin-filter-json-string**, accepts a json-formatted string containing filter data.

        Notation:

        .. code-block:: bash

            scripts/twister -T samples/philosophers --plugin-filter-json-string <json-string>

        Examples:

        .. code-block:: bash

            scripts/twister -T samples/philosophers --plugin-filter-json-string '[{"filter_file_path": "filter_id_regex", "args": [".*static"]}]'
            scripts/twister -T samples/philosophers --plugin-filter-json-string '[{"filter_file_path": "filter_id_regex", "kwargs": {"regex_pattern": ".*static", "exclude_reason": "not from static group", "include_reason": "from static group"}}]'
            scripts/twister -T samples/philosophers --plugin-filter-json-string '[{"filter_file_path": "filter_id_regex.py", "args": [".*static"]}]'
            scripts/twister -T samples/philosophers --plugin-filter-json-string '[{"filter_file_path": "filter_id_regex.py", "kwargs": {"regex_pattern": ".*static", "exclude_reason": "not from static group", "include_reason": "from static group"}}]'

    .. note::

        Any positional argument can be passed as a keyword argument,
        as long as the parameter name matches. The reverse is not allowed.

    .. note::

        Kwargs ``exclude_reason`` and ``include_reason``, although not mandatory, are highly suggested.

Pathing
=======

    Two formats are supported: system (path/to/the/filter) and python (path.to.the.filter) paths.

    .. note::

        python paths can **only** be used for subdirectories of *``scripts/pylib/twister/``*

    To avoid long paths, the script searches filters only inside the
    ``zephyr/scripts/pylib/twister/plugin_filters/filters`` directory.

    If additional filters are located elsewhere, an environmental variable called
    **TWISTER_PLUGIN_FILTER_ROOTS** can be defined, effectively expanding the search range of the
    function.

    To set this variable, insert all the desired paths (separated by semicolons) in a string
    and run the following snippet in the console:

    .. code-block:: bash

        export TWISTER_PLUGIN_FILTER_ROOTS="<path-1>;<path-n>"

    Once the roots have been exported, twister calls in the same terminal instance will
    automatically scan the specified directories and select the correct filter implementations.

    .. note::

        Paths in **TWISTER_PLUGIN_FILTER_ROOTS** can use both the system and the python format.

    .. note::

        The usage of semicolons as separators is not hard-coded. If desired, the environmental variable
        **TWISTER_PLUGIN_FILTER_ROOTS_SEPARATOR** will allow the selection of an alternative symbol.

    Filter paths - examples:

    *   The file ``filter_id_regex.py`` is located under
        *``zephyr/scripts/pylib/twister/plugin_filters/filters``*.
        Since ``plugin_filters.filters`` is considered the default filter directory,
        the filter path can be limited to ``filter_id_regex``.
    *   A hypotetical ``hello_world_filter.py`` filter located under
        *``zephyr/scripts/pylib/twister/plugin_filters/filters/subdirectory``*
        wouldn't need additions to the **ROOTS** list either,
        but its path should mention the subdirectory, meaning the filter
        path would become ``subdirectory.hello_world_filter``.
    *   When filters are stored outside of the region reachable by python paths,
        system ones are needed. Filtering with the file ``foo_bar_filter.py``,
        located under *``/home/user/desktop/my_filters/subdir/foo_bar_filter.py``*,
        would require the addition of *``/home/user/desktop/my_filters/``*
        to the **ROOTS** and the usage of ``subdir/foo_bar_filter.py`` in the filter path.

Arguments
=========

    Once the path is clear, args and kwargs must be specified. The easiest way to understand
    which ones are needed is to read the setup method of the filter. A well-written filter should
    describe its parameters with suitable names and avoid dynamically retrieving them from the
    ``*args`` list or ``**kwargs`` dictionary.

    Example:

    *   The setup method of ``filter_id_regex.py`` has the following implementation:

        .. code-block:: python

            def setup(self, regex_pattern: str, *args, **kwargs):
                super().setup(*args, **kwargs)

                self.regex_pattern = regex_pattern

        This shows that ``filter_id_regex.py`` requires one positional argument, a string.
        Inspecting the super class would reveal two more arguments, exclude_reason and
        include_reason. These are not mandatory, but highly suggested. The code above could
        therefore be improved by referencing them directly, instead of forwarding the kwargs
        dictionary.
