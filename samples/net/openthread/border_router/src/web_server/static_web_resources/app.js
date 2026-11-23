/*
 * Copyright (c) 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

function fetchAPI(url, method = 'GET', body = null) {
	const options = { method: method };
	if (body) {
		options.headers = { 'Content-Type': 'application/x-www-form-urlencoded' };
		options.body = body;
	}
	return fetch(url, options)
		.then(response => {
			if (response.headers.get('content-type')?.includes('application/json')) {
				return response.json();
			} else {
				return response.text().then(text => {
					try { return JSON.parse(text); } catch(e) { return {data: text}; }
				});
			}
		})
		.catch(error => ({ error: error.message }));
}

// ========== Refresh All ==========
function refreshAll() {
	refreshTopology();
	updateDatasetButtonState();
	showStatus(' Refreshed', 'success');
	setTimeout(() => {
		document.getElementById('control-result').innerHTML = '';
	}, 2000);
}

// ========== Thread Control ==========
function startThread() {
	showStatus('⏳ Starting Thread...', 'loading');
	fetchAPI('/api/thread/start').then(data => {
		if (data.error) {
			showStatus(data.error, 'error');
		} else {
			showStatus(data.message, 'success');
			setTimeout(refreshAll, 1000);
		}
	});
}

function stopThread() {
	showStatus('⏳ Stopping Thread...', 'loading');
	fetchAPI('/api/thread/stop').then(data => {
		if (data.error) {
			showStatus(data.error, 'error');
		} else {
			showStatus(data.message, 'success');
			setTimeout(refreshAll, 1000);
		}
	});
}

// ========== Network Configuration ==========
function submitNetworkConfig(event) {
	event.preventDefault();
	const form = document.getElementById('network-config-form');
	const formData = new FormData(form);
	const params = new URLSearchParams();
	for (const [key, value] of formData) {
		if (value) params.append(key, value);
	}

	showStatus('⏳ Saving configuration...', 'loading');
	fetchAPI('/api/network/config', 'POST', params.toString()).then(data => {
		if (data.error) {
			showStatus(data.error, 'error');
		} else {
			showStatus(data.message, 'success');
		}
	});
}

function loadCurrentConfig() {
	showStatus('⏳ Loading configuration...', 'loading');
	fetchAPI('/api/dataset').then(data => {
		if (data.network_name) {
			document.getElementById('network_name').value = data.network_name;
		}
		if (data.pan_id) {
			document.getElementById('pan_id').value = data.pan_id;
		}
		if (data.extpanid && data.extpanid !== 'null') {
			document.getElementById('extpanid').value = data.extpanid.replace(/"/g, '');
		}
		if (data.channel) {
			document.getElementById('channel').value = data.channel;
		}
		if (data.network_key && data.network_key !== 'null') {
			document.getElementById('network_key').value = data.network_key.replace(/"/g, '');
		}
		showStatus('✅ Configuration loaded', 'success');
	}).catch(error => {
		showStatus('❌ Failed to load', 'error');
	});
}

function copyDatasetToClipboard() {
	showStatus('⏳ Getting dataset...', 'loading');
	fetchAPI('/api/dataset/hex').then(data => {
		if (data.error) {
			showStatus('❌ ' + data.error, 'error');
		} else {
			const textarea = document.createElement('textarea');
			textarea.value = data.dataset_hex;
			textarea.style.position = 'fixed';
			textarea.style.opacity = '0';
			document.body.appendChild(textarea);

			if (navigator.clipboard && navigator.clipboard.writeText) {
				navigator.clipboard.writeText(data.dataset_hex).then(() => {
					showStatus('✅ Dataset copied to clipboard! (' + data.length + ' bytes)', 'success');
					document.body.removeChild(textarea);
				}).catch(err => {
					fallbackCopy(textarea, data.length);
				});
			} else {
				fallbackCopy(textarea, data.length);
			}
		}
	});
}

function fallbackCopy(textarea, length) {
	try {
		textarea.select();
		textarea.setSelectionRange(0, 99999);
		const successful = document.execCommand('copy');
		if (successful) {
			showStatus('✅ Dataset copied to clipboard! (' + length + ' bytes)', 'success');
		} else {
			showStatus('❌ Failed to copy to clipboard', 'error');
		}
	} catch (err) {
		showStatus('❌ Failed to copy to clipboard', 'error');
	}
	document.body.removeChild(textarea);
}

function updateDatasetButtonState() {
	fetchAPI('/api/dataset').then(data => {
		const btn = document.getElementById('copy-dataset-btn');
		if (data.network_name && data.network_name !== 'null') {
			btn.disabled = false;
			btn.style.opacity = '1';
			btn.style.cursor = 'pointer';
		} else {
			btn.disabled = true;
			btn.style.opacity = '0.5';
			btn.style.cursor = 'not-allowed';
		}
	});
}

function generateRandomKey() {
	const array = new Uint8Array(16);
	crypto.getRandomValues(array);
	const hexKey = Array.from(array).map(b => b.toString(16).padStart(2, '0')).join('');
	document.getElementById('network_key').value = hexKey;
}

function generateRandomExtPanId() {
	const array = new Uint8Array(8);
	crypto.getRandomValues(array);
	const hexExtPanId = Array.from(array).map(b => b.toString(16).padStart(2, '0')).join('');
	document.getElementById('extpanid').value = hexExtPanId;
}

// ========== TOPOLOGY ==========
let topologyData = null;

function refreshTopology() {
	console.log('Refreshing topology...');
	fetchAPI('/api/topology').then(data => {
		console.log('Topology data received:', data);
		if (data.error) {
			document.getElementById('topology-info').innerHTML = '<div class="error">❌ Error: ' + data.error + '</div>';
		} else {
			topologyData = data;
			drawTopology(data);
		}
	}).catch(error => {
		console.error('Error fetching topology:', error);
		document.getElementById('topology-info').innerHTML = '<div class="error">❌ Failed to load topology</div>';
	});
}

function drawTopology(data) {
	const canvas = document.getElementById('network-topology');
	if (!canvas) return;

	const rect = canvas.getBoundingClientRect();
	canvas.width = rect.width;
	canvas.height = rect.height;

	const ctx = canvas.getContext('2d');
	const width = canvas.width;
	const height = canvas.height;

	ctx.clearRect(0, 0, width, height);

	if (!data.nodes || data.nodes.length === 0) {
		ctx.fillStyle = '#999';
		ctx.font = '16px Arial';
		ctx.textAlign = 'center';
		ctx.fillText('No nodes in network', width/2, height/2);
		return;
	}

	const centerX = width / 2;
	const centerY = height / 2;
	const radius = Math.min(width, height) * 0.4;

	const positions = {};
	let leaderNode = null;
	let otherNodes = [];

	data.nodes.forEach(node => {
		if (node.shape === 'star') {
			leaderNode = node;
		} else {
			otherNodes.push(node);
		}
	});

	if (leaderNode) {
		positions[leaderNode.id] = { x: centerX, y: centerY, rloc16: leaderNode.rloc16 };
	}

	otherNodes.forEach((node, index) => {
		const angle = (2 * Math.PI * index) / otherNodes.length;
		positions[node.id] = {
			x: centerX + radius * Math.cos(angle),
			y: centerY + radius * Math.sin(angle),
			rloc16: node.rloc16
		};
	});

	ctx.strokeStyle = '#666';
	ctx.lineWidth = 2;
	data.edges.forEach(edge => {
		const from = positions[edge.from];
		const to = positions[edge.to];
		if (from && to) {
			ctx.beginPath();
			ctx.moveTo(from.x, from.y);
			ctx.lineTo(to.x, to.y);
			ctx.stroke();

			const angle = Math.atan2(to.y - from.y, to.x - from.x);
			const arrowLen = 10;
			ctx.beginPath();
			ctx.moveTo(to.x, to.y);
			ctx.lineTo(to.x - arrowLen * Math.cos(angle - Math.PI/6), to.y - arrowLen * Math.sin(angle - Math.PI/6));
			ctx.moveTo(to.x, to.y);
			ctx.lineTo(to.x - arrowLen * Math.cos(angle + Math.PI/6), to.y - arrowLen * Math.sin(angle + Math.PI/6));
			ctx.stroke();
		}
	});

	data.nodes.forEach(node => {
		const pos = positions[node.id];
		if (!pos) return;

		ctx.fillStyle = node.color;
		ctx.strokeStyle = '#333';
		ctx.lineWidth = 2;

		if (node.shape === 'star') {
			drawStar(ctx, pos.x, pos.y, 5, 12, 6);
		} else if (node.shape === 'diamond') {
			drawDiamond(ctx, pos.x, pos.y, 10);
		} else if (node.shape === 'dot') {
			ctx.beginPath();
			ctx.arc(pos.x, pos.y, 6, 0, 2 * Math.PI);
			ctx.fill();
			ctx.stroke();
		} else {
			ctx.fillRect(pos.x - 6, pos.y - 6, 12, 12);
			ctx.strokeRect(pos.x - 6, pos.y - 6, 12, 12);
		}
	});

	canvas.onclick = function(event) {
		const rect = canvas.getBoundingClientRect();
		const scaleX = canvas.width / rect.width;
		const scaleY = canvas.height / rect.height;
		const x = (event.clientX - rect.left) * scaleX;
		const y = (event.clientY - rect.top) * scaleY;

		for (const [id, pos] of Object.entries(positions)) {
			const dx = x - pos.x;
			const dy = y - pos.y;
			const distance = Math.sqrt(dx * dx + dy * dy);

			if (distance < 20) {
				showNodeInfo(pos.rloc16);
				break;
			}
		}
	};
}

function drawStar(ctx, cx, cy, spikes, outerRadius, innerRadius) {
	let rot = Math.PI / 2 * 3;
	let x = cx;
	let y = cy;
	const step = Math.PI / spikes;
	ctx.beginPath();
	ctx.moveTo(cx, cy - outerRadius);
	for (let i = 0; i < spikes; i++) {
		x = cx + Math.cos(rot) * outerRadius;
		y = cy + Math.sin(rot) * outerRadius;
		ctx.lineTo(x, y);
		rot += step;
		x = cx + Math.cos(rot) * innerRadius;
		y = cy + Math.sin(rot) * innerRadius;
		ctx.lineTo(x, y);
		rot += step;
	}
	ctx.lineTo(cx, cy - outerRadius);
	ctx.closePath();
	ctx.fill();
	ctx.stroke();
}

function drawDiamond(ctx, cx, cy, size) {
	ctx.beginPath();
	ctx.moveTo(cx, cy - size);
	ctx.lineTo(cx + size, cy);
	ctx.lineTo(cx, cy + size);
	ctx.lineTo(cx - size, cy);
	ctx.closePath();
	ctx.fill();
	ctx.stroke();
}

// ========== NODE INFO + DIAGNOSTICS ==========
function showNodeInfo(rloc16) {
	const params = new URLSearchParams();
	params.append('rloc16', rloc16);

	// Show loading
	document.getElementById('topology-info').innerHTML = '<div class="node-info-panel"><div class="loading">⏳ Loading node information...</div></div>';

	fetchAPI('/api/node/info', 'POST', params.toString()).then(data => {
		if (data.error) {
			document.getElementById('topology-info').innerHTML = '<div class="error">❌ Error: ' + data.error + '</div>';
		} else {
			displayNodeInfo(data);
		}
	});
}

function displayNodeInfo(data) {
	let html = '';

	html += '<div class="info-row"><div class="info-label">RLOC16:</div><div class="info-value">' + data.rloc16 + '</div></div>';
	html += '<div class="info-row"><div class="info-label">Role:</div><div class="info-value">' + data.role + '</div></div>';
	html += '<div class="info-row"><div class="info-label">Extended Address:</div><div class="info-value">' + data.ext_address + '</div></div>';

	if (data.age > 0) {
		html += '<div class="info-row"><div class="info-label">Link Quality In:</div><div class="info-value">' + data.link_quality_in + '</div></div>';
		html += '<div class="info-row"><div class="info-label">Age:</div><div class="info-value">' + data.age + ' seconds</div></div>';
	}

	html += '<div class="info-row"><div class="info-label">IPv6 Addresses:</div><div class="info-value">';
	data.ipv6_addresses.forEach(addr => {
		html += addr + '<br>';
	});
	html += '</div></div>';

	document.getElementById('topology-info').innerHTML = html;

	displayDiagnosticsControls(data.rloc16);
}

function displayDiagnosticsControls(rloc16) {
	let html = '';

	html += '<div class="diag-controls">';
	html += '<div class="diag-grid">';

	// Basic Information
	html += '<div class="diag-column">';
	html += '<div class="diag-category">Basic Information</div>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="0" checked><span>MAC Extended Address (0)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="1" checked><span>Address16/RLOC16 (1)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="2"><span>Mode (2)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="23"><span>EUI64 (23)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="24"><span>Version (24)</span></label>';
	html += '</div>';

	// Vendor Information
	html += '<div class="diag-column">';
	html += '<div class="diag-category">Vendor Information</div>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="25" checked><span>Vendor Name (25)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="26" checked><span>Vendor Model (26)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="27" checked><span>Vendor SW Version (27)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="28" checked><span>Thread Stack Version (28)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="35" checked><span>Vendor App URL (35)</span></label>';
	html += '</div>';

	// Network Data
	html += '<div class="diag-column">';
	html += '<div class="diag-category">Network Data</div>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="5"><span>Route64 (5)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="6"><span>Leader Data (6)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="7"><span>Network Data (7)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="8"><span>IPv6 Address List (8)</span></label>';
	html += '</div>';

	// Counters & Statistics
	html += '<div class="diag-column">';
	html += '<div class="diag-category">Counters & Statistics</div>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="9"><span>MAC Counters (9)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="14"><span>Battery Level (14)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="16"><span>Child Table (16)</span></label>';
	html += '<label class="diag-checkbox"><input type="checkbox" class="diag-tlv" value="34"><span>MLE Counters (34)</span></label>';
	html += '</div>';

	html += '</div>'; // diag-grid
	html += '</div>'; // diag-controls

	html += '<div style="margin-top: 10px; display: flex; gap: 5px; align-items: center;">';
	html += '<button class="button small" onclick="selectAllDiagnostics(true)">✓ Select All</button>';
	html += '<button class="button small" onclick="selectAllDiagnostics(false)">✗ Deselect All</button>';

	if (rloc16) {
		html += '<button class="button success small" onclick="requestDiagnostics(\'' + rloc16 + '\')">🔍 Get Diagnostics</button>';
	} else {
		html += '<button class="button success small" disabled style="opacity: 0.5; cursor: not-allowed;">🔍 Get Diagnostics</button>';
	}
	html += '</div>';

	document.getElementById('diagnostics-content').innerHTML = html;
}

function closeNodeInfo() {
	let html = '';
	html += '<div class="info-row"><div class="info-label">RLOC16:</div><div class="info-value" style="color: #bdc3c7;">-</div></div>';
	html += '<div class="info-row"><div class="info-label">Role:</div><div class="info-value" style="color: #bdc3c7;">-</div></div>';
	html += '<div class="info-row"><div class="info-label">Extended Address:</div><div class="info-value" style="color: #bdc3c7;">-</div></div>';
	html += '<div class="info-row"><div class="info-label">Link Quality In:</div><div class="info-value" style="color: #bdc3c7;">-</div></div>';
	html += '<div class="info-row"><div class="info-label">Age:</div><div class="info-value" style="color: #bdc3c7;">-</div></div>';
	html += '<div class="info-row"><div class="info-label">IPv6 Addresses:</div><div class="info-value" style="color: #bdc3c7;">-</div></div>';

	document.getElementById('topology-info').innerHTML = html;
	displayDiagnosticsControls(null);

}

function selectAllDiagnostics(select) {
	const checkboxes = document.querySelectorAll('.diag-tlv');
	checkboxes.forEach(cb => cb.checked = select);
}

function formatDiagnosticsData(data) {
	let html = '<div style="background: #f8f9fa; padding: 15px; border-radius: 5px; font-family: monospace; font-size: 12px;">';

	// === BASIC INFORMATION ===
	if ((data.ext_address && data.ext_address !== 'N/A') || (data.rloc16 && data.rloc16 !== 'N/A')) {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #3498db; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">📋 Basic Information</div>';
		if (data.ext_address && data.ext_address !== 'N/A') {
			html += '<div style="margin: 5px 0;"><strong>Extended Address:</strong> <span style="color: #16a085;">' + data.ext_address + '</span></div>';
		}
		if (data.rloc16 && data.rloc16 !== 'N/A') {
			html += '<div style="margin: 5px 0;"><strong>RLOC16:</strong> <span style="color: #16a085;">' + data.rloc16 + '</span></div>';
		}
		html += '</div>';
	}

	// === MODE ===
	if (data.mode && data.mode !== 'N/A') {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #9b59b6; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">⚙️ Device Mode</div>';
		html += '<div style="margin: 5px 0;">• <strong>RX On Idle:</strong> ' + (data.mode.rx_on_idle ? '✅ Yes' : '❌ No') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Device Type:</strong> ' + (data.mode.device_type ? data.mode.device_type.toUpperCase() : 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Network Data:</strong> ' + (data.mode.network_data || 'N/A') + '</div>';
		html += '</div>';
	}

	// === VENDOR INFORMATION ===
	const hasVendorInfo = (data.vendor_name && data.vendor_name !== 'N/A') ||
						  (data.vendor_model && data.vendor_model !== 'N/A') ||
						  (data.vendor_sw_version && data.vendor_sw_version !== 'N/A') ||
						  (data.thread_stack_version && data.thread_stack_version !== 'N/A') ||
						  (data.vendor_app_url && data.vendor_app_url !== 'N/A');

	if (hasVendorInfo) {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #e67e22; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">🏢 Vendor Information</div>';
		if (data.vendor_name && data.vendor_name !== 'N/A') html += '<div style="margin: 5px 0;">• <strong>Name:</strong> ' + data.vendor_name + '</div>';
		if (data.vendor_model && data.vendor_model !== 'N/A') html += '<div style="margin: 5px 0;">• <strong>Model:</strong> ' + data.vendor_model + '</div>';
		if (data.vendor_sw_version && data.vendor_sw_version !== 'N/A') html += '<div style="margin: 5px 0;">• <strong>SW Version:</strong> ' + data.vendor_sw_version + '</div>';
		if (data.thread_stack_version && data.thread_stack_version !== 'N/A') html += '<div style="margin: 5px 0;">• <strong>Thread Stack:</strong> ' + data.thread_stack_version + '</div>';
		if (data.vendor_app_url && data.vendor_app_url !== 'N/A') html += '<div style="margin: 5px 0;">• <strong>App URL:</strong> <a href="http://' + data.vendor_app_url + '" target="_blank" style="color: #3498db;">' + data.vendor_app_url + '</a></div>';
		html += '</div>';
	}

	// === LEADER DATA ===
	if (data.leader_data && data.leader_data !== 'N/A') {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #f39c12; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">👑 Leader Data</div>';
		html += '<div style="margin: 5px 0;">• <strong>Partition ID:</strong> ' + (data.leader_data.partition_id || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Weighting:</strong> ' + (data.leader_data.weighting || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Data Version:</strong> ' + (data.leader_data.data_version || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Stable Data Version:</strong> ' + (data.leader_data.stable_data_version || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Leader Router ID:</strong> ' + (data.leader_data.leader_router_id || 'N/A') + '</div>';
		html += '</div>';
	}

	// === ROUTE INFORMATION ===
	if (data.route && data.route !== 'N/A') {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #27ae60; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">🛣️ Route Information</div>';
		html += '<div style="margin: 5px 0;">• <strong>ID Sequence:</strong> ' + (data.route.id_sequence || 'N/A') + '</div>';
		if (data.route.route_data && data.route.route_data.length > 0) {
			html += '<div style="margin: 8px 0 5px 0;"><strong>Routes:</strong></div>';
			data.route.route_data.forEach((route, idx) => {
				html += '<div style="margin: 3px 0 3px 15px; font-size: 11px;">';
				html += '→ Router ' + (idx + 1) + ': ID=' + route.router_id +
						', LQI Out=' + route.link_quality_out +
						', LQI In=' + route.link_quality_in +
						', Cost=' + route.route_cost;
				html += '</div>';
			});
		}
		html += '</div>';
	}

	// === CONNECTIVITY ===
	if (data.connectivity && data.connectivity !== 'N/A') {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #1abc9c; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">🔗 Connectivity</div>';
		html += '<div style="margin: 5px 0;">• <strong>Parent Priority:</strong> ' + (data.connectivity.parent_priority || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Link Quality 3:</strong> ' + (data.connectivity.link_quality_3 || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Link Quality 2:</strong> ' + (data.connectivity.link_quality_2 || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Link Quality 1:</strong> ' + (data.connectivity.link_quality_1 || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Leader Cost:</strong> ' + (data.connectivity.leader_cost || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>ID Sequence:</strong> ' + (data.connectivity.id_sequence || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Active Routers:</strong> ' + (data.connectivity.active_routers || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>SED Buffer Size:</strong> ' + (data.connectivity.sed_buffer_size || 'N/A') + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>SED Datagram Count:</strong> ' + (data.connectivity.sed_datagram_count || 'N/A') + '</div>';
		html += '</div>';
	}

	// === BATTERY LEVEL ===
	if (data.battery_level && data.battery_level !== 'N/A') {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #f1c40f; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">🔋 Battery Level</div>';
		html += '<div style="margin: 5px 0;">• <strong>Level:</strong> ' + data.battery_level + '%</div>';
		html += '</div>';
	}

	// === CHILD TABLE ===
	if (data.child_table && data.child_table !== 'N/A') {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #e91e63; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">👶 Child Table</div>';
		if (Array.isArray(data.child_table) && data.child_table.length > 0) {
			data.child_table.forEach((child, idx) => {
				html += '<div style="margin: 5px 0; padding: 5px; background: #f8f9fa; border-radius: 3px;">';
				html += '<strong>Child ' + (idx + 1) + ':</strong><br>';
				html += '• Timeout: ' + child.timeout + '<br>';
				html += '• Link Quality: ' + child.link_quality + '<br>';
				html += '• Child ID: ' + child.child_id + '<br>';
				html += '• RX On Idle: ' + (child.rx_on_idle ? 'Yes' : 'No') + '<br>';
				html += '• FTD: ' + (child.ftd ? 'Yes' : 'No');
				html += '</div>';
			});
		}
		html += '</div>';
	}

	// === IPv6 ADDRESSES ===
	if (data.ipv6_addresses && data.ipv6_addresses !== 'N/A' && Array.isArray(data.ipv6_addresses) && data.ipv6_addresses.length > 0) {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #3498db; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">🌐 IPv6 Addresses</div>';
		data.ipv6_addresses.forEach(addr => {
			html += '<div style="margin: 3px 0; font-size: 11px; color: #16a085;">• ' + addr + '</div>';
		});
		html += '</div>';
	}

	// === MAC COUNTERS ===
	if (data.mac_counters && data.mac_counters !== 'N/A') {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #e74c3c; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">📊 MAC Counters</div>';
		html += '<div style="margin: 5px 0;">• <strong>In Errors:</strong> ' + (data.mac_counters.if_in_errors || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Out Errors:</strong> ' + (data.mac_counters.if_out_errors || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>In Unicast Packets:</strong> ' + (data.mac_counters.if_in_ucast_pkts || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>In Broadcast Packets:</strong> ' + (data.mac_counters.if_in_broadcast_pkts || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Out Unicast Packets:</strong> ' + (data.mac_counters.if_out_ucast_pkts || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Out Broadcast Packets:</strong> ' + (data.mac_counters.if_out_broadcast_pkts || 0) + '</div>';
		html += '</div>';
	}

	// === MLE COUNTERS ===
	if (data.mle_counters && data.mle_counters !== 'N/A') {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #8e44ad; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">📈 MLE Counters</div>';
		html += '<div style="margin: 5px 0;">• <strong>Disabled Role:</strong> ' + (data.mle_counters.disabled_role || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Detached Role:</strong> ' + (data.mle_counters.detached_role || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Child Role:</strong> ' + (data.mle_counters.child_role || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Router Role:</strong> ' + (data.mle_counters.router_role || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Leader Role:</strong> ' + (data.mle_counters.leader_role || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Attach Attempts:</strong> ' + (data.mle_counters.attach_attempts || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Partition ID Changes:</strong> ' + (data.mle_counters.partition_id_changes || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Better Partition Attach Attempts:</strong> ' + (data.mle_counters.better_partition_attach_attempts || 0) + '</div>';
		html += '<div style="margin: 5px 0;">• <strong>Parent Changes:</strong> ' + (data.mle_counters.parent_changes || 0) + '</div>';
		html += '</div>';
	}

	// === NETWORK DATA ===
	if (data.network_data && data.network_data !== 'N/A') {
		html += '<div style="background: #fff; padding: 10px; margin-bottom: 10px; border-left: 4px solid #95a5a6; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #2c3e50; margin-bottom: 8px; font-size: 13px;">🔐 Network Data (hex)</div>';
		html += '<div style="margin: 5px 0; word-break: break-all; color: #7f8c8d; font-size: 10px; max-height: 100px; overflow-y: auto; padding: 5px; background: #ecf0f1; border-radius: 3px;">' + data.network_data + '</div>';
		html += '</div>';
	}

	// === UNSUPPORTED TLVs ===
	const unsupportedTLVs = Object.keys(data).filter(key => key.startsWith('tlv_') && data[key] === 'unsupported');
	if (unsupportedTLVs.length > 0) {
		html += '<div style="background: #fff; padding: 10px; border-left: 4px solid #bdc3c7; border-radius: 3px;">';
		html += '<div style="font-weight: bold; color: #7f8c8d; margin-bottom: 8px; font-size: 13px;">⚠️ Unsupported TLVs</div>';
		unsupportedTLVs.forEach(key => {
			html += '<div style="margin: 3px 0; color: #95a5a6; font-size: 11px;">• TLV ' + key.replace('tlv_', '') + '</div>';
		});
		html += '</div>';
	}

	html += '</div>';
	return html;
}

function requestDiagnostics(rloc16) {
	const diagnosticsContent = document.getElementById('diagnostics-content');
	const diagnosticsResult = document.getElementById('diagnostics-result');

	diagnosticsContent.style.display = 'block';
	diagnosticsResult.style.display = 'block';

	const checkboxes = document.querySelectorAll('.diag-tlv:checked');
	const tlvs = Array.from(checkboxes).map(cb => cb.value).join(',');

	if (tlvs.length === 0) {
		diagnosticsResult.innerHTML = '<div class="error" style="margin-top: 10px;">❌ Please select at least one diagnostic type</div>';
		return;
	}

	const params = new URLSearchParams();
	params.append('rloc16', rloc16);
	params.append('tlvs', tlvs);

	diagnosticsResult.innerHTML = '<div class="loading" style="margin-top: 10px;">⏳ Requesting diagnostics... Please wait (up to 5 seconds)</div>';

	fetchAPI('/api/node/diagnostics', 'POST', params.toString()).then(data => {
		if (data.error) {
			diagnosticsResult.innerHTML = '<div class="error">❌ Error: ' + data.error + '</div>';
		} else if (data.data) {
			showStatus('✅ Diagnostics Results', 'success');
			let html = formatDiagnosticsData(data.data);
			diagnosticsResult.innerHTML = html;
		} else {
			diagnosticsResult.innerHTML = '<div class="success-msg">✅ ' + data.message + '</div>';
		}
	});
}

function showStatus(message, type = 'success', duration = 3000) {
	const statusBar = document.getElementById('status-bar');
	statusBar.className = 'status-bar ' + type;
	statusBar.innerHTML = message;

	if (duration > 0 && (type === 'error' || type === 'loading')) {
		setTimeout(() => {
			statusBar.className = 'status-bar';
			statusBar.innerHTML = '';
		}, duration);
	} else if (type === 'success' && duration > 0) {
		setTimeout(() => {
			statusBar.className = 'status-bar';
			statusBar.innerHTML = '';
		}, duration);
	}
}

// ========== INITIALIZATION ==========
window.onload = function() {
	console.log('🚀 OpenThread Border Router Web Interface loaded');
	closeNodeInfo();
	refreshTopology();
	updateDatasetButtonState();

	// Auto-refresh topology every 5 seconds
	setInterval(refreshTopology, 5000);
};
