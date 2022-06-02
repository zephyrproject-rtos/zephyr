#!/bin/sh -eu

# Check if we have Zephyr network set up
if ! ip link show zeth >/dev/null 2>&1; then
        echo "zeth network not set up"
        echo "run sudo ./net-setup.sh start from tools/net-tools first"
        exit 1
fi

# Download Leshan if needed
if [ ! -f leshan-server-demo.jar ]; then
        wget https://ci.eclipse.org/leshan/job/leshan/lastSuccessfulBuild/artifact/leshan-server-demo.jar
fi

mkdir -p log

start-stop-daemon --make-pidfile --pidfile log/leshan.pid --chdir $(pwd) --background --start \
        --startas /bin/bash -- -c "exec java -jar ./leshan-server-demo.jar -wp 8080 -vv >log/leshan.log 2>&1"
