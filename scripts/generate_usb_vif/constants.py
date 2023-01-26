#!/usr/bin/env python3

# Copyright (c) 2022 The Chromium OS Authors
# SPDX-License-Identifier: Apache-2.0

"""This file contains all constants defined to be used by all other scripts
in this folder i.e, generate_usb_vif"""

XML_ENCODING = "utf-8"
XML_ELEMENT_NAME_PREFIX = "vif"
XML_ROOT_ELEMENT_NAME = "VIF"
XML_NAMESPACE_ATTRIBUTES = {
    "xmlns:opt": "http://compliance.usb.org/cv/VendorInfoFile/Schemas/Current/VendorInfoFileOptionalContent.xsd",
    "xmlns:xsi": "http://www.w3.org/2001/XMLSchema",
    "xmlns:vif": "http://compliance.usb.org/cv/VendorInfoFile/Schemas/Current/VendorInfoFile.xsd",
}

NAME = "name"
VALUE = "value"
TEXT = "text"
ATTRIBUTES = "attributes"
CHILD = "child"
COMPONENT = "Component"
TRUE = "true"
FALSE = "false"

SINK_PDOS = "sink-pdos"
SINK_PDO = "SnkPDO"
SINK_PDO_SUPPLY_TYPE = "Snk_PDO_Supply_Type"
SINK_PDO_VOLTAGE = "Snk_PDO_Voltage"
SINK_PDO_OP_CURRENT = "Snk_PDO_Op_Current"
SINK_PDO_MIN_VOLTAGE = "Snk_PDO_Min_Voltage"
SINK_PDO_MAX_VOLTAGE = "Snk_PDO_Max_Voltage"
SINK_PDO_OP_POWER = "Snk_PDO_Op_Power"
PD_POWER_AS_SINK = "PD_Power_As_Sink"
NUM_SINK_PDOS = "Num_Snk_PDOs"
MODEL_PART_NUMBER = "Model_Part_Number"
EPR_SUPPORTED_AS_SINK = "EPR_Supported_As_Snk"
NO_USB_SUSPEND_MAY_BE_SET = "No_USB_Suspend_May_Be_Set"
HIGHER_CAPABILITY_SET = "Higher_Capability_Set"
FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE = "FR_Swap_Reqd_Type_C_Current_As_Initial_Source"

VIF_SPEC_ELEMENTS = {
    "VIF_Specification": {
        TEXT: "3.19",
    },
    "VIF_App": {
        CHILD: {
            "Vendor": {
                TEXT: "USB-IF",
            },
            "Name": {
                TEXT: "VIF Editor",
            },
            "Version": {
                TEXT: "3.2.4.0",
            }
        }
    },
    "VIF_Product_Type": {
        TEXT: "Port Product",
        ATTRIBUTES: {
            "value": "0",
        },
    },
    "Certification_Type": {
        TEXT: "End Product",
        ATTRIBUTES: {
            "value": "0",
        },
    }
}

VIF_ELEMENTS = ["VIF_Specification", "VIF_App", "Vendor", "Name", "Version",
                "Vendor_Name", "VIF_Product_Type", "Certification_Type",
                MODEL_PART_NUMBER, COMPONENT, SINK_PDOS, SINK_PDO,
                SINK_PDO_SUPPLY_TYPE, SINK_PDO_VOLTAGE, SINK_PDO_OP_CURRENT,
                SINK_PDO_MIN_VOLTAGE, SINK_PDO_MAX_VOLTAGE, SINK_PDO_OP_POWER,
                PD_POWER_AS_SINK, PD_POWER_AS_SINK, NUM_SINK_PDOS,
                EPR_SUPPORTED_AS_SINK, NO_USB_SUSPEND_MAY_BE_SET,
                HIGHER_CAPABILITY_SET, ]

DT_VIF_ELEMENTS = {
    SINK_PDOS: "SnkPdoList",
}

PDO_TYPE_FIXED = 0
PDO_TYPE_BATTERY = 1
PDO_TYPE_VARIABLE = 2
PDO_TYPE_AUGUMENTED = 3

PDO_TYPES = {
    PDO_TYPE_FIXED: "Fixed",
    PDO_TYPE_BATTERY: "Battery",
    PDO_TYPE_VARIABLE: "Variable",
    PDO_TYPE_AUGUMENTED: "Augmented",
}
