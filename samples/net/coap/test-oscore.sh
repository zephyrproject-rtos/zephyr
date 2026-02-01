#!/bin/bash
# Test script for OSCORE-enabled CoAP server
# Requires libcoap with OSCORE support

set -e

SCRIPT_DIR=$(dirname "$0")
# Server address (adjust as needed)
# Use IPv6 by default, or set SERVER_ADDR environment variable
SERVER_ADDR="${SERVER_ADDR:-[2001:db8::1]}"
# For IPv4, use: SERVER_ADDR="192.0.2.1"
SERVER_PORT="5683"
OSCORE_CONF="$SCRIPT_DIR/oscore.conf"
# Use sequence file to maintain replay protection state across requests
OSCORE_SEQ="/tmp/oscore_seq_$(date +%s).dat"
COAP_CLIENT="coap-client-openssl"
VERBOSE="-v 4"

# Trap to ensure cleanup on exit/interrupt
trap 'rm -f "$OSCORE_SEQ"' EXIT INT TERM

# Check if coap-client is available
if ! command -v $COAP_CLIENT &> /dev/null; then
    echo "Error: $COAP_CLIENT not found. Please install libcoap with OSCORE support."
    exit 1
fi

# Check if OSCORE config exists
if [ ! -f "$OSCORE_CONF" ]; then
    echo "Error: OSCORE configuration file '$OSCORE_CONF' not found."
    echo "Please create it with the appropriate OSCORE parameters."
    exit 1
fi

echo "========================================="
echo "Testing OSCORE-enabled CoAP Server"
echo "========================================="
echo ""

# Test 1: GET /test
echo "Test 1: GET /test (OSCORE-protected)"
$COAP_CLIENT -m get "coap://${SERVER_ADDR}:${SERVER_PORT}/test" \
    -E "$OSCORE_CONF,$OSCORE_SEQ" $VERBOSE
echo ""

# Test 2: POST /test
echo "Test 2: POST /test (OSCORE-protected)"
$COAP_CLIENT -m post "coap://${SERVER_ADDR}:${SERVER_PORT}/test" \
    -e "OSCORE test payload" -E "$OSCORE_CONF,$OSCORE_SEQ" $VERBOSE
echo ""

# Test 3: GET /large (block-wise transfer)
echo "Test 3: GET /large (OSCORE-protected, block-wise)"
$COAP_CLIENT -m get "coap://${SERVER_ADDR}:${SERVER_PORT}/large" \
    -E "$OSCORE_CONF,$OSCORE_SEQ" $VERBOSE
echo ""

# Test 4: GET /core (well-known resources)
echo "Test 4: GET /.well-known/core (OSCORE-protected)"
$COAP_CLIENT -m get "coap://${SERVER_ADDR}:${SERVER_PORT}/.well-known/core" \
    -E "$OSCORE_CONF,$OSCORE_SEQ" $VERBOSE
echo ""

# Test 5: Unprotected request (should work if require_oscore is false)
echo "Test 5: GET /test (unprotected, should work if server allows)"
$COAP_CLIENT -m get "coap://${SERVER_ADDR}:${SERVER_PORT}/test" $VERBOSE || \
    echo "Note: Unprotected request rejected (expected if require_oscore=true)"
echo ""

echo "========================================="
echo "All tests completed"
echo "========================================="
