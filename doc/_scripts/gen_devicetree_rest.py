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

from devicetree import edtlib

import gen_helpers

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
        vnd2vendor.update(edtlib.load_vendor_prefixes_txt(vendor_prefixes))

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
    dump_content(bindings, base_binding, vnd_lookup, args.out_dir,
                 args.turbo_mode)

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
    parser.add_argument('--turbo-mode', action='store_true',
                        help='Enable turbo mode (dummy references)')
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
        binding_files.extend(glob.glob(f'{dts_root}/dts/bindings/**/*.yml',
                                       recursive=True))
        binding_files.extend(glob.glob(f'{dts_root}/dts/bindings/**/*.yaml',
                                       recursive=True))

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
    base_includes = {"pm.yaml": os.fspath(ZEPHYR_BASE / 'dts' / 'bindings' / 'base'/ 'pm.yaml')}

    if not base_yaml.is_file():
        sys.exit(f'Expected to find base.yaml at {base_yaml}')
    return edtlib.Binding(os.fspath(base_yaml), base_includes, require_compatible=False,
                          require_description=False)

def dump_content(bindings, base_binding, vnd_lookup, out_dir, turbo_mode):
    # Dump the generated .rst files for a vnd2bindings dict.
    # Files are only written if they are changed. Existing .rst
    # files which would not be written by the 'vnd2bindings'
    # dict are removed.

    out_dir = Path(out_dir)

    setup_bindings_dir(bindings, out_dir)
    if turbo_mode:
        write_dummy_index(bindings, out_dir)
    else:
        write_bindings_rst(vnd_lookup, out_dir)
        write_orphans(bindings, base_binding, vnd_lookup, out_dir)

def setup_bindings_dir(bindings, out_dir):
    # Make a set of all the Path objects we will be creating for
    # out_dir / bindings / {binding_path}.rst. Delete all the ones that
    # shouldn't be there. Make sure the bindings output directory
    # exists.

    paths = set()
    bindings_dir = out_dir / 'bindings'
    logger.info('making output subdirectory %s', bindings_dir)
    bindings_dir.mkdir(parents=True, exist_ok=True)

    for binding in bindings:
        paths.add(bindings_dir / binding_filename(binding))

    for dirpath, _, filenames in os.walk(bindings_dir):
        for filename in filenames:
            path = Path(dirpath) / filename
            if path not in paths:
                logger.info('removing unexpected file %s', path)
                path.unlink()


def write_dummy_index(bindings, out_dir):
    # Write out_dir / bindings.rst, with dummy anchors

    # header
    content = '\n'.join((
        '.. _devicetree_binding_index:',
        '.. _dt_vendor_zephyr:',
        '',
        'Dummy bindings index',
        '####################',
        '',
    ))

    # build compatibles set and dump it
    compatibles = {binding.compatible for binding in bindings}
    content += '\n'.join((
        f'.. dtcompatible:: {compatible}' for compatible in compatibles
    ))

    write_if_updated(out_dir / 'bindings.rst', content)


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

def write_orphans(bindings, base_binding, vnd_lookup, out_dir):
    # Write out_dir / bindings / foo / binding_page.rst for each binding
    # in 'bindings', along with any "disambiguation" pages needed when a
    # single compatible string can be handled by multiple bindings.
    #
    # These files are 'orphans' in the Sphinx sense: they are not in
    # any toctree.

    logging.info('updating :orphan: files for %d bindings', len(bindings))
    num_written = 0

    # First, figure out which compatibles map to multiple bindings. We
    # need this information to decide which of the generated files for
    # a compatible are "disambiguation" pages that point to per-bus
    # binding pages, and which ones aren't.

    compat2bindings = defaultdict(list)
    for binding in bindings:
        compat2bindings[binding.compatible].append(binding)
    dup_compat2bindings = {compatible: bindings for compatible, bindings
                           in compat2bindings.items() if len(bindings) > 1}

    # Next, write the per-binding pages. These contain the
    # per-compatible targets for compatibles not in 'dup_compats'.
    # We'll finish up by writing per-compatible "disambiguation" pages
    # for compatibles in 'dup_compats'.

    # Names of properties in base.yaml.
    base_names = set(base_binding.prop2specs.keys())
    for binding in bindings:
        string_io = io.StringIO()

        print_binding_page(binding, base_names, vnd_lookup,
                           dup_compat2bindings, string_io)

        written = write_if_updated(out_dir / 'bindings' /
                                   binding_filename(binding),
                                   string_io.getvalue())

        if written:
            num_written += 1

    # Generate disambiguation pages for dup_compats.
    compatibles_dir = out_dir / 'compatibles'
    setup_compatibles_dir(dup_compat2bindings.keys(), compatibles_dir)
    for compatible in dup_compat2bindings:
        string_io = io.StringIO()

        print_compatible_disambiguation_page(
            compatible, dup_compat2bindings[compatible], string_io)

        written = write_if_updated(compatibles_dir /
                                   compatible_filename(compatible),
                                   string_io.getvalue())

        if written:
            num_written += 1

    logging.info('done writing :orphan: files; %d files needed updates',
                 num_written)

