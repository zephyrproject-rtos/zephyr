import os
import sys


DEFAULT_ROOT = 'plugin_filters'
ROOTS_VARIABLE_NAME = "TWISTER_PLUGIN_FILTER_ROOTS"


def get_roots_list():
    root_list = os.getenv(ROOTS_VARIABLE_NAME)

    if root_list is None:
        root_list = []
    elif not isinstance(root_list, list):
        sys.exit("$The TWISTER_PLUGIN_FILTER_ROOTS environment variable must be a list")

    if DEFAULT_ROOT not in root_list:
        root_list.append(DEFAULT_ROOT)

    return root_list
