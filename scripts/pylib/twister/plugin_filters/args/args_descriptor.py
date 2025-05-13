#!/usr/bin/env python3
#
# Copyright (c) 2025 Baumer Group. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import os
from argparse import _ArgumentGroup

_GENERAL_FILTER_DESCRIPTION = (
    "Allows the selection of one (or multiple) filter(s) "
    "to reduce the amount of tests to perform on twister launch. "
)

_COMMAND_DELIMITERS_WARNING = (
    "(< and > are present only for delimiting the command, "
    "do not include them in the actual call). "
)

_GENERAL_JSON_FILTER_DESCRIPTION = (
    "The names of these filters, along with their arguments, will be parsed from a json "
)

_JSON_STRING_FILTER_DESCRIPTION = _GENERAL_JSON_FILTER_DESCRIPTION + "string. "
_JSON_PATH_FILTER_DESCRIPTION = (
    _GENERAL_JSON_FILTER_DESCRIPTION
    + "file. "
    + "The path to said file can be either absolute or relative. "
    + f"The root for relative paths is the current working directory '{os.getcwd()}'. "
)

_BASIC_FILTER_FORMAT = (
    "The argument should have one of the two following formats: "
    + "< --plugin-filter 'filter_file_path_1 arg1 arg2 kwarg1=1234 kwarg2=5678; "
    + "filter_file_path_2 arg3' >, or alternatively "
    + "< --plugin-filter filter_file_path_1 arg1 arg2 kwarg1=1234 kwarg2=5678\\; "
    + "filter_file_path_2 arg3 > "
    + _COMMAND_DELIMITERS_WARNING
    + "In both cases spaces serve as a separator for the parameters, "
    + "while semicolons are used to mark the end of a filter and its data, "
    + "allowing the user to append a new one. "
)

_JSON_FILTER_FORMAT = (
    "The JSON should comply with the following structural guidelines: "
    "The outermost layer must be a list, "
    "which is used to contain one or more dictionaries with the filter data. "
    "Each dictionary must contain a 'filter_file_path' key-value pair. "
    "This variable specifies the relative path to the file where the filter is located "
    "(the absolute path is generated separately by iterating through "
    "the TWISTER_PLUGIN_FILTER_ROOTS environmental variable). "
    "The dictionary allows to specify an 'args' and a 'kwargs' property to feed to the filter. "
    "If used, the args key must be linked to a list, while kwargs must be linked to a dictionary. "
)

_JSON_STRING_FILTER_EXAMPLE = (
    "Examples: "
    "without arguments: "
    "< --plugin-filter-json-string '[{\"filter_file_path\": \"skip_matching_id\"}]' >, "
    "with the usage of kwargs: "
    "< --plugin-filter-json-string "
    "'[{"
    "\"filter_file_path\": \"skip_matching_id\", "
    "\"kwargs\": {\"id_filter\": \"sample.kernel.philosopher.semaphores\"}"
    "}]' "
    ">, "
    "and with both args and kwargs: "
    "< --plugin-filter-json-string "
    "'[{"
    "\"filter_file_path\": \"skip_matching_id\", "
    "\"args\": [123, 456], "
    "\"kwargs\": {\"id_filter\": \"sample.kernel.philosopher.semaphores\"}"
    "}]' "
    ">. " + _COMMAND_DELIMITERS_WARNING
)

_JSON_PATH_FILTER_EXAMPLE = (
    "Examples: "
    "< --plugin-filter-json-path filter_details.json > "
    "(only for .json files located inside the current working directory tree), or also "
    "< --plugin-filter-json-path '/home/user/filter_configs/filter_details.json' > "
    + _COMMAND_DELIMITERS_WARNING
)

_GENERAL_ROOT_NOTES = (
    "NOTE: "
    "In order to find the correct filter file, "
    "the script must be provided with a list of root directories. "
    "This list can be defined by creating a new environmental variable, "
    "named 'TWISTER_PLUGIN_FILTER_ROOTS'. "
    "This variable will be used to iterate through the specified roots "
    "until the required filter_file_path is found. "
    "An example for said variable could be: ['/home/user/external_filters']. "
    "Although valid, this array will still undergo a slight modification: "
    "The automatic addition of the 'plugin_filters' root at the end of the list. "
    "This is done because this directory contains "
    "both the filter framework and the regex filter example. "
    "Roots can either be normal ('/home/user/external_filters') "
    "or pythonic paths ('myfilters.filters'). "
    "Although both can be used, "
    "the format of filter_file_path must always correspond to the one of its root. "
    "For example, if the root is 'plugin_filters', "
    "valid filter_file_paths would be 'file_name1' or 'sub_dir.file_name2'. "
    "On the other hand, a root such as '/home/user/external_filters'"
    " requires 'file_name1.py' or 'dir/file_name2.py'. "
)


def add_arguments(case_select: _ArgumentGroup):
    plugin_filter_mex_group = case_select.add_mutually_exclusive_group()

    plugin_filter_mex_group.add_argument(
        "--plugin-filter",
        nargs="+",
        help=(_GENERAL_FILTER_DESCRIPTION + _BASIC_FILTER_FORMAT + _GENERAL_ROOT_NOTES),
    )

    plugin_filter_mex_group.add_argument(
        "--plugin-filter-json-string",
        help=(
            _GENERAL_FILTER_DESCRIPTION
            + _JSON_STRING_FILTER_DESCRIPTION
            + _JSON_FILTER_FORMAT
            + _JSON_STRING_FILTER_EXAMPLE
            + _GENERAL_ROOT_NOTES
        ),
    )

    plugin_filter_mex_group.add_argument(
        "--plugin-filter-json-path",
        help=(
            _GENERAL_FILTER_DESCRIPTION
            + _JSON_PATH_FILTER_DESCRIPTION
            + _JSON_FILTER_FORMAT
            + _JSON_PATH_FILTER_EXAMPLE
            + _GENERAL_ROOT_NOTES
        ),
    )
