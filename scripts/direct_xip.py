#!/usr/bin/env python3
#
# Copyright (c) 2021, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import sys
import os
import pathlib
import shutil

ZEPHYR_BASE = os.environ.get("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "dts"))

from dts.dtlib import DT

# Slot to alternative slot mapping. Only below configurations are allowed.
CROSS_PARTITIONS = {
    'slot0_partition' : 'slot1_partition',
    'slot1_partition' : 'slot0_partition',
    'slot0_ns_partition' : 'slot1_ns_partition',
    'slot1_ns_partition' : 'slot0_ns_partition'
}

def gen_xip_overlay(partition, path):
    """Generate overlay file for selected partition

    Parameters
    ----------
    partition : str
        String representing the partition for which the DTS overlay will be
        generated.
    path: pathlib.Path
        Path for an overlay file; the file will be overwritten if exists.

    Raises
    ------
    BaseException
        On any non-recoverable error, with string set to error message.
    """
    dts_overlay = [
                    '/ {\n',
                    'chosen {\n'
                    'zephyr,code-partition = &' + partition + ';\n',
                    '};\n',
                    '};'
                ]

    try:
        with open(path,'wt') as dtf:
            dtf.writelines(dts_overlay)
            dtf.close()

    except:
        raise BaseException("Failed to create " + str(path) + "DTC overlay")


def gen_ccache_conf(s_path, d_path, overlay_name):
    """Generate CMakeCache.txt based on configuration of existing build

    Function takes CMakeCache.txt file from source build, at s_path directory,
    and generates new configuration for build at d_path directory; function adds
    provided overlay to list of DTC overlays

    Parameters
    ----------
    s_path: pathlib.Path
        Path to application build directory that will be used as a base for
        a new build.
    d_path: pathlib.Path
        Path to destination build directory.
    overlay_name: string
        Name of DTS overlay to be added to destination build configuration;
        the overlay file is expected to be present within d_path.

    Raises
    ------
    BaseException
        On any non-recoverable error, with string set to error message.
    """
    in_lines = []
    s_ccache_path = s_path / 'CMakeCache.txt'
    d_ccache_path = d_path / 'CMakeCache.txt'
    d_dts_path = d_path / overlay_name

    try:
        with open(s_ccache_path, 'rt') as occ:
            in_lines = occ.readlines()
    except:
        raise BaseException('Failed to read source configuration ' + str(s_ccache_path))

    # Search for path the original build uses
    o_path = None
    marker = 'APPLICATION_BINARY_DIR:PATH='
    for line in in_lines:
        if line.startswith(marker):
            o_path = line[len(marker):-1]
            break

    if not o_path:
        raise BaseException('Failed to extract ' + marker + ' from ' +
                str(s_ccache_path))

    # Generate new file with all the o_path replaced with the d_path
    try:
        with open(d_ccache_path, 'wt') as occ:
            for line in in_lines:
                if line.startswith('DTC_OVERLAY_FILE:STRING='):
                    occ.write(line[:-1] + ' ' + str(d_dts_path) + '\n')
                else:
                    occ.write(line.replace(o_path, str(d_path)))

    except:
        raise BaseException('Failed to write ' + str(d_ccache_path))


def main():
    # ZEPHYR_BASE needs to be set as the script will actually build the product
    if not ZEPHYR_BASE:
        raise BaseException('The ZEPHYR_BASE environment has not been set')

    if len(sys.argv) != 2:
        raise BaseException('Expected no more than build directory name')

    # First need to successfully build the product or confirm it has already been
    # built.
    o_path = pathlib.Path(pathlib.Path(sys.argv[1]).resolve())
    print('Complete original build ' + str(o_path) + '\n')
    origin_ret = os.system('cd ' + str(o_path) + ' && ninja')

    if origin_ret != 0:
        raise BaseException('Failed to re-build the ' + str(o_path))

    # Now, get the code-partition chosen to figure out for which slot the
    # original build has been done. The original build needs to complete
    # successfully as the data will be used to generate the alternative slot
    # configuration
    o_dts_path = pathlib.Path(str(o_path) + '/zephyr/zephyr.dts')

    try:
        dts = DT(o_dts_path)
    except Exception as e:
        raise BaseException('Failed to parse source DTS ' + str(o_dts_path) +
                '\n(' + str(e) + ')')

    chosen_code_partition = str(dts.get_node('/chosen').props['zephyr,code-partition'])
    if not chosen_code_partition:
        raise BaseException('Chosen zephyr,code-partition does not seem to be set')

    # Get the phandle for for the code_partition
    chosen_partition_pat = 'zephyr,code-partition = &'
    chosen_partition_pat_len = len(chosen_partition_pat)
    if len(chosen_partition_pat) >= len(chosen_code_partition):
        raise 'Could not extract ' + chosen_code_partition

    o_partition = chosen_code_partition[chosen_partition_pat_len:-1].strip()

    if o_partition not in CROSS_PARTITIONS:
        raise BaseException('Not valid partition chosen: ' + chosen_code_partition)

    x_partition =  CROSS_PARTITIONS[o_partition]
    x_path = o_path.with_name(o_path.name + '_' + x_partition)
    x_dts_path = x_path / 'xip.overlay'

    print('\nThe ' + str(o_path) + ' has been built for ' + o_partition)
    print('The ' + str(x_path) + ' will be build for ' + x_partition)
    print('Preparing configuration for ' + x_partition + '...\n')

    # Create the alternative build directory and start filling it with files.
    try:
        x_path.mkdir()

        gen_xip_overlay(x_partition, x_dts_path)
        gen_ccache_conf(o_path, x_path, x_dts_path.name)

    except FileExistsError as e:
        # Not able to overwrite build directory. The script does not know why the
        # directory already exists and does not want to take decision upon its
        # faith.
        raise BaseException('Destination build path ' + str(x_path) + ' exists.' +
                ' Refusing to overwrite.')

    # Invoke cmake to complete configuration
    x_ret = os.system('cd ' + str(x_path) + ' && cmake -GNinja --config CMakeCache.txt')
    if x_ret != 0:
        raise BaseException('Failed to prepare build directory for ' + str(x_path))

    # Now the .config needs to be taken from the original build and placed here.
    o_config = o_path.joinpath('zephyr/.config')
    x_config = x_path.joinpath('zephyr/.config')

    try:
        shutil.copy(o_config, x_config)
    except:
        raise BaseException('Failed to copy ' + str(o_config) + ' to ' + str(x_config))

    # Finally try to build the alternative build
    print('\nBuilding for code located at ' + x_partition + '...\n')
    f_ret = os.system('cd ' + str(x_path) + ' && ninja')

    if f_ret != 0:
        raise BaseException('Failed to build alternative build ' + str(x_path))

    print('\nSUCCESS!\n')

if __name__ == '__main__':
    try:
        main()
    except (BaseException) as e:
        print(e)
        sys.exit(1)

    sys.exit(0)
