#!/usr/bin/env python3
#
# Copyright (c) 2025 Baumer Group. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

from argparse import _ArgumentGroup

from plugin_filters.args import args_descriptor, args_parser
from plugin_filters.filter_interface import FilterInterface
from plugin_filters.logic import filter_handler


def plugin_filter_add_arguments(case_select: _ArgumentGroup):
    return args_descriptor.add_arguments(case_select)


def plugin_filter_parse_arguments(options):
    return args_parser.parse_arguments(options)


def plugin_filter_get_filters(
    filter_dictionaries: list[dict], filter_roots: list[str] | None = None
):
    return filter_handler.get_filters(filter_dictionaries, filter_roots)


def plugin_filter_handle_suite(suite, plugin_filters: list[FilterInterface]):
    return filter_handler.handle_suite(suite, plugin_filters)


def plugin_filter_teardown_filters(plugin_filters: list[FilterInterface]):
    for plugin_filter in plugin_filters:
        plugin_filter.teardown()
