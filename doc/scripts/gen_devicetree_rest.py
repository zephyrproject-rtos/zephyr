# Copyright (c) 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

"""
Like gen_kconfig_rest.py, but for generating an index of existing
devicetree bindings.
"""

import argparse
from collections import defaultdict
import glob
import io
import logging
import os
from pathlib import Path
import pprint
import re
import sys
import textwrap

import edtlib

import gen_helpers

# A line of '-' characters in vendor-prefixes.txt that separates
# the data from comments about it.
VENDOR_PREFIXES_SEPARATOR = '-' * 50

ZEPHYR_BASE = Path(__file__).parents[2]

GENERIC_OR_VENDOR_INDEPENDENT = 'Generic or vendor-independent'
UNKNOWN_VENDOR = 'Unknown vendor'

ZEPHYR_BASE = Path(__file__).parents[2]

# Base properties that have documentation in 'dt-important-props'.
DETAILS_IN_IMPORTANT_PROPS = set('compatible label reg status interrupts'.split())

logger = logging.getLogger('gen_devicetree_rest')

class VndLookup:
    """
    A convenience class for looking up information based on a
    devicetree compatible's vendor prefix 'vnd'.
    """

    def __init__(self, vendor_prefixes, bindings):
        self.vnd2vendor = self.load_vnd2vendor(vendor_prefixes)
        self.vnd2bindings = self.init_vnd2bindings(bindings)
        self.vnd2ref_target = self.init_vnd2ref_target()

    def vendor(self, vnd):
        return self.vnd2vendor.get(vnd, UNKNOWN_VENDOR)

    def bindings(self, vnd, default=None):
        return self.vnd2bindings.get(vnd, default)

    def target(self, vnd):
        return self.vnd2ref_target.get(
            vnd, self.vnd2ref_target[(UNKNOWN_VENDOR,)])

    @staticmethod
    def load_vnd2vendor(vendor_prefixes):
        # Load the vendor-prefixes.txt file. Return a dict mapping 'vnd'
        # vendor prefixes as they are found in compatible properties to
        # each vendor's full name.
        #
        # For example, this line:
        #
        #    vnd	A stand-in for a real vendor
        #
        # Gets split into a key 'vnd' and a value 'A stand-in for a real
        # vendor' in the return value.
        #
        # The 'None' key maps to GENERIC_OR_VENDOR_INDEPENDENT.

        vnd2vendor = {
            None: GENERIC_OR_VENDOR_INDEPENDENT,
        }
        found_separator = False     # have we found the '-----' separator?
        with open(vendor_prefixes, 'r') as f:
            for line in f:
                line = line.strip()
                if line and found_separator:
                    # Every line after the separator should be in this form:
                    #
                    # <vnd><TAB><vendor>
                    vnd_vendor = line.split('\t', 1)
                    assert len(vnd_vendor) == 2, line
                    vnd2vendor[vnd_vendor[0]] = vnd_vendor[1]
                elif line.startswith(VENDOR_PREFIXES_SEPARATOR):
                    found_separator = True

        logger.info('found %d vendor prefixes in %s', len(vnd2vendor) - 1,
                    vendor_prefixes)
        if logger.isEnabledFor(logging.DEBUG):
            logger.debug('vnd2vendor=%s', pprint.pformat(vnd2vendor))

        return vnd2vendor

    def init_vnd2bindings(self, bindings):
        # Take a 'vnd2vendor' map and a list of bindings and return a dict
        # mapping 'vnd' vendor prefixes prefixes to lists of bindings. The
        # bindings in each list are sorted by compatible. The keys in the
        # return value are sorted by vendor name.
        #
        # Special cases:
        #
        # - The 'None' key maps to bindings with no vendor prefix
        #   in their compatibles, like 'gpio-keys'. This is the first key.
        # - The (UNKNOWN_VENDOR,) key maps to bindings whose compatible
        #   has a vendor prefix that exists, but is not known,
        #   like 'somethingrandom,device'. This is the last key.

        # Get an unsorted dict mapping vendor prefixes to lists of bindings.
        unsorted = defaultdict(list)
        generic_bindings = []
        unknown_vendor_bindings = []
        for binding in bindings:
            vnd = compatible_vnd(binding.compatible)
            if vnd is None:
                generic_bindings.append(binding)
            elif vnd in self.vnd2vendor:
                unsorted[vnd].append(binding)
            else:
                unknown_vendor_bindings.append(binding)

        # Key functions for sorting.
        def vnd_key(vnd):
            return self.vnd2vendor[vnd].casefold()

        def binding_key(binding):
            return binding.compatible

        # Sort the bindings for each vendor by compatible.
        # Plain dicts are sorted in CPython 3.6+, which is what we
        # support, so the return dict's keys are in the same
        # order as vnd2vendor.
        #
        # The unknown-vendor bindings being inserted as a 1-tuple key is a
        # hack for convenience that ensures they won't collide with a
        # known vendor. The code that consumes the dict below handles
        # this.
        vnd2bindings = {
            None: sorted(generic_bindings, key=binding_key)
        }
        for vnd in sorted(unsorted, key=vnd_key):
            vnd2bindings[vnd] = sorted(unsorted[vnd], key=binding_key)
        vnd2bindings[(UNKNOWN_VENDOR,)] = sorted(unknown_vendor_bindings,
                                                      key=binding_key)

        if logger.isEnabledFor(logging.DEBUG):
            logger.debug('vnd2bindings: %s', pprint.pformat(vnd2bindings))

        return vnd2bindings

    def init_vnd2ref_target(self):
        # The return value, vnd2ref_target, is a dict mapping vendor
        # prefixes to ref targets for their relevant sections in this
        # file, with these special cases:
        #
        # - The None key maps to the ref target for bindings with no
        #   vendor prefix in their compatibles, like 'gpio-keys'
        # - The (UNKNOWN_VENDOR,) key maps to the ref target for bindings
        #   whose compatible has a vendor prefix that is not recognized.
        vnd2ref_target = {}

        for vnd in self.vnd2bindings:
            if vnd is None:
                vnd2ref_target[vnd] = 'dt_no_vendor'
            elif isinstance(vnd, str):
                vnd2ref_target[vnd] = f'dt_vendor_{vnd}'
            else:
                assert vnd == (UNKNOWN_VENDOR,), vnd
                vnd2ref_target[vnd] = 'dt_unknown_vendor'

        return vnd2ref_target

