# Copyright (c) 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

"""
Like gen_kconfig_rest.py, but for generating an index of existing
devicetree drivers.
"""

import argparse
from collections import defaultdict
import glob
import io
import itertools
import logging
from pathlib import Path
import re
import shutil
import textwrap
from typing import Dict, List, NamedTuple
import xml.etree.ElementTree as ET

import gen_helpers

ZEPHYR_BASE = Path(__file__).parents[2]

logger = logging.getLogger('gen_driver_rest')

class DriverXmlEntry(NamedTuple):
    '''Represents an element of the zephyrdriver.xml file.

    For our purposes, this file contains a sequence of
    (name, filename, refid) tuples:

    - 'name' is the human-readable driver name
    - 'filename' is the basename of the driver source file
    - 'refid' is enough to find the path to the driver XML file
      in the doxygen XML directory
    '''

    name: str
    filename: str
    xml: Path

class CompatibleXmlEntry(NamedTuple):
    '''Represents an element of the dtcompatible.xml file.

    For our purposes, this file contains a sequence of
    (compatibles, filename, refid) tuples:

    - 'compatibles' is the list of devicetree compatibles as they
      appear in DTS
    - 'filename' is the basename of the driver source file which
      claims to create struct devices from nodes of this compatible
    - 'refid' is enough to find the path to the driver XML file
      in the doxygen XML directory
    '''

    compatibles: str
    filename: str
    xml: Path

class Driver(NamedTuple):
    '''
    Driver metadata, created by cross-referencing the data in the
    various DriverXmlEntry and CompatibleXmlEntry tuples we create,
    along with additional information in the driver XML files.
    '''

    name: str                   # human-readable name
    compatibles: List[str]      # devicetree compatible(s) handled
    path: Path                  # absolute path to driver
    api: str                    # zephyr API name

# A map from API names (like 'i2c', 'spi') to the list of drivers
# within that API.
DriverMap = Dict[str, List[Driver]]

def main():
    args = parse_args()
    xml_dir = Path(args.doxygen_xml)

    setup_logging(args.verbose)

    drv_entries: List[DriverXmlEntry] = load_zephyrdriver_xml(xml_dir)
    cmpt_entries: List[CompatibleXmlEntry] = load_dtcompatible_xml(xml_dir)

    driver_dirs = [ZEPHYR_BASE]
    for module in args.modules:
        driver_dirs.append(Path(module))
    api2drivers: DriverMap = resolve_drivers(driver_dirs,
                                             drv_entries,
                                             cmpt_entries)

    dump_content(api2drivers, Path(args.out_dir))

def parse_args():
    # Parse command line arguments from sys.argv.

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', default=0, action='count',
                        help='increase verbosity; may be given multiple times')
    parser.add_argument('--doxygen-xml', required=True,
                        help='doxygen xml output directory')
    parser.add_argument('--module', dest='modules', action='append',
                        default=[],
                        help='''additional module to search in;
                        ZEPHYR_BASE is included by default''')
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

def load_zephyrdriver_xml(doxygen_xml: Path) -> List[DriverXmlEntry]:
    # Load the zephyrdriver.xml file from the Doxygen output. This
    # contains the index of driver files which used the
    # "@zephyrdriver{Driver name}" doxygen alias to declare that they
    # are a driver implementation.
    #
    # Return a view of the relevant files and their compatibles.
    # This drives generation of the driver RST files.

    zephyrdriver_xml = doxygen_xml / 'zephyrdriver.xml'
    if not zephyrdriver_xml.is_file():
        raise RuntimeError(f'expected to find file {zephyrdriver_xml}')

    zephyrdriver_xml_tree = ET.parse(zephyrdriver_xml)
    ret = []
    for entry, items in varlist_entry_items(zephyrdriver_xml_tree):
        assert len(items) == 1
        name = "".join(items[0].itertext()).strip()
        ref = entry[0][0]
        filename = ref.text
        xml = doxygen_xml / f"{ref.attrib['refid']}.xml"

        logger.debug('driver name="%s", filename="%s", xml="%s"' ,
                     name, filename, xml)

        ret.append(DriverXmlEntry(name, filename, xml))

    logger.info('found %d entries in %s', len(ret), zephyrdriver_xml)

    return ret

