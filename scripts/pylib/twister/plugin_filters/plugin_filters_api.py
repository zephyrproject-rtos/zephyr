from argparse import _ArgumentGroup

from plugin_filters.logic import filter_handler
from plugin_filters.filter_interface import FilterInterface
from plugin_filters.args import args_descriptor, args_parser


def plugin_filter_add_arguments(case_select: _ArgumentGroup):
    return args_descriptor.add_arguments(case_select)


def plugin_filter_parse_arguments(options, logger):
    return args_parser.parse_arguments(options, logger)


def plugin_filter_get_filters(filter_dictionaries: list[dict], logger, filter_roots: list[str] = None):
    return filter_handler.get_filters(filter_dictionaries, logger, filter_roots)


def plugin_filter_handle_suite(suite, plugin_filters: list[FilterInterface]):
    return filter_handler.handle_suite(suite, plugin_filters)
