#!/usr/bin/python3

from jsonschema import validate

schema = {
    "$schema": "http://json-schema.org/schema#",
    "description": "Schema for border router information",
    "title": "Border Router Information",
    "type": "object",
    "properties": {
        "information": {
            "type": "object",
            "oneOf": [
                { "$ref": "#/definitions/interface_configuration" },
                { "$ref": "#/definitions/rpl_configuration" },
                { "$ref": "#/definitions/neighbors" },
                { "$ref": "#/definitions/routes" },
                { "$ref": "#/definitions/topology" }
            ]
        }
    },

    "definitions": {
        "interface_configuration": {
            "title": "Network Interface Configuration options",
            "type": "array",
            "items": {
	        "type": "object",
                "items" : {
                    "type": "array",
                    "values": {
                        "oneOf": [
                            { "$ref": "#/unicast/ipv6" },
                            { "$ref": "#/unicast/ipv4" },
                            { "$ref": "#/multicast/mcast_ipv6" },
                            { "$ref": "#/multicast/mcast_ipv4" }
                        ]
                    }
                }
            }
        },

        "rpl_configuration": {
            "title": "RPL Configuration options",
            "type": "array",
            "items": {
	        "type": "object",
                "items" : {
                    "RPL mode" : "string",
                    "Objective function" : "string",
                    "Routing metric" : "string",
                    "Mode of operation" : "string",
                    "Send probes to nodes" : "string",
                    "Max instances" : "string",
                    "Max DAG / instance" : "string",
                    "Min hop rank increment" : "string",
                    "Initial link metric" : "string",
                    "RPL preference value" : "string",
                    "DAG grounded by default" : "string",
                    "Default instance id" : "string",
                    "Insert hop-by-hop option" : "string",
                    "Specify DAG when sending DAO" : "string",
                    "DIO min interval" : "string",
                    "DIO doublings interval" : "string",
                    "DIO redundancy value" : "string",
                    "DAO send timer" : "string",
                    "DAO max retransmissions" : "string",
                    "DAO ack expected" : "string",
                    "DIS send periodically" : "string",
                    "DIS interval" : "string",
                    "Default route lifetime unit" : "string",
                    "Default route lifetime" : "string",
                    "Multicast MOP3 route lifetime" : "string"
                }
            }
        },

        "neighbors": {
            "title": "Neighbor information",
            "type": "array",
	    "items": {
	        "type": "object",
                "items" : {
                    "Operation" : "string",
	            "IPv6 address": "string",
	            "Link address": "string",
	            "Is router": "string",
	            "Link metric": "string",
	        }
            }
        },

        "routes": {
            "title": "Network routes",
            "type": "array",
            "items": {
	        "type": "object",
                "items" : {
                    "Operation" : "string",
                    "IPv6 prefix": "string",
                    "IPv6 address": "string",
                    "Link address": "string"
                }
            }
        },

        "topology": {
            "type": "object",
            "properties": {
                "nodes": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "id": {
                                "type": "integer"
                             },
                            "label": {
                                "type": "string"
                            },
                            "title": {
                                "type": "string"
                            }
                        }
                    }
                },
                "edges": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "from": {
                                "type": "integer"
                            },
                            "to": {
                                "type": "integer"
                            }
                        }
                    }
                }
            }
        }
    },

    "unicast": {
        "ipv6": {
            "title": "IPv6 addresses",
            "type": "array",
            "items": {
                "state": "string",
                "type": "string",
                "lifetime": "string"
            }
        },

        "ipv4": {
            "title": "IPv4 addresses",
            "type": "array",
            "items": {
                "state": "string",
                "type": "string",
                "lifetime": "string"
            }
        }
    },

    "multicast": {
        "mcast_ipv6": {
            "title": "IPv6 addresses",
            "type": "array",
            "items": {
                "address": "string"
            }
        },

        "mcast_ipv4": {
            "title": "IPv4 addresses",
            "type": "array",
            "items": {
                "value": "string"
            }
        }
    }
}

