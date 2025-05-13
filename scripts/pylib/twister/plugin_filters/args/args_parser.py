#!/usr/bin/env python3
#
# Copyright (c) 2025 Baumer Group. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import json
import logging
import os
import shlex
import sys

_FILTER_SEPARATOR = ';'

_FILTER_DICT_PATH = 'filter_file_path'
_FILTER_DICT_ARGS = 'args'
_FILTER_DICT_KWARGS = 'kwargs'

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)


class _FilterDictionary:
    path: str
    args: list
    kwargs: dict

    def __init__(self, path, args: list | None = None, kwargs: dict | None = None):
        self.path = path

        self.args = args if isinstance(args, list) else []
        self.kwargs = kwargs if isinstance(kwargs, dict) else {}

    def to_dictionary(self) -> dict:
        return {
            _FILTER_DICT_PATH: self.path,
            _FILTER_DICT_ARGS: self.args,
            _FILTER_DICT_KWARGS: self.kwargs,
        }


def _handle_filter_token(token, block_starts: bool, filter_list: list[_FilterDictionary]) -> bool:
    next_block_starts = token.endswith(_FILTER_SEPARATOR)
    clean_token = token.removesuffix(_FILTER_SEPARATOR)

    if block_starts:
        filter_list.append(_FilterDictionary(clean_token))

    elif len(filter_list) > 0 and (current_filter := filter_list[-1]) is not None:
        if '=' in clean_token and not (clean_token.startswith('"') or clean_token.startswith("'")):
            key, value = clean_token.split('=', 1)

            if key in current_filter.kwargs:
                logger.warning(
                    "Kwarg key %s already registered for filter %s. "
                    "Value will be updated from %s to %s.",
                    key,
                    current_filter.path,
                    current_filter.kwargs[key],
                    value,
                )

            current_filter.kwargs[key] = value
        else:
            current_filter.args.append(clean_token)

    return next_block_starts


def _handle_simple_filter(filter_args):
    filter_list = []
    next_block_starts = True

    for args_wrapper in filter_args:
        for token in shlex.split(args_wrapper, posix=False):
            next_block_starts = _handle_filter_token(token, next_block_starts, filter_list)

    return [filter_obj.to_dictionary() for filter_obj in filter_list]


def _handle_json_string_filter(filter_args):
    return json.loads(filter_args)


def _handle_json_path_filter(filter_args):
    if not os.path.exists(filter_args):
        logger.error(
            "Provided path %s doesn't exist in the system "
            "or in the current working directory %s. "
            "Halting twister process...",
            filter_args,
            os.getcwd(),
        )
        sys.exit(1)

    if not os.path.isfile(filter_args):
        logger.error(
            "Provided path %s doesn't lead to a file. Halting twister process...", filter_args
        )
        sys.exit(1)

    try:
        with open(filter_args, encoding='UTF-8') as json_file:
            return json.load(json_file)
    except OSError as e:
        logger.error(
            "JSON parser crashed with error %s while trying to parse %s. "
            "Halting twister process...",
            e,
            filter_args,
        )
        sys.exit(1)


def _handle_json_filter(filter_args, handle_method):
    if callable(handle_method):
        try:
            return handle_method(filter_args, logger)
        except (TypeError, ValueError) as e:
            logger.error(
                "JSON parser crashed with error %s while trying to parse %s. "
                "Halting twister process...",
                e,
                filter_args,
            )
            sys.exit(1)
    else:
        logger.error(
            "Handle JSON Filter method was fed an uncallable handle method. "
            "Halting twister process..."
        )
        sys.exit(1)


def parse_arguments(options):
    if filter_args := options.plugin_filter:
        options.plugin_filter = _handle_simple_filter(filter_args)
    elif filter_args := options.plugin_filter_json_string:
        options.plugin_filter = _handle_json_filter(filter_args, _handle_json_string_filter)
    elif filter_args := options.plugin_filter_json_path:
        options.plugin_filter = _handle_json_filter(filter_args, _handle_json_path_filter)
