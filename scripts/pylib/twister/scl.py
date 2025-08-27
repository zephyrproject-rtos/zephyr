#! /usr/bin/python
#
# SPDX-License-Identifier: Apache-2.0
# Zephyr's Twister library
#
# pylint: disable=unused-import
#
# Set of code that other projects can also import to do things on
# Zephyr's sanity check testcases.

import jsonschema
import logging
import yaml

from jsonschema.exceptions import best_match

try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    # It's much faster.
    from yaml import CLoader as Loader
    from yaml import CSafeLoader as SafeLoader
    from yaml import CDumper as Dumper
except ImportError:
    from yaml import Loader, SafeLoader, Dumper

log = logging.getLogger("scl")
# Cache for validators to avoid recreating them for the same JSON schema
_validator_cache = {}

class EmptyYamlFileException(Exception):
    pass


#
#
def yaml_load(filename):
    """
    Safely load a YAML document

    Follows recommendations from
    https://security.openstack.org/guidelines/dg_avoid-dangerous-input-parsing-libraries.html.

    :param str filename: filename to load
    :raises yaml.scanner: On YAML scan issues
    :raises: any other exception on file access errors
    :return: dictionary representing the YAML document
    """
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            return yaml.load(f, Loader=SafeLoader)
    except yaml.scanner.ScannerError as e:	# For errors parsing schema.yaml
        mark = e.problem_mark
        cmark = e.context_mark
        log.error("%s:%d:%d: error: %s (note %s context @%s:%d:%d %s)",
                  mark.name, mark.line, mark.column, e.problem,
                  e.note, cmark.name, cmark.line, cmark.column, e.context)
        raise

def _yaml_validate(data, schema):
    if not schema:
        return

    schema_key = id(schema)
    if schema_key not in _validator_cache:
        validator_class = jsonschema.validators.validator_for(schema)
        validator_class.check_schema(schema)
        _validator_cache[schema_key] = validator_class(schema)

    validator = _validator_cache[schema_key]
    errors = list(validator.iter_errors(data))
    if errors:
        err = best_match(errors)
        raise jsonschema.ValidationError(f"Error: {err.message} in {err.json_path}")

def yaml_load_verify(filename, schema):
    """
    Safely load a testcase/sample yaml document and validate it
    against the YAML schema, returning in case of success the YAML data.

    :param str filename: name of the file to load and process
    :param dict schema: loaded YAML schema (can load with :func:`yaml_load`)

    # 'document.yaml' contains a single YAML document.
    :raises yaml.scanner.ScannerError: on YAML parsing error
    :raises jsonschema.exceptions.ValidationError: on Schema violation error
    """
    # 'document.yaml' contains a single YAML document.
    y = yaml_load(filename)
    if not y:
        raise EmptyYamlFileException('No data in YAML file: %s' % filename)
    _yaml_validate(y, schema)
    return y
