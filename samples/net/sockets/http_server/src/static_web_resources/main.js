/*
 * Copyright (c) 2024, Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */
async function fetchUptime()
{
	try {
		const response = await fetch("/uptime");
		if (!response.ok) {
			throw new Error(`Response status: ${response.status}`);
		}

		const json = await response.json();
		const uptime = document.getElementById("uptime");
		uptime.innerHTML = "Uptime: " + json + " milliseconds"
	} catch (error) {
		console.error(error.message);
	}
}

async function postLed(state)
{
	try {
		const payload = JSON.stringify({"led_num" : 0, "led_state" : state});

		const response = await fetch("/led", {method : "POST", body : payload});
		if (!response.ok) {
			throw new Error(`Response satus: ${response.status}`);
		}
	} catch (error) {
		console.error(error.message);
	}
}

function setNetStat(json_data, stat_name)
{
	document.getElementById(stat_name).innerHTML = json_data[stat_name];
}

window.addEventListener("DOMContentLoaded", (ev) => {
	/* Fetch the uptime once per second */
	setInterval(fetchUptime, 1000);

	/* POST to the LED endpoint when the buttons are pressed */
	const led_on_btn = document.getElementById("led_on");
	led_on_btn.addEventListener("click", (event) => {
		console.log("led_on clicked");
		postLed(true);
	})

	const led_off_btn = document.getElementById("led_off");
	led_off_btn.addEventListener("click", (event) => {
		console.log("led_off clicked");
		postLed(false);
	})

	/* Setup websocket for handling network stats */
	const ws = new WebSocket("/");

	ws.onmessage = (event) => {
		const data = JSON.parse(event.data);
		setNetStat(data, "bytes_recv");
		setNetStat(data, "bytes_sent");
		setNetStat(data, "ipv6_pkt_recv");
		setNetStat(data, "ipv6_pkt_sent");
		setNetStat(data, "ipv4_pkt_recv");
		setNetStat(data, "ipv4_pkt_sent");
		setNetStat(data, "tcp_bytes_recv");
		setNetStat(data, "tcp_bytes_sent");
	}
})
