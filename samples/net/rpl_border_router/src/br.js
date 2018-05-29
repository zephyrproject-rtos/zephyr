/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

var connected;
var first_run;
var ws;
var interfaces;
var column;
var iface_table_id;
var neighbors_table_id;
var routes_table_id;

function init() {
    ws = new WebSocket(location.origin.replace("http", "ws") + "/ws");

    interfaces = {};
    first_run = "true";
    connected = "false";
    changeConnectText();
    createIfaceTable();
    createNeighborTable();
    createRouteTable();

    ws.onopen = function() {
	output("Connection opened");
	connected = "true";
	changeConnectText();
    };

    ws.onmessage = function(e) {
	//output("Data received: " + e.data);
	var info = JSON.parse(e.data);

	try {
	    if (info.interface_configuration.length > 0) {
		output("Network interface configuration received");
		print_iface_configuration(info.interface_configuration);
	    }
	} catch(err) {
	}

	try {
	    if (info.rpl_configuration.length > 0) {
		output("RPL configuration received");
		print_rpl_configuration(info.rpl_configuration);
	    }
	} catch(err) {
	}

	try {
	    if (info.neighbors.length > 0) {
		output("Neighbor information received");
		add_neighbors(info.neighbors);
	    }
	} catch(err) {
	}

	try {
	    if (info.routes.length > 0) {
		output("Route information received");
		add_routes(info.routes);
	    }
	} catch(err) {
	}

	try {
	    if (info.topology.nodes.length > 0) {
		output("Topology information received");
		draw_topology(info.topology);
	    }
	} catch(err) {
	}

	if (first_run == "true") {
	    first_run = "false";
	    select_default_tab();
	}
    };

    ws.onclose = function() {
	output("Connection closed");
	connected = "false";
	changeConnectText();
    };

    ws.onerror = function(e) {
	output("Data error (see console)");
	console.log(e)
    };
}

function print_iface_configuration(config) {
    for (var i = 0; i < config.length; i++) {
	print_iface(config[i]);
    }
}

function print_iface(iface) {

    for (const property in iface) {
	var table_id = "iface_table_" + property;

	//output("iface " + property);

	print_iface_settings(iface, property);
    }
}

function print_iface_settings(iface, property) {
    var row = table_insert_row(iface_table_id);

    column = 0;

    for (const value in property) {
	print_iface_data(property, iface[property][value], row, column++);
    }
}

function print_iface_data(iface_id, setting, row, column) {
    for (const key in setting) {
	if (key == "Type") {
	    interfaces[iface_id] = setting[key];
	}

	if (key == "IPv6" || key == "IPv4") {
	    /* TODO: Print detailed IPv6 and IPv4 data */
	    var addresses = "";
	    //output(key + " has " + setting[key].length + " addresses");

	    for (var i = 0; i < setting[key].length; i++) {
		for (const address in setting[key][i]) {
		    addresses += address + " ";
		}
	    }

	    //output("insert " + iface_table_id + " " + addresses + " to column " + column);
	    table_add_data(iface_table_id, row, addresses, column, "wrappable");
	} else {
	    //output("insert " + iface_table_id + " " + setting[key].toString() + " to column " + column);
	    table_add_data(iface_table_id, row, setting[key], column);
	}
    }
}

function print_rpl_configuration(config) {
    var table_id = "rpl_table";
    table_create(document.getElementById("rpl"), table_id);

    for (var i = 0; i < config.length; i++) {
	print_rpl_data(config[i], table_id);
    }
}

function print_rpl_data(property, table_id) {
    for (const value in property) {
	//output(value + "=" + property[value]);
	print_rpl(value, property[value], table_id);
    }
}

function print_rpl(label, property, table_id) {
    var row = table_insert_row(table_id);

    table_add_data(table_id, row, label, 0);
    table_add_data(table_id, row, property, 1);
}

function add_neighbors(config) {
    for (var i = 0; i < config.length; i++) {
	add_neighbor_iface_data(config[i]);
    }
}