def load_dtcompatible_xml(doxygen_xml: Path) -> List[CompatibleXmlEntry]:

    dtcompatible_xml = doxygen_xml / 'dtcompatible.xml'
    if not dtcompatible_xml.is_file():
        raise RuntimeError(f'expected to find file {dtcompatible_xml}')

    dtcompatible_xml_tree = ET.parse(dtcompatible_xml)
    ret = []
    for entry, items in varlist_entry_items(dtcompatible_xml_tree):
        compatibles = ["".join(item.itertext()).strip() for item in items]
        ref = entry[0][0]
        filename = ref.text
        xml = doxygen_xml / f"{ref.attrib['refid']}.xml"

        logger.debug('compatibles="%s", filename="%s", xml="%s"',
                     compatibles, filename, xml)

        ret.append(CompatibleXmlEntry(compatibles, filename, xml))

    logger.info('found %d entries in %s', len(ret), dtcompatible_xml)

    return ret

def varlist_entry_items(tree):
    # The doxygen XML files for each type of \xrefitem contain a
    # 'variablelist' element which is a sequence of pairs. The
    # elements of each pair are a 'varlistentry' and a 'listitem':
    #
    # - the 'varlistentry' contains the reference data we need to find
    #   the file which used the \xrefitem
    #
    # - the 'listitem' contains the argument to the \xrefitem
    #
    # This function find the variablelist, checks that its length is
    # well-formed (i.e. contains a whole number of pairs), and returns
    # an iterator for each (varlistentry, listitem) pair.

    iterator = tree.getroot().iter('variablelist')
    vlist = list(next(iterator))
    assert len(vlist) % 2 == 0

    for i in range(len(vlist) // 2):
        entry, items = vlist[2 * i], vlist[2 * i + 1]
        yield (entry, items)

def resolve_drivers(
        driver_dirs: List[Path],
        drv_entries: List[DriverXmlEntry],
        cmpt_entries: List[CompatibleXmlEntry]
) -> DriverMap:
    # Figure out the Driver objects we really care about by
    # cross-referencing the data from zephyrdriver.xml and
    # dtcompatible.xml with the 'module/drivers' directory for each
    # 'module' in 'modules.
    #
    # Return a dictionary mapping APIs to lists of Driver

    def path_api(path):
        # get the API from the path. The path should end in
        # 'drivers/the_api/.../filename.c'. The return value is 'the_api'.
        maybe_api = None
        for part in reversed(path.parts):
            if part == 'drivers':
                return maybe_api
            maybe_api = part

    globs = (glob.glob(f'{driver_dir}/drivers/*/**/*.c', recursive=True)
             for driver_dir in driver_dirs)
    all_paths = [Path(abspath) for abspath in itertools.chain(*globs)]
    driver_filenames = set(drv_entry.filename for drv_entry in drv_entries)
    driver_paths = set(path for path in all_paths
                       if path.name in driver_filenames)

    filename2drv_entry = {entry.filename: entry for entry in drv_entries}
    filename2cmpt_entry = {entry.filename: entry for entry in cmpt_entries}
    filename2api = {path.name: path_api(path) for path in driver_paths}
    filename2path = {path.name: path for path in driver_paths}

    api2filenames = defaultdict(list)
    for filename, api in filename2api.items():
        api2filenames[api].append(filename)

    api2drivers: DriverMap = defaultdict(list)
    for api, filenames in api2filenames.items():
        for fn in filenames:
            api2drivers[api].append(
                Driver(name=filename2drv_entry[fn].name,
                       compatibles=filename2cmpt_entry[fn].compatibles,
                       path=filename2path[fn],
                       api=api))

    return api2drivers

def dump_content(api2drivers: DriverMap, out_dir: Path):
    setup_out_dir(api2drivers, out_dir)

    for api, drivers in api2drivers.items():
        api_dir = api_out_dir(api, out_dir)
        write_api_index(api, drivers, api_dir)
        write_per_driver_orphans(drivers, api_dir)

def setup_out_dir(api2drivers: DriverMap, out_dir: Path):
    out_dir.mkdir(exist_ok=True)

    api_dirs = set()

    for api in api2drivers:
        api_dir = api_out_dir(api, out_dir)
        api_dir.mkdir(exist_ok=True)
        api_dirs.add(api_dir)

        api_drivers = api2drivers[api]
        for child in api_dir.iterdir():
            if child.is_dir():
                logger.debug('removing unexpected subdirectory %s', child)
                shutil.rmtree(child)
            if not (child.name == 'index.rst' or
                    any(child.name == driver_rst_name(driver)
                        for driver in api_drivers)):
                logger.debug('removing unexpected file %s', child)
                child.unlink()

    for child in out_dir.iterdir():
        if child.is_dir() and child not in api_dirs:
            logger.debug('removing unexpected directory %s', child)
            shutil.rmtree(child)

    logger.debug('set up output directory %s', out_dir)

def write_api_index(api: str, drivers: List[Driver], api_dir: Path) -> None:
    # Write the f'{api_dir}/index.rst' file, which links to the
    # per-driver orphan pages for that API.

    string_io = io.StringIO()
    api_name = api_full_name(api)
    title = f'{api_name} device drivers'
    underline = '#' * len(title)

    print_block(f'''\
    .. _{api}_device_drivers:

    {title}
    {underline}

    This is an automatically generated [#footnote]_ index of
    {api_name} drivers.

    .. rst-class:: rst-columns
    ''', string_io)

    for driver in sorted(drivers, key=lambda driver: driver.name):
        print(f'- :ref:`{driver_ref_target(driver)}`', file=string_io)

    print_block('''\

    .. rubric:: Footnotes

    .. [#footnote]
       The drivers in the list were discovered, and their documentation
       created, via Doxygen comments in driver source code.
    ''', string_io)

    write_if_updated(api_dir / 'index.rst', string_io.getvalue())

def write_per_driver_orphans(drivers: List[Driver], api_dir: Path) -> None:
    # Write the f'{api_dir}/{driver}.rst' orphan files for each 'driver'
    # in 'drivers'.
    api = drivers[0].api

    logging.debug('%s: updating :orphan: files for %d drivers',
                  api, len(drivers))

    num_written = 0
    for driver in drivers:
        string_io = io.StringIO()
        print_driver_page(driver, string_io)
        written = write_if_updated(api_dir / driver_rst_name(driver),
                                   string_io.getvalue())
        if written:
            num_written += 1

    logging.debug('%s: done writing :orphan: files, %d files needed updates',
                  api, num_written)

def print_driver_page(driver: Driver, string_io: io.StringIO) -> None:
    # Print the per-driver content for 'driver' to 'string_io'.
    # The caller is responsible for saving to disk.

    title = driver_title(driver)
    underline = '#' * len(title)

    # TODO: link to the API reference page for the API. Requires
    # hand-rolling and maintaining a map or ensuring the ref targets
    # match the directory names.
    print_block(f'''\
    :orphan:

    .. _{driver_ref_target(driver)}:

    {title}
    {underline}

    Driver API: {api_full_name(driver.api)}.

    Device instances
    ****************
    ''', string_io)

    ncompatibles = len(driver.compatibles)
    if ncompatibles == 0:
        print_block('''\
        This driver does not create ``struct device`` instances from
        nodes in the :ref:`devicetree <dt-guide>`. Information on how to get
        device pointers may be available below.
        ''', string_io)
    elif ncompatibles == 1:
        print_block(f'''\
        This driver creates ``struct device`` instances from nodes in the
        :ref:`devicetree <dt-guide>` with compatible
        :dtcompatible:`{driver.compatibles[0]}`.
        ''', string_io)
    elif ncompatibles > 1:
        compatibles_list = '\n'.join(f'- :dtcompatible:`{compatible}`'
                                     for compatible in driver.compatibles)
        print_block(f'''\
        This driver creates ``struct device`` instances from nodes in the
        :ref:`devicetree <dt-guide>` with the following compatibles:

        {compatibles_list}
        ''', string_io)

    print_block(f'''\
    Description
    ***********

    .. doxygenfile:: {driver.path.name}
       :sections: detaileddescription
    ''', string_io)

def api_full_name(api: str) -> str:
    # Full, human readable name for an API (TODO)

    return api

def api_out_dir(api: str, out_dir: Path) -> Path:
    # Where should drivers for API 'api' go underneath 'out_dir'?

    return out_dir / api

def driver_title(driver: Driver) -> str:
    # What is the title of the .rst page for 'driver'?

    return f'{driver.name} ({driver.path.name})'

def driver_rst_name(driver: Driver) -> str:
    # What is the name of the .rst file corresponding to 'driver'?

    return f'{driver.path.stem}.rst'

def driver_ref_target(driver: Driver) -> str:
    # Return the sphinx ':ref:' target name for a driver.

    stem = Path(driver.path).stem
    return 'driver_' + re.sub('[^a-z0-9]', '_', stem.lower())

def write_if_updated(path: Path, s: str) -> bool:
    # gen_helpers.write_if_updated() wrapper that handles logging and
    # creating missing parents, as needed.

    if not path.parent.is_dir():
        path.parent.mkdir(parents=True)
    written = gen_helpers.write_if_updated(path, s)
    logger.debug('%s %s', 'wrote' if written else 'did NOT write', path)
    return written

def print_block(block: str, string_io: io.StringIO) -> None:
    # Helper for dedenting and printing a triple-quoted RST block.
    # (Just a block of text, not necessarily just a 'code-block'
    # directive.)

    print(textwrap.dedent(block), file=string_io)

if __name__ == '__main__':
    main()