def main():
    args = parse_args()
    setup_logging(args.verbose)
    bindings = load_bindings(args.dts_roots)
    base_binding = load_base_binding()
    vnd_lookup = VndLookup(args.vendor_prefixes, bindings)
    dump_content(bindings, base_binding, vnd_lookup, args.out_dir)

def parse_args():
    # Parse command line arguments from sys.argv.

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', default=0, action='count',
                        help='increase verbosity; may be given multiple times')
    parser.add_argument('--vendor-prefixes', required=True,
                        help='vendor-prefixes.txt file path')
    parser.add_argument('--dts-root', dest='dts_roots', action='append',
                        help='''additional DTS root directory as it would
                        be set in DTS_ROOTS''')
    parser.add_argument('out_dir', help='output files are generated here')

    return parser.parse_args()

def setup_logging(verbose):
    if verbose >= 2:
        log_level = logging.DEBUG
    elif verbose == 1:
        log_level = logging.INFO
    else:
        log_level = logging.ERROR
    logging.basicConfig(format='%(filename)s:%(levelname)s: %(message)s',
                        level=log_level)

def load_bindings(dts_roots):
    # Get a list of edtlib.Binding objects from searching 'dts_roots'.

    if not dts_roots:
        sys.exit('no DTS roots; use --dts-root to specify at least one')

    binding_files = []
    for dts_root in dts_roots:
        binding_files.extend(glob.glob(f'{dts_root}/dts/bindings/**/*.yaml'))

    bindings = edtlib.bindings_from_paths(binding_files, ignore_errors=True)

    num_total = len(bindings)

    # Remove bindings from the 'vnd' vendor, which is not a real vendor,
    # but rather a stand-in we use for examples and tests when a real
    # vendor would be inappropriate.
    bindings = [binding for binding in bindings if
                compatible_vnd(binding.compatible) != 'vnd']

    logger.info('found %d bindings (ignored %d) in this dts_roots list: %s',
                len(bindings), num_total - len(bindings), dts_roots)

    return bindings

