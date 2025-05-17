#!/usr/bin/env python3
#
# Copyright (c) 2025 Baumer Group. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import json
import os
import shlex
import sys
from logging import Logger

_FILTER_SEPARATOR = ';'

_FILTER_DICT_PATH = 'filter_file_path'
_FILTER_DICT_ARGS = 'args'
_FILTER_DICT_KWARGS = 'kwargs'


class _FilterDictionary:
    path: str = None
    args: list = None
    kwargs: dict = None

    def __init__(self, path, args:list=None, kwargs:dict=None):
        self.path = path

        self.args = args if isinstance(args, list) else []
        self.kwargs = kwargs if isinstance(kwargs, dict) else {}

    def to_dictionary(self) -> dict:
        return {
            _FILTER_DICT_PATH: self.path,
            _FILTER_DICT_ARGS: self.args,
            _FILTER_DICT_KWARGS: self.kwargs
        }


def _handle_simple_filter(filter_args, _logger: Logger=None):
    filter_list = []
    current_filter = None

    filter_block_start = True
    filter_block_end = False

    for args_wrapper in filter_args:
        for token in shlex.split(args_wrapper, posix=False):
            filter_block_end = token.endswith(_FILTER_SEPARATOR)
            clean_token = token.removesuffix(_FILTER_SEPARATOR)

            if filter_block_start:
                current_filter = _FilterDictionary(clean_token)
                filter_list.append(current_filter)
                filter_block_start = False
            else:
                if (
                    '=' in clean_token and 
                    not (clean_token.startswith('"') or clean_token.startswith("'"))
                ):
                    key, value = clean_token.split('=', 1)

                    if key in current_filter.kwargs:
                        _logger.info(
                            f"Key { key } already registered for filter { current_filter.path }. "
                            f"Value will be updated from  { current_filter.kwargs[key] } "
                            f"to { value }."
                        )

                    current_filter.kwargs[key] = value
                else:
                    current_filter.args.append(clean_token)

            if filter_block_end:
                filter_block_start = True

    return [ filter_obj.to_dictionary() for filter_obj in filter_list ]


def _handle_json_string_filter(filter_args, _logger: Logger=None):
    return json.loads(filter_args)


def _handle_json_path_filter(filter_args, _logger: Logger):
    if not os.path.exists(filter_args):
        _logger.error(
            f"Provided path { filter_args } doesn't exist in the system "
            f"or in the current working directory { os.getcwd() }. "
            "Halting twister process..."
        )
        sys.exit(1)

    if not os.path.isfile(filter_args):
        _logger.error(
            f"Provided path { filter_args } doesn't lead to a file. "
            "Halting twister process..."
        )
        sys.exit(1)

    try:
        with open(filter_args, encoding='UTF-8') as json_file:
            return json.load(json_file)
    except OSError as e:
        _logger.error(
            f"JSON parser crashed with error { e } when trying to parse { filter_args }. "
            "Halting twister process..."
        )
        sys.exit(1)


def _handle_json_filter(filter_args, logger: Logger, handle_method):
    if callable(handle_method):
        try:
            return handle_method(filter_args, logger)
        except (json.JSONDecodeError, TypeError, ValueError) as e:
            logger.error(
                f"JSON parser crashed with error { e } when trying to parse { filter_args }. "
                "Halting twister process..."
            )
            sys.exit(1)
    else:
        logger.error(
            "Handle JSON Filter method was fed an uncallable handle method. "
            "Halting twister process..."
        )
        sys.exit(1)


def parse_arguments(options, logger: Logger):
    if filter_args := options.plugin_filter:
        options.plugin_filter = _handle_simple_filter(filter_args, logger)
    elif filter_args := options.plugin_filter_json_string:
        options.plugin_filter = _handle_json_filter(
            filter_args, logger, _handle_json_string_filter
        )
    elif filter_args := options.plugin_filter_json_path:
        options.plugin_filter = _handle_json_filter(
            filter_args, logger, _handle_json_path_filter
        )