def print_binding_page(binding, base_names, vnd_lookup, dup_compats,
                       string_io):
    # Print the rst content for 'binding' to 'string_io'. The
    # 'dup_compats' argument should support membership testing for
    # compatibles which have multiple associated bindings; if
    # 'binding.compatible' is not in it, then the ref target for the
    # entire compatible is generated in this page as well.

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
    if compatible not in dup_compats:
        # If this binding is the only one that handles this
        # compatible, point the ".. dtcompatible:" directive straight
        # to this page. There's no need for disambiguation.
        dtcompatible = f'.. dtcompatible:: {binding.compatible}'
    else:
        # This compatible is handled by multiple bindings;
        # its ".. dtcompatible::" should be in a disambiguation page
        # instead.
        dtcompatible = ''

    print_block(f'''\
    :orphan:

    .. raw:: html

        <!--
        FIXME: do not limit page width until content uses another representation
        format other than tables
        -->
        <style>.wy-nav-content {{ max-width: none; !important }}</style>

    {dtcompatible}
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

def print_top_level_properties(binding, base_names, string_io):
    # Print the RST for top level properties for 'binding' to 'string_io'.
    #
    # The 'base_names' set contains all the base.yaml properties.

    def prop_table(filter_fn, deprecated):
        # Get a properly formatted and indented table of properties.
        specs = [prop_spec for prop_spec in binding.prop2specs.values()
                 if filter_fn(prop_spec)]
        indent = ' ' * 14
        if specs:
            temp_io = io.StringIO()
            print_property_table(specs, temp_io, deprecated=deprecated)
            return textwrap.indent(temp_io.getvalue(), indent)

        return indent + '(None)'

    def node_props_filter(prop_spec):
        return prop_spec.name not in base_names and not prop_spec.deprecated

    def deprecated_node_props_filter(prop_spec):
        return prop_spec.name not in base_names and prop_spec.deprecated

    def base_props_filter(prop_spec):
        return prop_spec.name in base_names

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

{prop_table(node_props_filter, False)}

           .. group-tab:: Deprecated node specific properties

              Deprecated properties not inherited from the base binding file.

{prop_table(deprecated_node_props_filter, False)}

           .. group-tab:: Base properties

              Properties inherited from the base binding file, which defines
              common properties that may be set on many nodes. Not all of these
              may apply to the "{binding.compatible}" compatible.

{prop_table(base_props_filter, True)}

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
            print_property_table(child.prop2specs.values(), string_io,
                                 deprecated=True)
        child = child.child_binding
        level += 1

def print_property_table(prop_specs, string_io, deprecated=False):
    # Writes a table of properties based on 'prop_specs', an iterable
    # of edtlib.PropertySpec objects, to 'string_io'.
    #
    # If 'deprecated' is true and the property is deprecated, an extra
    # line is printed mentioning that fact. We allow this to be turned
    # off for tables where all properties are deprecated, so it's
    # clear from context.

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

        if deprecated and prop_spec.deprecated:
            details += '\n\nThis property is **deprecated**.'

        return f"""\
   * - ``{prop_spec.name}``
     - ``{prop_spec.type}``
     - {textwrap.indent(details, ' ' * 7).lstrip()}
"""

    # Print each row.
    for prop_spec in prop_specs:
        print(to_prop_table_row(prop_spec), file=string_io)

def setup_compatibles_dir(compatibles, compatibles_dir):
    # Make a set of all the Path objects we will be creating for
    # out_dir / compatibles / {compatible_path}.rst. Delete all the ones that
    # shouldn't be there. Make sure the compatibles output directory
    # exists.

    logger.info('making output subdirectory %s', compatibles_dir)
    compatibles_dir.mkdir(parents=True, exist_ok=True)

    paths = set(compatibles_dir / compatible_filename(compatible)
                for compatible in compatibles)

    for path in compatibles_dir.iterdir():
        if path not in paths:
            logger.info('removing unexpected file %s', path)
            path.unlink()


def print_compatible_disambiguation_page(compatible, bindings, string_io):
    # Print the disambiguation page for 'compatible', which can be
    # handled by any of the bindings in 'bindings', to 'string_io'.

    assert len(bindings) > 1, (compatible, bindings)

    underline = '#' * len(compatible)
    output_list = '\n    '.join(f'- :ref:`{binding_ref_target(binding)}`'
                                for binding in bindings)

    print_block(f'''\
    :orphan:

    .. dtcompatible:: {compatible}

    {compatible}
    {underline}

    The devicetree compatible ``{compatible}`` may be handled by any
    of the following bindings:

    {output_list}
    ''', string_io)

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

def compatible_filename(compatible):
    # Name of the per-compatible disambiguation page within the
    # out_dir / compatibles directory.

    return f'{compatible}.rst'

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
    # Returns the output file name for a binding relative to the
    # directory containing documentation for all bindings. It does
    # this by stripping off the '.../dts/bindings/' prefix common to
    # all bindings files in a DTS_ROOT directory.
    #
    # For example, for .../zephyr/dts/bindings/base/base.yaml, this
    # would return 'base/base.yaml'.
    #
    # Hopefully that's unique across roots. If not, we'll need to
    # update this function.

    as_posix = Path(binding.path).as_posix()
    dts_bindings = 'dts/bindings/'
    idx = as_posix.rfind(dts_bindings)

    if idx == -1:
        raise ValueError(f'binding path has no {dts_bindings}: {binding.path}')

    # Cut past dts/bindings, strip off the extension (.yaml or .yml), and
    # replace with .rst.
    return os.path.splitext(as_posix[idx + len(dts_bindings):])[0] + '.rst'

def binding_ref_target(binding):
    # Return the sphinx ':ref:' target name for a binding.

    stem = Path(binding.path).stem
    return 'dtbinding_' + re.sub('[/,-]', '_', stem)

def write_if_updated(path, s):
    # gen_helpers.write_if_updated() wrapper that handles logging and
    # creating missing parents, as needed.

    if not path.parent.is_dir():
        path.parent.mkdir(parents=True)
    written = gen_helpers.write_if_updated(path, s)
    logger.debug('%s %s', 'wrote' if written else 'did NOT write', path)
    return written


if __name__ == '__main__':
    main()
    sys.exit(0)
