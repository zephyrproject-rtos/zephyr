.. _pluginFilterDocu:

Test Runner (Twister)
#####################

Twister works only with static information found in files such as testcase.yaml and samples.yaml.
Filtering on dynamic information, for example an analysis of which files have changed compared to
the main branch, is not natively supported.

The Plugin Filter system has been developed to introduce this feature. 
The implementation lets developers write filtering scripts, which will be later used to select
only the desired Test Suites. Based on its usage, this approach potentially reduces the CI load
for tests and static code analysis.

Usage
*****

This section will explain how to use the plugin with filters that have already been implemented.
To understand how to create a filter from scratch,
read the :ref:`Filter Creation <_pluginFilterCreation>` page.

#.  Pathing

    Every filter command requires a path to the .py file where the filter is located.
    To facilitate this part, an environmental variable called 
    **TWISTER_PLUGIN_FILTER_ROOTS** has been created.
    This variable contains a list of strings, all representing system paths.
    The backend will try to combine these paths and the filter name
    until a valid instance is found or no more combinations are possible.

    In order to call a plugin filter without issues, the first step is the establishment
    of the correct path. Two types of paths are supported: system and python paths. 
    If the required filter is located in a subdirectory of *``scripts/pylib/twister/``*,
    python paths can be used, otherwise normal system paths are the only option.

    Examples:

    *   The file ``filter_id_regex.py`` contains the desired filter and is located under
        *``zephyr/scripts/pylib/twister/plugin_filters/filters``*.
        Since ``plugin_filters.filters`` is considered the default filter directory,
        it is (and will always be) automatically included in the **ROOTS** env. variable.
        This means that the filter path can be limited to ``filter_id_regex``.
    *   A hypotetical ``hello_world_filter.py`` filter located under
        *``zephyr/scripts/pylib/twister/plugin_filters/filters/subdirectory``*
        wouldn't need additions to the **ROOTS** list either,
        but its path should mention the subdirectory, meaning the filter
        path would become ``subdirectory.hello_world_filter``.
    *   When filters are stored outside of the region reachable by relative paths,
        normal ones are needed. Filtering with the file ``foo_bar_filter.py``,
        located under *``/home/user/desktop/my_filters/subdir/foo_bar_filter.py``*,
        would require the addition of *``/home/user/desktop/my_filters/``*
        to the **ROOTS** and the usage of ``subdir/foo_bar_filter.py`` in the filter path.

#.  Arguments

    Once the path is clear, args and kwargs must be defined. The easiest way to establish
    which ones are needed is reading the setup method of the filter. A well-written filter should
    describe its parameters with suitable names and avoid dynamically retrieving them from the
    ``*args`` list or ``**kwargs`` dictionary.

    Example:

    *   The setup method of ``filter_id_regex.py`` is the following:

        .. code-block:: python

            def setup(self, regex_pattern: str, *args, **kwargs):
                super().setup(*args, **kwargs)

                self.regex_pattern = regex_pattern

        This shows that ``filter_id_regex.py`` requires one positional argument, a string.
        The method also calls its implementation in the super class. This allows the assignment
        of exclude and include reasons without adding any code duplication.

#. Twister call

    Once path and arguments are clear, the actual command can be built. 
    There are three plugin-filter variants that can be called:

.. _pluginFilterCommandsPG:

    *   **--plugin-filter**, accepts a string containing filter data.
        This string must be made out of "blocks" separated by semicolons
        where each block corresponds to a filter and its parameters.
        A block must always start with the path to the filter,
        followed by the filter's args and kwargs (if required).

        Examples:

        .. code-block:: bash
            scripts/twister -T samples/philosophers --plugin-filter filter_id_regex '.*static'
            scripts/twister -T samples/philosophers --plugin-filter filter_id_regex regex_pattern='.*static' exclude_reason='not from static group' include_reason='from static group'
            scripts/twister -T samples/philosophers --plugin-filter filter_id_regex '.*static'; filter_id_regex '.*twister'
            scripts/twister -T samples/philosophers --plugin-filter filter_id_regex.py '.*static'
            scripts/twister -T samples/philosophers --plugin-filter filter_id_regex.py regex_pattern='.*static' exclude_reason='not from static group' include_reason='from static group'
            scripts/twister -T samples/philosophers --plugin-filter filter_id_regex.py '.*static'; filter_id_regex '.*twister'

.. _pluginFilterCommandsPGJS:

    *   **--plugin-filter-json-string**, accepts a json-formatted string containing filter data.
        This string must be made out of a list with one or more dictionaries,
        each with their filter_file_path property (the path to the filter) and,
        if required, the filter's args and / or kwargs.

        Examples:

        .. code-block:: bash
            scripts/twister -T samples/philosophers --plugin-filter-json-string '[{"filter_file_path": "filter_id_regex", "args": [".*static"]}]'
            scripts/twister -T samples/philosophers --plugin-filter-json-string '[{"filter_file_path": "filter_id_regex", "kwargs": {"regex_pattern": ".*static", "exclude_reason": "not from static group", "include_reason": "from static group"}}]'
            scripts/twister -T samples/philosophers --plugin-filter-json-string '[{"filter_file_path": "filter_id_regex.py", "args": [".*static"]}]'
            scripts/twister -T samples/philosophers --plugin-filter-json-string '[{"filter_file_path": "filter_id_regex.py", "kwargs": {"regex_pattern": ".*static", "exclude_reason": "not from static group", "include_reason": "from static group"}}]'

.. _pluginFilterCommandsPGJP:

    *   **--plugin-filter-json-path**, accepts a path pointing to a json file
        containing filter data. The data in said file must follow the same guidelines
        as the string in --plugin-filter-json-string.

        Examples:

        .. code-block:: bash
            scripts/twister -T samples/philosophers --plugin-filter-json-path 'scripts/pylib/twister/plugin_filters/filters/filter_id_regex_example_conf.json'
            scripts/twister -T samples/philosophers --plugin-filter-json-path '/home/user/filter_id_regex_example_conf.json'

.. note::

    Any positional argument can be passed as a keyword argument,
    as long as the parameter name matches. The reverse is not allowed.

.. note::

    Each of the previous examples was successfully tested from the working directory
    ``~/zephyrproject/zephyr`` and with the twister python venv active.

.. note::

    Kwargs ``exclude_reason`` and ``include_reason``, although not mandatory, are highly suggested.
