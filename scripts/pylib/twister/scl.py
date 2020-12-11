#! /usr/bin/python
#
# SPDX-License-Identifier: Apache-2.0
# Zephyr's Twister library
#
# pylint: disable=unused-import
#
# Set of code that other projects can also import to do things on
# Zephyr's sanity check testcases.

import logging
import yaml
try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    # It's much faster.
    from yaml import CLoader as Loader
    from yaml import CSafeLoader as SafeLoader
    from yaml import CDumper as Dumper
except ImportError:
    from yaml import Loader, SafeLoader, Dumper

log = logging.getLogger("scl")

#
#
def yaml_load(filename):
    """
    Safely load a YAML document

    Follows recomendations from
    https://security.openstack.org/guidelines/dg_avoid-dangerous-input-parsing-libraries.html.

    :param str filename: filename to load
    :raises yaml.scanner: On YAML scan issues
    :raises: any other exception on file access erors
    :return: dictionary representing the YAML document
    """
    try:
        with open(filename, 'r') as f:
            return yaml.load(f, Loader=SafeLoader)
    except yaml.scanner.ScannerError as e:	# For errors parsing schema.yaml
        mark = e.problem_mark
        cmark = e.context_mark
        log.error("%s:%d:%d: error: %s (note %s context @%s:%d:%d %s)",
                  mark.name, mark.line, mark.column, e.problem,
                  e.note, cmark.name, cmark.line, cmark.column, e.context)
        raise

# If pykwalify is installed, then the validate functionw ill work --
# otherwise, it is a stub and we'd warn about it.
try:
    import pykwalify.core
    # Don't print error messages yourself, let us do it
    logging.getLogger("pykwalify.core").setLevel(50)

    def _yaml_validate(data, schema):
        if not schema:
            return
        c = pykwalify.core.Core(source_data=data, schema_data=schema)
        c.validate(raise_exception=True)

except ImportError as e:
    log.warning("can't import pykwalify; won't validate YAML (%s)", e)
    def _yaml_validate(data, schema):
        pass

def yaml_load_verify(filename, schema):
    """
    Safely load a testcase/sample yaml document and validate it
    against the YAML schema, returing in case of success the YAML data.

    :param str filename: name of the file to load and process
    :param dict schema: loaded YAML schema (can load with :func:`yaml_load`)

    # 'document.yaml' contains a single YAML document.
    :raises yaml.scanner.ScannerError: on YAML parsing error
    :raises pykwalify.errors.SchemaError: on Schema violation error
    """
    # 'document.yaml' contains a single YAML document.
    y = yaml_load(filename)
    _yaml_validate(y, schema)
    return y
