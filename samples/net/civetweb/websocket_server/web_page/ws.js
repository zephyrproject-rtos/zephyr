/*
 * Copyright (c) 2020 Alexander Kozhinov <AlexanderKozhinov@yandex.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

var connected
var first_run
var ws

var LogMsgType = {
	TEXT: 'text',
	BINARY: 'binary'
}

var LogMsgSender = {
	SYSTEM: 'system',
	LOCAL: 'local',
	REMOTE: 'remote',
	CONSOLE: 'console'
}

var MAX_LOG_SIZE = 10

var SECOND = 1000
var MINUTE = SECOND * 60
var HOUR = MINUTE * 60
var DAY = HOUR * 24
var MONTH = DAY * 30
var YEAR = MONTH * 12

var TIME_UP_PERIOD_SEC = 5

function init() {
	addLogEntry(LogMsgSender.CONSOLE,
		    LogMsgType.TEXT, 'establishing connection to the server...')

	ws = new WebSocket(location.origin.replace("http", "ws") + "/ws")

	first_run = "true"
	connected = "false"

	ws.onopen = function() {
		addLogEntry(LogMsgSender.CONSOLE, LogMsgType.TEXT,
			    'connection opened')

		addLogEntry(LogMsgSender.SYSTEM, LogMsgType.TEXT,
			"The connection was established successfully (in " +
				(Date.now() - ws.__openTime)+ " ms).\n" +
			(ws.extensions ? 'Negotiated Extensions: ' +
				ws.extensions : '') +
			(ws.protocol ? 'Negotiated Protocol: ' +
				ws.protocol : ''))
		connected = "true"
	}

	ws.onmessage = function(e) {
		addLogEntry(LogMsgSender.REMOTE,
			    LogMsgType.TEXT, 'data received: ' + e.data)
	}

	ws.onclose = function() {
		addLogEntry(LogMsgSender.CONSOLE, LogMsgType.TEXT,
			    'connection closed')
		connected = "false"
	}

	ws.onerror = function(e) {
		addLogEntry(LogMsgSender.CONSOLE, LogMsgType.TEXT,
			    'data error: ' + e.data)
		console.log(e)
	}

	var sendText = function(text) {
		addLogEntry(LogMsgSender.LOCAL, LogMsgType.TEXT, text)
		ws.send(text)
	}

	document.getElementById('btn_send').onclick = function(event) {
		event.preventDefault()
		var text2send = document.getElementById('message_text').value
		sendText(text2send)
		scrollLogToBottom()
	}

	document.getElementById('btn_clear_log').onclick = function(event) {
		event.preventDefault()
		clearLog()
	}

	window.setInterval(updateTimestamps, TIME_UP_PERIOD_SEC * SECOND)
}

function updateTimestamps() {
	var entries = document.getElementsByClassName('entries')
	var now = Date.now()

	var timestamp = null
	var entry = null
	var old_time = null
	for (var i = 0; i < entries[0].childElementCount; i++) {
		entry = entries[0].children[i]
        old_time = Number(entry.getAttribute('timestamp'))
		timestamp = entry.getElementsByClassName('timestamp')[0]
		timestamp.innerHTML = formatTimeDifference(now, old_time)
	}
}

function formatTimeDifference(now, then) {
	var difference = Math.abs(now - then)

	if (difference < TIME_UP_PERIOD_SEC * SECOND) {
		return 'just now'
	}
	if (difference < MINUTE) {
		return '< 1 min ago'
	}
	if (difference < HOUR) {
		return Math.round(difference / MINUTE) + ' min ago'
	}
	if (difference < DAY) {
		return Math.round(difference / HOUR) + ' hr ago'
	}
	if (difference < MONTH) {
		return Math.round(difference / DAY) + ' day ago'
	}
	return Math.round(difference / YEAR) + ' yr ago'
}

function blobToHex(blob) {
	// TODO: Implement (seems to be non-trivial).
	return ''
}

function logIsScrolledToBottom() {
	var j = document.getElementsByClassName('log')
	var e = j[0]
	return e.scrollTop + j.height +
		10 /* padding */ >= e.scrollHeight - 10 /* some tolerance */
}

function scrollLogToBottom() {
	var e = document.getElementsByClassName('log')[0]
	e.scrollTop = e.scrollHeight
}

function pruneLog() {
	var e = document.getElementsByClassName('entries')

	if (e.length == 0) {
		return
	}

	if (e[0].children.length == MAX_LOG_SIZE) {
		e[0].children[0].remove()
	}
}

function addLogEntry(sender, type, data) {
	pruneLog()

	if (type == LogMsgType.BINARY) {
		data = '(BINARY MESSAGE: ' + data.size + ' bytes)\n' +
			blobToHex(data)
	} else {
		data = data || '(empty message)'
	}

	var entries = document.getElementsByClassName('entries')

	var entry = entries[0].appendChild(document.createElement('div'))
	entry.classList += 'entry'
	var publisher = entry.appendChild(document.createElement('div'))
	publisher.classList += 'publisher'
	publisher.classList += ' ' + sender
	var content = entry.appendChild(document.createElement('div'))
	content.classList += 'content'
	content.classList += ' ' + type
	var timestamp = publisher.appendChild(document.createElement('div'))
	timestamp.classList += 'timestamp'

	entry.setAttribute('timestamp', '' + Date.now())
	content.innerHTML = data
	timestamp.innerHTML = 'just now'

	var scroll = logIsScrolledToBottom()
	if (scroll) {
		scrollLogToBottom()
	}
}

function clearLog() {
	var entries = document.getElementsByClassName('entries')[0]
    while (entries.firstChild) entries.removeChild(entries.firstChild)
}
