#!/bin/sh -eu

start-stop-daemon --remove-pidfile --pidfile log/leshan.pid --stop