validate(
    {
        "interface_configuration": [
            {
                "0x20009000": [
                    {
                        "Type" : "Ethernet"
                    },
                    {
                        "Link address" : "00:04:9F:2A:00:01"
                    },
                    {
                        "MTU" : "1500"
                    },
                    {
                        "IPv6" : [
                            {
                                "fe80::204:9fff:fe2a:1" : {
                                    "state": "preferred",
                                    "type": "autoconf",
                                    "lifetime": "infinite"
                                }
                            },
                            {
                                "2001:db8::1" : {
                                    "state": "preferred",
                                    "type": "manual",
                                    "lifetime": "infinite"
                                }
                            },
                        ]
                    },
                    {
                        "IPv4" : [
                            {
                                "192.0.2.1" : {
                                    "state": "preferred",
                                    "type": "autoconf",
                                    "lifetime": "infinite"
                                }
                            }
                        ]
                    },
                    {
                        "IPv6 multicast" :
                        [
                            "ff02::1",
                            "ff02::1:ff2a:1",
                            "ff02::1:ff00:1"
                        ]
                    },
                    {
                        "IPv4 multicast" :
                        [
                            "224.0.0.251"
                        ]
                    },
                    {
                        "IPv4 gateway" : "0.0.0.0"
                    },
                    {
                        "IPv4 netmask" : "255.255.255.0"
                    },
                ]
            },
            {
                "0x20009a30": [
                    { "Type" : "IEEE 802.15.4" }
                ]
            }
        ],

        "neighbors": [
            {
                "0x20009000": [
                    {
                        "IPv6 address": "2001:db8::1",
                        "Link address": "00:11:22:33:44:55:66:77",
	                "Is router": "true",
	                "Link metric": "0"
                    },
                    {
                        "IPv6 address": "2001:db8::2",
                        "Link address": "77:11:22:33:44:55:66:00",
	                "Is router": "false",
	                "Link metric": "1"
                    },
                    {
                        "Operation" : "delete",
                        "IPv6 address": "2001:db8::3"
                    }
                ]
            },
            {
                "0x20009a30": []
            }
        ],

        "rpl_configuration": [
            { "RPL mode" : "mesh" },
            { "Objective function" : "MRHOF" },
            { "Routing metric" : "none" },
            { "Mode of operation" : "MOP2" },
            { "Send probes to nodes" : "false" },
            { "Max instances" : "1" },
            { "Max DAG / instance" : "2" },
            { "Min hop rank increment" : "256" },
            { "Initial link metric" : "2" },
            { "RPL preference value" : "0" },
            { "DAG grounded by default" : "false" },
            { "Default instance id" : "42" },
            { "Insert hop-by-hop option" : "true" },
            { "Specify DAG when sending DAO" : "true" },
            { "DIO min interval" : "12" },
            { "DIO doublings interval" : "8" },
            { "DIO redundancy value" : "10" },
            { "DAO send timer" : "4" },
            { "DAO max retransmissions" : "4" },
            { "DAO ack expected" : "true" },
            { "DIS send periodically" : "true" },
            { "DIS interval" : "60" },
            { "Default route lifetime unit" : "65535" },
            { "Default route lifetime" : "255" },
            { "Multicast MOP3 route lifetime" : "0" }
        ],

        "routes": [
            {
                "0x20009000": [
                    {
                        "IPv6 prefix" : "fde3:2cda:3eea:4d14:212:4b00:0:2/128",
                        "IPv6 address" : "fe80::212:4b00:0:2",
                        "Link address" : "00:12:4B:00:00:00:00:02"
                    },
                    {
                        "Operation" : "delete",
                        "IPv6 prefix" : "fde3:2cda:3eea:4d14:212:4b00:0:3/128"
                    }

                ]
            },
            {
                "0x20009a30": [
                ]
            }
        ],

        "topology" : {
            "nodes": [
                {
                    "id": 1,
                    "label": "N1",
                    "title": "Node 1"
                },
                {
                    "id": 2,
                    "label": "N2",
                    "title": "Node 2"
                }
                ],
            "edges": [
                {
                    "from": 1,
                    "to": 2
                }
                ]
            }
    }, schema)