def load_base_binding():
    # Make a Binding object for base.yaml.
    #
    # This helps separate presentation for properties common to all
    # nodes from node-specific properties.

    base_yaml = ZEPHYR_BASE / 'dts' / 'bindings' / 'base' / 'base.yaml'
    if not base_yaml.is_file():
        sys.exit(f'Expected to find base.yaml at {base_yaml}')
    return edtlib.Binding(os.fspath(base_yaml), {}, require_compatible=False,
                          require_description=False)

def dump_content(bindings, base_binding, vnd_lookup, out_dir):
    # Dump the generated .rst files for a vnd2bindings dict.
    # Files are only written if they are changed. Existing .rst
    # files which would not be written by the 'vnd2bindings'
    # dict are removed.

    out_dir = Path(out_dir)

    setup_bindings_dir(bindings, out_dir)
    write_bindings_rst(vnd_lookup, out_dir)
    write_per_compatible_orphans(bindings, base_binding, vnd_lookup, out_dir)

def setup_bindings_dir(bindings, out_dir):
    # Make a set of all the Path objects we will be creating for
    # out_dir / bindings / {compatible}.rst. Delete all the ones that
    # shouldn't be there. Make sure the bindings output directory
    # exists.

    paths = set()
    bindings_dir = out_dir / 'bindings'
    logger.info('making output subdirectory %s', bindings_dir)
    bindings_dir.mkdir(parents=True, exist_ok=True)

    for binding in bindings:
        paths.add(bindings_dir / binding_filename(binding))

    for path in bindings_dir.iterdir():
        if path not in paths:
            logger.info('removing unexpected file %s', path)
            path.unlink()

def write_bindings_rst(vnd_lookup, out_dir):
    # Write out_dir / bindings.rst, the top level index of bindings.

    string_io = io.StringIO()

    print_block(f'''\
    .. _devicetree_binding_index:

    Bindings index
    ##############

    This page documents the available devicetree bindings.
    See {zref('dt-bindings')} for an introduction to the Zephyr bindings
    file format.

    Vendor index
    ************

    This section contains an index of hardware vendors.
    Click on a vendor's name to go to the list of bindings for
    that vendor.

    .. rst-class:: rst-columns
    ''', string_io)

    for vnd in vnd_lookup.vnd2bindings:
        print(f'- :ref:`{vnd_lookup.target(vnd)}`', file=string_io)

    print_block('''\

    Bindings by vendor
    ******************

    This section contains available bindings, grouped by vendor.
    Within each group, bindings are listed by the "compatible" property
    they apply to, like this:

    **Vendor name (vendor prefix)**

    .. rst-class:: rst-columns

    - <compatible-A>
    - <compatible-B> (on <bus-name> bus)
    - <compatible-C>
    - ...

    The text "(on <bus-name> bus)" appears when bindings may behave
    differently depending on the bus the node appears on.
    For example, this applies to some sensor device nodes, which may
    appear as children of either I2C or SPI bus nodes.
    ''', string_io)

    for vnd, bindings in vnd_lookup.vnd2bindings.items():
        if isinstance(vnd, tuple):
            title = vnd[0]
        else:
            title = vnd_lookup.vendor(vnd).strip()
            if isinstance(vnd, str):
                title += f' ({vnd})'
        underline = '=' * len(title)

        print_block(f'''\
        .. _{vnd_lookup.target(vnd)}:

        {title}
        {underline}

        .. rst-class:: rst-columns
        ''', string_io)
        for binding in bindings:
            print(f'- :ref:`{binding_ref_target(binding)}`', file=string_io)
        print(file=string_io)

    write_if_updated(out_dir / 'bindings.rst', string_io.getvalue())

