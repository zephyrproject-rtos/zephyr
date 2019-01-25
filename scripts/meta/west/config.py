# Copyright (c) 2018, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''
Configuration file handling, using the standard configparser module.
'''

import configparser
import os
import platform

from west.util import west_dir


# Configuration values.
#
# Initially empty, populated in read_config(). Always having this available is
# nice in case something checks configuration values before the configuration
# file has been read (e.g. the log.py functions, to check color settings, and
# tests).
config = configparser.ConfigParser()


def read_config():
    '''
    Reads all configuration files, making the configuration values available as
    a configparser.ConfigParser object in config.config. This object works
    similarly to a dictionary: config.config['foo']['bar'] gets the value for
    key 'bar' in section 'foo'.

    Git conventions for configuration file locations are used. See the FILES
    section in the git-config(1) man page.

    The following configuration files are read.

    System-wide:

        Linux:   /etc/westconfig
        Mac OS:  /usr/local/etc/westconfig
        Windows: %PROGRAMDATA%\\west\\config

    User-specific:

        $XDG_CONFIG_HOME/west/config (on Linux)
          and
        ~/.westconfig

        ($XDG_CONFIG_DIR defaults to ~/.config/ if unset.)

    Instance-specific:

        <West base directory>/west/config

    Configuration values from later configuration files override configuration
    from earlier ones. Instance-specific configuration values have the highest
    precedence, and system-wide the lowest.
    '''

    # Gather (potential) configuration file paths

    # System-wide and user-specific

    if platform.system() == 'Linux':
        # Probably wouldn't hurt to check $XDG_CONFIG_HOME (defaults to
        # ~/.config) on all systems. It's listed in git-config(1). People were
        # iffy about it as of writing though.
        files = ['/etc/westconfig',
                 os.path.join(os.environ.get('XDG_CONFIG_HOME',
                                             os.path.expanduser('~/.config')),
                              'west', 'config')]

    elif platform.system() == 'Darwin':  # Mac OS
        # This was seen on a local machine ($(prefix) = /usr/local)
        files = ['/usr/local/etc/westconfig']
    elif platform.system() == 'Windows':
        # Seen on a local machine
        files = [os.path.expandvars('%PROGRAMDATA%\\west\\config')]

    files.append(os.path.expanduser('~/.westconfig'))

    # Repository-specific

    files.append(os.path.join(west_dir(), 'config'))

    #
    # Parse all existing configuration files
    #

    config.read(files, encoding='utf-8')


def use_colors():
    # Convenience function for reading the color.ui setting
    return config.getboolean('color', 'ui', fallback=True)
