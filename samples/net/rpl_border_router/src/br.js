/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

var connected;
var first_run;
var ws;
var interfaces;
var header_added;
var column;

function init() {
    ws = new WebSocket(location.origin.replace("http", "ws") + "/ws");

    interfaces = {};
    first_run = "true";
    connected = "false";
    changeConnectText();

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
		print_neighbors(info.neighbors);
	    }
	} catch(err) {
	}

	try {
	    if (info.routes.length > 0) {
		output("Route information received");
		print_routes(info.routes);
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
    header_added = false;

    for (const property in iface) {
	var table_id = "iface_table_" + property;

	//output("iface " + property);

	table_create(document.getElementById("iface"), table_id);

	print_iface_settings(iface, property, table_id);
    }
}

function print_iface_settings(iface, property, table_id) {
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
	print_iface_data(property, iface[property][value], table_id,
			 head_row, row, column++);
    }

    header_added = true;
}

function print_iface_data(iface_id, setting, table_id, head_row,
			  row, column) {
    for (const key in setting) {
	if (!header_added) {
	    table_column_header(table_id, head_row, key, column);
	}

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

	    //output("insert " + table_id + " " + addresses + " to column " + column);
	    table_add_data(table_id, row, addresses, column, "wrappable");
	} else {
	    //output("insert " + table_id + " " + setting[key].toString() + " to column " + column);
	    table_add_data(table_id, row, setting[key], column);
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

function print_neighbors(config) {
    var table_id = "neighbors_table";
    header_added = false;
    table_create(document.getElementById("neighbors"), table_id);

    for (var i = 0; i < config.length; i++) {
	print_neighbor_iface_data(config[i], table_id);
    }
}

function print_neighbor_iface_data(iface, table_id) {
    for (const value in iface) {
	//output("iface " + value.toString());
	print_neighbor_data(iface[value], table_id);
    }
}

function print_neighbor_data(property, table_id) {
    for (var i = 0; i < property.length; i++) {
	print_table_values(property[i], table_id);
    }
}

function print_routes(config) {
    var table_id = "routes_table";
    header_added = false;
    table_create(document.getElementById("routes"), table_id);

    for (var i = 0; i < config.length; i++) {
	print_route_iface_data(config[i], table_id);
    }
}

function print_route_iface_data(iface, table_id) {
    for (const value in iface) {
	//output("iface " + value.toString());
	print_route_data(iface[value], table_id);
    }
}

function print_route_data(property, table_id) {
    for (var i = 0; i < property.length; i++) {
	print_table_values(property[i], table_id);
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

	//output("insert " + table_id + " " + property[value] + " to column " + column);
	table_add_data(table_id, row, property[value], column++);
    }

    header_added = true;
}

function select_default_tab() {
    configuration();
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
