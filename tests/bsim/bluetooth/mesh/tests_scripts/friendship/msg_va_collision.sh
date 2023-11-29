#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that LPN sends only one Subscription List Add and only one Subscription List Remove message
# to Friend when LPN is subscribed to 2 virtual addresses with collision.
# Test scenario:
# 1. LPN subscribes a model to 2 virtual addresses with collision and sends single Friend
# Subscription List Add message to its friend.
# 2. Step 1:
# 2.1. Friend sends a message to each virtual address, LPN receives both messages.
# 3. Step 2:
# 3.1. LPN unsubscribes from one of the virtual addresses. At this step no Friend Subscription
# Remove messages are sent from LPN to its friend.
# 3.2. Friend sends a message to each virtual address. LPN receives both, but successfully decrypts
# only one message, which it is still subscribed to.
# 4. Step 3:
# 4.1. LPN unsubscribes from the second virtual address and sends Friend Subscription Remove message
# to its friend.
# 4.2. Friend sends a message to each virtual address, but non of them are received by LPN.
RunTest mesh_friendship_msg_va_collision \
	friendship_lpn_va_collision \
	friendship_friend_va_collision

overlay=overlay_psa_conf
RunTest mesh_friendship_msg_va_collision_psa \
	friendship_lpn_va_collision \
	friendship_friend_va_collision
