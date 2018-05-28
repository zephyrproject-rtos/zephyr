FROM ubuntu:17.10

ENV ZEPHYR_BASE=/zephyr
ENV ZEPHYR_GCC_VARIANT=zephyr
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk

# tools pre-requisites
RUN apt-get update \
    && apt-get install -yq --no-install-recommends git cmake ninja-build gperf \
               ccache doxygen dfu-util device-tree-compiler \
               python3-ply python3-pip python3-setuptools xz-utils file \
               wget make vim-tiny \
    && rm -rf /var/lib/apt/lists/*

# Zephyr SDK
ENV ZEPHYR_SDK_VERSION=0.9.2
RUN wget -O /tmp/zephyr-setup https://github.com/zephyrproject-rtos/meta-zephyr-sdk/releases/download/${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}-setup.run \
    && chmod 755 /tmp/zephyr-setup \
    && /tmp/zephyr-setup --quiet \
    && rm /tmp/zephyr-setup

# Zephyr Project repo and Python pre-requisites
RUN git clone --depth 1 https://github.com/zephyrproject-rtos/zephyr.git $ZEPHYR_BASE \
    && cd $ZEPHYR_BASE \
    && pip3 install --user -r scripts/requirements.txt

WORKDIR $ZEPHYR_BASE

# make sure we've run the zephyr env setter
RUN echo "source $ZEPHYR_BASE/zephyr-env.sh" >> /etc/bash.bashrc