function add_neighbor_iface_data(iface) {
    for (const value in iface) {
	//output("value " + value.toString());
	if (interfaces[value] == "IEEE 802.15.4") {
	    add_neighbor_data(iface, iface[value]);
	}
    }
}

function toogle(b) {
   var req = new Object();
   var details = new Object();
   var jsonString;

   details.command = "toggle";
   details.ipv6_addr = b.getAttribute("IPv6_address");

   req.coap = details;

   jsonString = JSON.stringify(req);
   //output("request " + jsonString);

   if (connected == "true") {
        ws.send(jsonString);
   }
}

function add_neighbor_data(iface, property) {
    for (var i = 0; i < property.length; i++) {
        //output("property " + property[i].toString());
        if (property[i]["Operation"] == "delete") {
            delete_neighbor(property[i]["IPv6 address"]);
        } else {
            var neighbor_row = table_insert_row(neighbors_table_id);
            var value = property[i]["IPv6 address"];
            var toggle;

            toggle = table_add_button(neighbors_table_id, neighbor_row, 'Toggle', 'Resources', '', "toggle(this);");
            toggle.setAttribute("IPv6_address", value);

            neighbor_row.setAttribute("IPv6_address", value);

            table_add_data(neighbors_table_id, neighbor_row, value, 'IP address', '');
        }
    }
}

function delete_neighbor(ipv6_addr) {
    var table = document.getElementById(neighbors_table_id);

    for (var i = 0, row; row = table.rows[i]; i++) {
        if (row.getAttribute("IPv6_address") == ipv6_addr) {
            output("deleting " + ipv6_addr);
            table.deleteRow(i);
            return;
        }
    }
}

function add_routes(config) {
    for (var i = 0; i < config.length; i++) {
	add_route_iface_data(config[i]);
    }
}

function add_route_iface_data(iface) {
    for (const value in iface) {
	//output("iface " + value.toString());
	add_route_data(iface[value]);
    }
}

function add_route_data(property) {
    for (var i = 0; i < property.length; i++) {
        if (property[i]["Operation"] == "delete") {
		delete_route(property[i]["IPv6 prefix"]);
	} else {
            var route_row = table_insert_row(routes_table_id);

            table_add_data(routes_table_id, route_row, property[i]["Link address"], 'Link address', '');
            table_add_data(routes_table_id, route_row, property[i]["IPv6 prefix"], 'IPv6 prefix', '');
            route_row.setAttribute("IPv6_prefix", property[i]["IPv6 prefix"]);
	}
    }
}

function delete_route(ipv6_prefix) {
    var table = document.getElementById(routes_table_id);

    for (var i = 0, row; row = table.rows[i]; i++) {
        if (row.getAttribute("IPv6_prefix") == ipv6_prefix) {
            output("deleting " + ipv6_prefix);
            table.deleteRow(i);
            return;
        }
    }
}

function print_table_values(property, table_id) {
    var row = table_insert_row(table_id);
    var head_row;
    var thead;

    column = 0;

    if (!header_added) {
	var table = document.getElementById(table_id);
	thead = table.createTHead();
	head_row = thead.insertRow();
    }

    for (const value in property) {
	if (!header_added) {
	    table_column_header(table_id, head_row, value, column);
	}

	table_add_data(table_id, row, property[value], column++);
    }

    header_added = true;
}

function select_default_tab() {
    configuration();
}

function draw_topology(topology) {
    var network;
    var options = { interaction: { hover: true } };

    var container = document.getElementById('topology');
    container.innerHTML = "";

    network = new vis.Network(container, topology, options);
}

function configuration() {
    document.getElementById("interface_open").click();
}

function rpl() {
    document.getElementById("rpl_open").click();
}

function neighbors() {
    document.getElementById("neighbors_open").click();
}

function routes() {
    document.getElementById("routes_open").click();
}

function zconsole() {
    document.getElementById("zconsole_open").click();
}

function openTab(evt, tabName) {
    var i, tabcontent, tablinks;

    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }

    tablinks = document.getElementsByClassName("tablinks");
    for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
    }

    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " active";
}

