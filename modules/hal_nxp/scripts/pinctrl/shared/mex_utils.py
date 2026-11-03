# SPDX-License-Identifier: Apache-2.0

"""
Convenience methods, used to get board and processor data from MEX files
"""

import logging
import xml.etree.ElementTree as ET

# MEX file has a default namespace, map it here
NAMESPACES = {'mex': 'http://mcuxpresso.nxp.com/XSD/mex_configuration_14'}


class NxpMexUtil:
    def __init__(self, mexfile):
        """
        Initialize instance of this utility class
        @param mexfile: mex file to parse for board name
        """
        self.mexfile = mexfile

    def get_pins_version(self):
        """
        Gets version of pins tool from MEX file.
        @param self: instance of this class
        @ret version as integer
        """
        try:
            config_tree = ET.parse(self.mexfile)
            pins_version = config_tree.getroot().find('mex:tools/mex:pins', NAMESPACES)
            if pins_version is None:
                return 0.0
            return float(pins_version.get('version'))
        except ET.ParseError:
            logging.error("Malformed XML tree %s", self.mexfile)
            return None
        except OSError:
            logging.error("File %s could not be opened", self.mexfile)
            return None

    def get_board_name(self):
        """
        Extracts board name from a mex file
        @param self: instance of this class
        """
        try:
            config_tree = ET.parse(self.mexfile)
            if config_tree.getroot().find('mex:common/mex:board', NAMESPACES) is None:
                return self.get_processor_name() + '-board'
            return config_tree.getroot().find('mex:common/mex:board', NAMESPACES).text
        except ET.ParseError:
            logging.error("Malformed XML tree %s", self.mexfile)
            return None
        except OSError:
            logging.error("File %s could not be opened", self.mexfile)
            return None

    def get_processor_name(self):
        """
        Extracts processor name from a mex file
        @param self: instance of this class
        """
        try:
            config_tree = ET.parse(self.mexfile)
            processor = config_tree.getroot().find('mex:common/mex:processor', NAMESPACES)
            if processor is None:
                raise RuntimeError("Cannot locate processor name in MEX file")
            return processor.text
        except ET.ParseError:
            logging.error("Malformed XML tree %s", self.mexfile)
            return None
        except OSError:
            logging.error("File %s could not be opened", self.mexfile)
            return None

    def get_package_name(self):
        """
        Extracts package name from a mex file
        @param self.mexfile: mex file to parse for package name
        @param self: instance of this class
        """
        try:
            config_tree = ET.parse(self.mexfile)
            package = config_tree.getroot().find('mex:common/mex:package', NAMESPACES)
            if package is None:
                raise RuntimeError("Cannot locate package name in MEX file")
            return package.text
        except ET.ParseError:
            logging.error("Malformed XML tree %s", self.mexfile)
            return None
        except OSError:
            logging.error("File %s could not be opened", self.mexfile)
            return None
