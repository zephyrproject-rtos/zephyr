import os
import sys
import inspect
import importlib
import importlib.util

from plugin_filters.filter_interface import FilterInterface


__DEFAULT_ROOT = 'plugin_filters.filters'
__ROOTS_ENV_VARIABLE_NAME = "TWISTER_PLUGIN_FILTER_ROOTS"


def __get_root_list():
    root_list = os.getenv(__ROOTS_ENV_VARIABLE_NAME)

    if root_list is None:
        root_list = []
    elif not isinstance(root_list, list):
        sys.exit("$The TWISTER_PLUGIN_FILTER_ROOTS environment variable must be a list")

    if __DEFAULT_ROOT not in root_list:
        root_list.append(__DEFAULT_ROOT)

    return root_list


def __get_class(file_name, file_path, logger, class_name='Filter'):
    class_reference = None

    try:
        if (
            os.path.exists((class_path := os.path.join(file_path, file_name))) and
            (module_specification := importlib.util.spec_from_file_location(class_name, class_path)) and
            (module := importlib.util.module_from_spec(module_specification))
        ):
            module_specification.loader.exec_module(module)
            class_reference = getattr(module, class_name, None)

        if class_reference is None:
            import_string = f"{ file_path }.{ file_name }"

            try:
                if importlib.util.find_spec(import_string):
                    class_reference = getattr(importlib.import_module(import_string), class_name, None)
            except ModuleNotFoundError:
                pass

        if class_reference and inspect.isclass(class_reference):
            return class_reference()
    except FileNotFoundError as fnf_error:
        logger.error(f"Exception while attempting to import file { file_name }: { fnf_error }")

    return None


def get_filters(filter_dictionaries: list[dict], logger, filter_roots: list[str] = None):
    plugin_filters: list[FilterInterface] = []
    filter_roots = filter_roots if filter_roots is not None else __get_root_list()

    if filter_dictionaries is None:
        return plugin_filters

    for plugin_filter in filter_dictionaries:
        if isinstance(plugin_filter, dict):
            file_path = plugin_filter.get('filter_file_path', None)
            args = plugin_filter.get('args', [])
            kwargs = plugin_filter.get('kwargs', {})

            if file_path:

                logger.info(f"Initialising filter: { file_path }")

                root_found = False

                for root in filter_roots:
                    filter_obj = __get_class(file_path, root, logger)

                    if isinstance(filter_obj, FilterInterface):
                        filter_obj.setup(*args, **kwargs)

                        plugin_filters.append(filter_obj)

                        logger.info(f"Found filter instance in root { root }")
                        root_found = True

                        break

                if not root_found:
                    logger.warning(f"Filter { file_path } could not be found in any of the examined roots ({ filter_roots })")

    return plugin_filters


def handle_suite(suite, plugin_filters: list[FilterInterface]):
    suite.skip = True

    for filter_obj in plugin_filters:
        if not filter_obj.exclude(suite):
            suite.skip = False

        filter_obj.teardown()
