#!/usr/bin/env bash
# Copyright (c) 2025, Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# This script installs Wi-Fi certificates to a device using the `device_credentials_installer` tool.
#
# shellcheck disable=SC2086,SC2154
# Usage:
#   ./install_certs.sh <path_to_certs> [port] [mode]
#
# Arguments:
#   <path_to_certs>  Path to the directory containing the certificate files.
#   [port]           (Optional) Serial port to use for communication with the device. Default is
#                    /dev/ttyACM1.
#   [mode]           (Optional) Mode of operation: AP or STA. Default is STA.
#
# Dependencies:
#   - `device_credentials_installer` tool must be installed. You can install it using:
#     pip3 install nrfcloud-utils
#
# Certificate Files:
#   The script expects the following certificate files in the specified directory:
#   - ca.pem: CA certificate for Wi-Fi Enterprise mode (Phase 1)
#   - client-key.pem: Client private key for Wi-Fi Enterprise mode (Phase 1)
#   - server-key.pem: Server private key for Wi-Fi Enterprise mode, for used in AP mode.
#   - client.pem: Client certificate for Wi-Fi Enterprise mode (Phase 1)
#   - server.pem: Server certificate for Wi-Fi Enterprise mode, for used in AP mode.
#   - ca2.pem: CA certificate for Wi-Fi Enterprise mode (Phase 2)
#   - client-key2.pem: Client private key for Wi-Fi Enterprise mode (Phase 2)
#   - client2.pem: Client certificate for Wi-Fi Enterprise mode (Phase 2)
#
# Each certificate file is associated with a specific security tag (sec_tag) which is used during
# installation.
#
# The script performs the following steps:
#   1. Checks if the required arguments are provided.
#   2. Validates the existence of the certificate directory.
#   3. Checks if the `device_credentials_installer` tool is available.
#   4. Iterates over the expected certificate files and installs them to the device if they exist.
#   5. Logs the success or failure of each certificate installation.
#
# Note:
#   - If a certificate file is missing, the script will skip its installation and log a warning.
#   - The script will terminate on the first encountered error (set -e).
set -e

if [ -z "$1" ]; then
  echo -e "\033[31mError: Usage: $0 <path_to_certs> [port] [mode]\033[0m"
  exit 1
fi

CERT_PATH=$1
PORT=${2:-/dev/ttyACM1}  # Default port is /dev/ttyACM1 if not provided
MODE=${3:-STA}  # Default mode is STA if not provided

if [ ! -d "$CERT_PATH" ]; then
  echo -e "\033[31mError: Directory $CERT_PATH does not exist.\033[0m"
  exit 1
fi

echo -e "\033[33mWarning: Please make sure that the UART is not being used by another" \
  " application.\033[0m"

read -r -p "Press Enter to continue or Ctrl+C to exit..."

if ! command -v device_credentials_installer &> /dev/null; then
  echo -e "\033[31mError: device_credentials_installer could not be found.\033[0m"
  echo "Please install it using: pip3 install nrfcloud-utils"
  exit 1
fi

INSTALLED_VERSION=$(pip3 show nrfcloud-utils | grep Version | awk '{print $2}')
REQUIRED_VERSION="1.0.4"

if [ "$(printf '%s\n' "$REQUIRED_VERSION" "$INSTALLED_VERSION" | sort -V | head -n1)" != \
"$REQUIRED_VERSION" ]; then
  echo -e "\033[31mError: device_credentials_installer >= $REQUIRED_VERSION required. Installed: \
$INSTALLED_VERSION.\033[0m"
  echo "Update: pip3 install --upgrade nrfcloud-utils"
  exit 1
fi

# From zephyr/subsys/net/lib/tls_credentials/tls_credentials_shell.c
TLS_CREDENTIAL_CA_CERTIFICATE=0 # CA
TLS_CREDENTIAL_PUBLIC_CERTIFICATE=1 # SERV
TLS_CREDENTIAL_PRIVATE_KEY=2 # PK


WIFI_CERT_SEC_TAG_BASE=0x1020001
declare -A WIFI_CERT_SEC_TAG_MAP=(
  ["ca.pem"]="{\"$TLS_CREDENTIAL_CA_CERTIFICATE\" $((WIFI_CERT_SEC_TAG_BASE))}"
  ["client-key.pem"]="{\"$TLS_CREDENTIAL_PRIVATE_KEY\" $((WIFI_CERT_SEC_TAG_BASE + 1))}"
  ["server-key.pem"]="{\"$TLS_CREDENTIAL_PRIVATE_KEY\" $((WIFI_CERT_SEC_TAG_BASE + 2))}"
  ["client.pem"]="{\"$TLS_CREDENTIAL_PUBLIC_CERTIFICATE\" $((WIFI_CERT_SEC_TAG_BASE + 3))}"
  ["server.pem"]="{\"$TLS_CREDENTIAL_PUBLIC_CERTIFICATE\" $((WIFI_CERT_SEC_TAG_BASE + 4))}"
  ["ca2.pem"]="{\"$TLS_CREDENTIAL_CA_CERTIFICATE\" $((WIFI_CERT_SEC_TAG_BASE + 5))}"
  ["client-key2.pem"]="{\"$TLS_CREDENTIAL_PRIVATE_KEY\" $((WIFI_CERT_SEC_TAG_BASE + 6))}"
  ["client2.pem"]="{\"$TLS_CREDENTIAL_PUBLIC_CERTIFICATE\" $((WIFI_CERT_SEC_TAG_BASE + 7))}"
)

# Select certificates based on mode
if [ "$MODE" == "AP" ]; then
  CERT_FILES=("ca.pem" "server-key.pem" "server.pem")
else
  CERT_FILES=("ca.pem" "client-key.pem" "client.pem" "ca2.pem" "client-key2.pem" "client2.pem")
fi

total_certs=${#CERT_FILES[@]}
processed_certs=0

for cert in "${CERT_FILES[@]}"; do
  processed_certs=$((processed_certs + 1))
  echo "Processing certificate $processed_certs of $total_certs: $cert"

  if [ ! -f "$CERT_PATH/$cert" ]; then
  echo -e "\033[31mWarning: Certificate file $CERT_PATH/$cert does not exist. Skipping...\033[0m"
  continue
  fi

  cert_info=${WIFI_CERT_SEC_TAG_MAP[$cert]}
  cert_type=$(echo "$cert_info" | awk -F'[{} ]' '{print $2}' | tr -d '"')
  cert_type_int=$((10#$cert_type))
  sec_tag=$(echo "$cert_info" | awk -F'[{} ]' '{print $3}' | tr -d '"')
  sec_tag_int=$((10#$sec_tag))
  if device_credentials_installer --local-cert-file "$CERT_PATH/$cert" \
  --cmd-type tls_cred_shell --delete \
  --port $PORT -S $sec_tag_int --cert-type $cert_type_int; then
  echo "Successfully installed $cert."
  else
  echo -e "\033[31mFailed to install $cert.\033[0m"
  fi
done


echo "Certificate installation process completed."