def write_per_compatible_orphans(bindings, base_binding, vnd_lookup, out_dir):
    # For each compatible in vnd2bindings, write the corresponding
    # out_dir / bindings / {binding.compatible}.rst file, if its
    # content needs updating. This file is an 'orphan' because it's not
    # in any toctree.

    logging.info('updating up to %d :orphan: files', len(bindings))
    num_written = 0
    # Names of properties in base.yaml.
    base_names = set(base_binding.prop2specs.keys())
    for binding in bindings:
        string_io = io.StringIO()

        # :orphan:
        #
        # .. ref_target:
        #
        # Title [(on <bus> bus)]
        # ######################
        if binding.on_bus:
            on_bus_title = f' (on {binding.on_bus} bus)'
        else:
            on_bus_title = ''
        compatible = binding.compatible
        title = f'{compatible}{on_bus_title}'
        underline = '#' * len(title)
        print_block(f'''\
        :orphan:

        .. _{binding_ref_target(binding)}:

        {title}
        {underline}
        ''', string_io)

        # Vendor: <link-to-vendor-section>
        vnd = compatible_vnd(compatible)
        print('Vendor: '
              f':ref:`{vnd_lookup.vendor(vnd)} <{vnd_lookup.target(vnd)}>`\n',
              file=string_io)

        # Binding description.
        if binding.bus:
            bus_help = f'These nodes are "{binding.bus}" bus nodes.'
        else:
            bus_help = ''
        print_block(f'''\
        Description
        ***********

        {bus_help}
        ''', string_io)
        print(to_code_block(binding.description.strip()), file=string_io)

        # Properties.
        print_block('''\
        Properties
        **********
        ''', string_io)
        print_top_level_properties(binding, base_names, string_io)
        print_child_binding_properties(binding, string_io)

        # Specifier cells.
        #
        # This presentation isn't particularly nice. Perhaps something
        # better can be done for future work.
        if binding.specifier2cells:
            print_block('''\
            Specifier cell names
            ********************
            ''', string_io)
            for specifier, cells in binding.specifier2cells.items():
                print(f'- {specifier} cells: {", ".join(cells)}',
                      file=string_io)

        written = write_if_updated(out_dir / 'bindings' /
                                   binding_filename(binding),
                                   string_io.getvalue())

        if written:
            num_written += 1

    logging.info('done writing :orphan: files; %d files needed updates',
                 num_written)

def print_top_level_properties(binding, base_names, string_io):
    # Print the RST for top level properties for 'binding' to 'string_io'.
    #
    # The 'base_names' set contains all the base.yaml properties.

    def prop_table(filter_fn):
        # Get a properly formatted and indented table of properties.
        specs = [prop_spec for prop_spec in binding.prop2specs.values()
                 if filter_fn(prop_spec)]
        indent = ' ' * 14
        if specs:
            temp_io = io.StringIO()
            print_property_table(specs, temp_io)
            return textwrap.indent(temp_io.getvalue(), indent)

        return indent + '(None)'

    if binding.child_binding:
        print_block('''\
        Top level properties
        ====================
        ''', string_io)
    if binding.prop2specs:
        if binding.child_binding:
            print_block(f'''
            These property descriptions apply to "{binding.compatible}"
            nodes themselves. This page also describes child node
            properties in the following sections.
            ''', string_io)

        print_block(f'''\
        .. tabs::

           .. group-tab:: Node specific properties

              Properties not inherited from the base binding file.

{prop_table(lambda prop_spec: prop_spec.name not in base_names)}

           .. group-tab:: Base properties

              Properties inherited from the base binding file, which defines
              common properties that may be set on many nodes. Not all of these
              may apply to the "{binding.compatible}" compatible.

{prop_table(lambda prop_spec: prop_spec.name in base_names)}

        ''', string_io)
    else:
        print('No top-level properties.\n', file=string_io)

def print_child_binding_properties(binding, string_io):
    # Prints property tables for all levels of nesting of child
    # bindings.

    level = 1
    child = binding.child_binding
    while child is not None:
        if level == 1:
            level_string = 'Child'
        elif level == 2:
            level_string = 'Grandchild'
        else:
            level_string = f'Level {level} child'
        if child.prop2specs:
            title = f'{level_string} node properties'
            underline = '=' * len(title)
            print(f'{title}\n{underline}\n', file=string_io)
            print_property_table(child.prop2specs.values(), string_io)
        child = child.child_binding
        level += 1

