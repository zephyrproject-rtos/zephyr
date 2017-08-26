/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

var connected;
var first_run;
var ws;

function init() {
    ws = new WebSocket(location.origin.replace("http", "ws") + "/ws");

    first_run = "true";
    connected = "false";

    ws.onopen = function() {
	output("Websocket connection opened");
	connected = "true";
    };

    ws.onmessage = function(e) {
	output("Websocket data received: " + e.data);
    };

    ws.onclose = function() {
	output("Websocket connection closed");
	connected = "false";
    };

    ws.onerror = function(e) {
	output("Websocket data error (see console)");
	console.log(e)
    };
}

function escape(str) {
    return str.replace(/&/, "&amp;").replace(/</, "&lt;").
	replace(/>/, "&gt;").replace(/"/, "&quot;"); // "
}

function output(str) {
    var log = document.getElementById("output");
    log.innerHTML += escape(str) + "<br>";
}