function onSubmit() {
    var input = document.getElementById("input");
    ws.send(input.value);
    input.value = "";
    input.focus();
}

function wsConnect() {
    if (connected == "false") {
	location.reload();
    }
}

function changeConnectText() {
    if (connected == "false") {
	document.getElementById("connect_button").innerText= "Connect";
    } else {
	document.getElementById("connect_button").innerText= "Close";
    }
}

function onConnectClick() {
    if (connected == "true") {
	ws.close();
	connected = "false";
	changeConnectText();
    } else {
	changeConnectText();
	wsConnect();
    }
}

function escape(str) {
    return str.replace(/&/, "&amp;").replace(/</, "&lt;").
	replace(/>/, "&gt;").replace(/"/, "&quot;"); // "
}

function output(str) {
    var log = document.getElementById("output");
    log.innerHTML += escape(str) + "<br>";
}

function table_create(parent, table_id) {
    var tbl = document.createElement("table");
    tbl.setAttribute("id", table_id);
    parent.appendChild(tbl);
}

function table_header(table_id, label) {
    var escaped_label = escape(label);
    var table = document.getElementById(table_id);
    var table_head = table.tHead;

    if (!table_head) {
	table_head = table.createTHead();
    }

    var text_label = document.createTextNode(escaped_label);

    table_head.appendChild(escaped_label);
    table.appendChild(table_head);
}

function table_column_header(table_id, head_row, label, column) {
    var escaped_label = escape(label);
    var text_label = document.createTextNode(escaped_label);
    var cell_th = head_row.insertCell(column);

    cell_th.appendChild(text_label);
}

function table_insert_row(table_id) {
    var table = document.getElementById(table_id);
    return row = table.insertRow();
}

function table_add_data(table_id, row, value, column, cell_class) {
    var escaped_value = escape(value);
    var table = document.getElementById(table_id);
    var cell_value  = row.insertCell(column);
    var text_value = document.createTextNode(escaped_value);

    cell_value.appendChild(text_value);

    if (cell_class) {
	cell_value.className = cell_class;
    }
}

function table_add_button(table_id, row, value, column, cell_class, on_click) {
    var escaped_value = escape(value);
    var table = document.getElementById(table_id);
    var cell_value  = row.insertCell(column);
    var button = document.createElement('input');

    button.setAttribute('type', 'button');
    button.setAttribute('value', value);

    cell_value.appendChild(button);

    if (cell_class) {
        cell_value.className = cell_class;
    }

    if (on_click) {
       button.setAttribute("onclick", on_click);
    }

    return button;
}

function createNeighborTable() {
    var table;

    neighbors_table_id = "neighbors_table";
    table_create(document.getElementById("neighbors"), neighbors_table_id);
    table = document.getElementById(neighbors_table_id);

    thead = table.createTHead();
    head_row = thead.insertRow();
    table_column_header(neighbors_table_id, head_row, 'IP address', 0);
    table_column_header(neighbors_table_id, head_row, 'Resources', 1);
}

function createRouteTable() {
    var table;

    routes_table_id = "routes_table";
    table_create(document.getElementById("routes"), routes_table_id);
    table = document.getElementById(routes_table_id);

    thead = table.createTHead();
    head_row = thead.insertRow();
    table_column_header(routes_table_id, head_row, 'IPv6 prefix', 0);
    table_column_header(routes_table_id, head_row, 'Link address', 1);
}

function createIfaceTable() {
    var table;

    iface_table_id = "iface_table";
    table_create(document.getElementById("iface"), iface_table_id);
    table = document.getElementById(iface_table_id);

    thead = table.createTHead();
    head_row = thead.insertRow();
    table_column_header(iface_table_id, head_row, 'Type', 0);
    table_column_header(iface_table_id, head_row, 'MTU', 1);
    table_column_header(iface_table_id, head_row, 'Link Address', 2);
    table_column_header(iface_table_id, head_row, 'IPv6', 3);
}
