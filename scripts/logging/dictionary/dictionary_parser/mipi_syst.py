#!/usr/bin/env python3
#
# Copyright (c) 2022 - 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Utilities for MIPI Sys-T Collateral XML data
"""

from xml.sax.saxutils import escape


XML_HEADER = """
<?xml version="1.0" encoding="utf-8"?>
<syst:Collateral xmlns:syst="http://www.mipi.org/1.0/sys-t"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://www.mipi.org/1.0/sys-t
                        https://www.mipi.org/schema/sys-t/sys-t_1-0.xsd">

  <syst:Client Name="Zephyr">
"""

XML_FOOTER = """
  </syst:Client>

</syst:Collateral>
"""

XML_MODULE_HEADER = """
    <syst:Modules>"""

XML_MODULE_EACH = """
      <syst:Module ID="{0}"><![CDATA[{1}]]></syst:Module>"""

XML_MODULE_FOOTER = """
    </syst:Modules>
"""

XML_CATALOG32_HEADER = """
    <syst:Catalog32>"""

XML_CATALOG32_EACH = """
      <syst:Format ID="0x{0:08x}"><![CDATA[{1}]]></syst:Format>"""

XML_CATALOG32_FOOTER = """
    </syst:Catalog32>
"""

XML_CATALOG64_HEADER = """
    <syst:Catalog64>"""

XML_CATALOG64_EACH = """
      <syst:Format ID="0x{0:016x}"><![CDATA[{1}]]></syst:Format>"""

XML_CATALOG64_FOOTER = """
    </syst:Catalog64>
"""

XML_GUIDS = """
    <syst:Guids>
      <syst:Guid   ID="{00000000-0000-0000-0000-000000000000}"
                 Mask="{00000000-0000-0000-FF00-000000000000}"><![CDATA[Zephyr]]></syst:Guid>
    </syst:Guids>
"""


def __gen_syst_modules(database):
    """
    Generate syst:Modules, which corresponds to log source ID and name
    """
    if 'log_subsys' not in database.database:
        return ""

    if 'log_instances' not in database.database['log_subsys']:
        return ""

    instances = database.database['log_subsys']['log_instances']

    if not instances:
        # Empty dictionary: no instances
        return ""

    xml = XML_MODULE_HEADER

    for _, one_inst in instances.items():
        xml += XML_MODULE_EACH.format(one_inst['source_id'], escape(one_inst['name']))

    xml += XML_MODULE_FOOTER

    return xml


def __gen_syst_catalog(database):
    """
    Generate syst:Catalog, which corresponds to log strings
    """
    if not database.has_string_mappings():
        return ""

    if database.is_tgt_64bit():
        xml = XML_CATALOG64_HEADER
        fmt = XML_CATALOG64_EACH
    else:
        xml = XML_CATALOG32_HEADER
        fmt = XML_CATALOG32_EACH

    for addr, one_str in database.get_string_mappings().items():
        xml += fmt.format(addr, one_str)

    if database.is_tgt_64bit():
        xml += XML_CATALOG64_FOOTER
    else:
        xml += XML_CATALOG32_FOOTER

    return xml


def gen_syst_xml_file(database):
    """
    Generate MIPI Sys-T Collateral XML data
    """
    xml = XML_HEADER

    xml += XML_GUIDS

    xml += __gen_syst_modules(database)

    xml += __gen_syst_catalog(database)

    xml += XML_FOOTER

    return xml
