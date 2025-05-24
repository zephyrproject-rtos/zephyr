#!/usr/bin/env python3
#
# Copyright (c) 2025 Baumer Group. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import importlib
import importlib.util
import inspect
import os
import sys
from importlib.util import spec_from_file_location
from typing import Optional

from plugin_filters.filter_interface import FilterInterface
from twisterlib.statuses import TwisterStatus

_DEFAULT_ROOT_NORMAL = './scripts/pylib/twister/plugin_filters/filters'
_DEFAULT_ROOT_PYTHONIC = 'plugin_filters.filters'
_ROOTS_ENV_VARIABLE_NAME = "TWISTER_PLUGIN_FILTER_ROOTS"


def _get_root_list():
    root_list = os.getenv(_ROOTS_ENV_VARIABLE_NAME)

    if root_list is None:
        root_list = []
    elif not isinstance(root_list, list):
        sys.exit("$The TWISTER_PLUGIN_FILTER_ROOTS environment variable must be a list")

    if _DEFAULT_ROOT_PYTHONIC not in root_list:
        root_list.append(_DEFAULT_ROOT_PYTHONIC)

    if _DEFAULT_ROOT_NORMAL not in root_list:
        root_list.append(_DEFAULT_ROOT_NORMAL)

    return root_list


def _get_class(file_name, file_path, logger, class_name='Filter'):
    class_reference = None

    try:
        if (
            os.path.exists(class_path := os.path.join(file_path, file_name)) and
            (module_specification := spec_from_file_location(class_name, class_path)) and
            (module := importlib.util.module_from_spec(module_specification))
        ):
            module_specification.loader.exec_module(module)
            class_reference = getattr(module, class_name, None)

        if class_reference is None:
            import_string = f"{ file_path }.{ file_name }"

            try:
                if importlib.util.find_spec(import_string):
                    class_reference = getattr(
                        importlib.import_module(import_string),
                        class_name,
                        None
                    )
            except (ModuleNotFoundError, ImportError):
                pass

        if class_reference and inspect.isclass(class_reference):
            return class_reference()
    except FileNotFoundError as fnf_error:
        logger.error(f"Exception while attempting to import file { file_name }: { fnf_error }")

    return None


def _get_filter_instances(file_path, filter_roots: list[str], logger):
    filter_instances = []

    for root in filter_roots:
        filter_obj = _get_class(file_path, root, logger)

        if isinstance(filter_obj, FilterInterface):
            filter_instances.append((filter_obj, root))

    return filter_instances


def get_filters(filter_dictionaries: list[dict], logger, filter_roots: Optional[list[str]] = None):
    plugin_filters: list[FilterInterface] = []
    filter_roots = filter_roots if filter_roots is not None else _get_root_list()

    if filter_dictionaries is None:
        return plugin_filters

    for plugin_filter in filter_dictionaries:
        if (
            isinstance(plugin_filter, dict) and
            (file_path := plugin_filter.get('filter_file_path', None))
        ):
            args = plugin_filter.get('args', [])
            kwargs = plugin_filter.get('kwargs', {})

            logger.info(f"Initialising filter: { file_path }")

            root_found = False
            filters = _get_filter_instances(file_path, filter_roots, logger)

            for filter_instance, filter_root in filters:
                try:
                    filter_instance.setup(*args, **kwargs)

                    plugin_filters.append(filter_instance)

                    logger.info(f"Found filter instance in root { filter_root }")
                    root_found = True

                    break
                except TypeError as error:
                    logger.warning(
                        f"Filter { filter_instance } located in root { filter_root } "
                        f"crashed with Type Error { error }. "
                        "If the name of the given filter and its arguments "
                        "correspond to the intended ones, the issue might be caused by "
                        f"the root configuration ({ filter_roots }). "
                        "The script will keep searching for the filter in lower roots. "
                        "In order to avoid this warning, please check if any of the roots "
                        f"contains a filter with the same path as { file_path }. If so, "
                        "consider renaming your filter or giving its root a higher priority "
                        "(the highest priority is given to the first element of the list "
                        "linked to the plugin filter roots env variable, "
                        "while the last one has the lowest. "
                        "To increase the priority of your root, "
                        "move it towards the start of the list.). "
                    )

            if not root_found:
                logger.warning(
                    f"Filter { file_path } could not be found "
                    f"in any of the examined roots ({ filter_roots })"
                )

    # Reverse the filter list,
    # so that the last filter to be executed will be the one that was input first
    plugin_filters.reverse()

    return plugin_filters


def is_valid_suite(suite):
    return suite.status not in [
        TwisterStatus.FILTER,
        TwisterStatus.SKIP
    ] and not suite.skip


def handle_suite(suite, plugin_filters: list[FilterInterface]):
    if is_valid_suite(suite):
        for filter_obj in plugin_filters:
            if filter_obj.exclude(suite):
                suite.skip = True
            if filter_obj.include(suite):
                suite.skip = False
