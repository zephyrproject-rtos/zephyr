#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_ZEPHYR_BASE="$(realpath "${SCRIPT_DIR}/../../../..")"
ZEPHYR_BASE="${ZEPHYR_BASE:-${DEFAULT_ZEPHYR_BASE}}"
BOARD="nrf54l15dk/nrf54l15/cpuapp"
MICRO_CORE_ROOT="${MICRO_CORE_ROOT:-}"

# The labels on the probes include leading zeroes. nrfutil reports the same
# identifiers without them, which is the form used by west's nrfutil runner.
SENDER_DEVICE_ID="${SENDER_DEVICE_ID:-1057753351}"
RECEIVER_DEVICE_ID="${RECEIVER_DEVICE_ID:-1057744552}"

usage()
{
	echo "Usage: $0 sender|receiver"
}

if [ "$#" -ne 1 ]; then
	usage
	exit 2
fi

case "$1" in
sender)
	BUILD_DIR="${SCRIPT_DIR}/build_sender"
	CONF_FILE="${SCRIPT_DIR}/prj_sender.conf"
	DEVICE_ID="${SENDER_DEVICE_ID}"
	;;
receiver)
	BUILD_DIR="${SCRIPT_DIR}/build_receiver"
	CONF_FILE="${SCRIPT_DIR}/prj_receiver.conf"
	DEVICE_ID="${RECEIVER_DEVICE_ID}"
	;;
*)
	usage
	exit 2
	;;
esac

command -v west >/dev/null 2>&1 || {
	echo "west is not available. Open an nRF Connect SDK terminal first." >&2
	exit 1
}

if [ ! -f "${MICRO_CORE_ROOT}/src/io/communication/mc_communication.c" ]; then
	echo "Set MICRO_CORE_ROOT to the micro_core source directory." >&2
	exit 1
fi

west -z "${ZEPHYR_BASE}" build \
	-b "${BOARD}" \
	"${SCRIPT_DIR}" \
	-d "${BUILD_DIR}" \
	--pristine=always \
	-- -DCONF_FILE="${CONF_FILE}" \
	-DMICRO_CORE_ROOT="${MICRO_CORE_ROOT}"

west -z "${ZEPHYR_BASE}" flash \
	-d "${BUILD_DIR}" \
	--dev-id "${DEVICE_ID}"

echo "$1 build successfully flashed to ${DEVICE_ID}"