def print_property_table(prop_specs, string_io):
    # Writes a table of properties based on 'prop_specs', an iterable
    # of edtlib.PropertySpec objects, to 'string_io'.

    # Table header.
    print_block('''\
    .. list-table::
       :widths: 1 1 4
       :header-rows: 1

       * - Name
         - Type
         - Details
    ''', string_io)

    def to_prop_table_row(prop_spec):
        # Get a multiline string for a PropertySpec table row.

        # The description column combines the description field,
        # along with things like the default value or enum values.
        #
        # The property 'description' field from the binding may span
        # one or multiple lines. We try to come up with a nice
        # presentation for each.
        details = ''
        raw_prop_descr = prop_spec.description
        if raw_prop_descr:
            details += to_code_block(raw_prop_descr)

        if prop_spec.required:
            details += '\n\nThis property is **required**.'

        if prop_spec.default:
            details += f'\n\nDefault value: ``{prop_spec.default}``'

        if prop_spec.const:
            details += f'\n\nConstant value: ``{prop_spec.const}``'
        elif prop_spec.enum:
            details += ('\n\nLegal values: ' +
                        ', '.join(f'``{repr(val)}``' for val in
                                  prop_spec.enum))

        if prop_spec.name in DETAILS_IN_IMPORTANT_PROPS:
            details += (f'\n\nSee {zref("dt-important-props")} for more '
                        'information.')

        return f"""\
   * - ``{prop_spec.name}``
     - ``{prop_spec.type}``
     - {textwrap.indent(details, ' ' * 7).lstrip()}
"""

    # Print each row.
    for prop_spec in prop_specs:
        print(to_prop_table_row(prop_spec), file=string_io)

def print_block(block, string_io):
    # Helper for dedenting and printing a triple-quoted RST block.
    # (Just a block of text, not necessarily just a 'code-block'
    # directive.)

    print(textwrap.dedent(block), file=string_io)

def to_code_block(s, indent=0):
    # Converts 's', a string, to an indented rst .. code-block::. The
    # 'indent' argument is a leading indent for each line in the code
    # block, in spaces.
    indent = indent * ' '
    return ('.. code-block:: none\n\n' +
            textwrap.indent(s, indent + '   ') + '\n')

def compatible_vnd(compatible):
    # Get the vendor prefix for a compatible string 'compatible'.
    #
    # For example, compatible_vnd('foo,device') is 'foo'.
    #
    # If 'compatible' has no comma (','), None is returned.

    if ',' not in compatible:
        return None

    return compatible.split(',', 1)[0]

def binding_ref_target(binding):
    # Return the sphinx ':ref:' target name for a binding.

    ret = re.sub('[,-]', '_', binding.compatible)
    if binding.on_bus is not None:
        ret += f'_{binding.on_bus}'
    return ret

def zref(target, text=None):
    # Make an appropriate RST :ref:`text <target>` or :ref:`target`
    # string to a zephyr documentation ref target 'target', and return
    # it.
    #
    # By default, the bindings docs are in the main Zephyr
    # documentation, but this script supports putting them in a
    # separate Sphinx doc set. Since we also link to Zephyr
    # documentation from the generated content, we have an environment
    # variable based escape hatch for putting the target in the zephyr
    # doc set.
    #
    # This relies on intersphinx:
    # https://www.sphinx-doc.org/en/master/usage/extensions/intersphinx.html

    docset = os.environ.get('GEN_DEVICETREE_REST_ZEPHYR_DOCSET', '')

    if docset.strip():
        target = f'{docset}:{target}'

    if text:
        return f':ref:`{text} <{target}>`'

    return f':ref:`{target}`'

def binding_filename(binding):
    if binding.on_bus is None:
        return f'{binding.compatible}.rst'

    return f'{binding.compatible}-{binding.on_bus}.rst'

def write_if_updated(path, s):
    # gen_helpers.write_if_updated() wrapper that handles logging.

    written = gen_helpers.write_if_updated(path, s)
    logger.debug('%s %s', 'wrote' if written else 'did NOT write', path)
    return written


if __name__ == '__main__':
    main()
    sys.exit(0)
