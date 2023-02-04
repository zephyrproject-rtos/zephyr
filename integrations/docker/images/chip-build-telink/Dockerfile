ARG VERSION=latest
FROM connectedhomeip/chip-build:${VERSION} as build

RUN set -x \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -fy --no-install-recommends \
    wget \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/ \
    && : # last line

# Setup toolchain
RUN set -x \
    && mkdir /opt/telink \
    && cd /opt/telink \
    && wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.15.2/zephyr-sdk-0.15.2_linux-x86_64.tar.gz \
    && wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.15.2/sha256.sum | shasum --check --ignore-missing \
    && tar xvf zephyr-sdk-0.15.2_linux-x86_64.tar.gz \
    && rm -rf zephyr-sdk-0.15.2_linux-x86_64.tar.gz \
    && cd zephyr-sdk-0.15.2 \
    && ./setup.sh -t riscv64-zephyr-elf -h -c \
    && cd / \
    && : # last line

# Setup Zephyr
ARG ZEPHYR_REVISION=3da02564e056033ef184081b0ca15e8e00e40bc9
WORKDIR /opt/telink/zephyrproject
RUN set -x \
    && python3 -m pip install -U --no-cache-dir \
    west==0.12.0 \
    imgtool==1.7.0 \
    && git clone https://github.com/telink-semi/zephyr \
    && cd zephyr \
    && git reset ${ZEPHYR_REVISION} --hard \
    && west init -l \
    && cd .. \
    && west update -o=--depth=1 -n -f smart \
    && west blobs fetch hal_telink \
    && west zephyr-export \
    && : # last line

FROM connectedhomeip/chip-build:${VERSION}

COPY --from=build /opt/telink/zephyr-sdk-0.15.2/ /opt/telink/zephyr-sdk-0.15.2/
COPY --from=build /opt/telink/zephyrproject/ /opt/telink/zephyrproject/

ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/telink/zephyr-sdk-0.15.2

RUN set -x \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -fy --no-install-recommends \
    ccache=3.7.7-1 \
    dfu-util=0.9-1 \
    device-tree-compiler=1.5.1-1 \
    gcc-multilib=4:9.3.0-1ubuntu2 \
    g++-multilib=4:9.3.0-1ubuntu2 \
    libsdl2-dev=2.0.10+dfsg1-3 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/ \
    && python3 -m pip install -U --no-cache-dir \
    pyelftools==0.27 \
    && pip3 install --no-cache-dir --user -r /opt/telink/zephyrproject/zephyr/scripts/requirements.txt \
    && : # last line

# Setup Telink tools required for "west flash"
ARG TELINK_TOOLS_BASE=/opt/telink/tools
RUN wget http://wiki.telink-semi.cn/tools_and_sdk/Tools/IDE/telink_riscv_ice_flash_tool.zip -O /opt/telink/tools.zip \
    && unzip /opt/telink/tools.zip -d ${TELINK_TOOLS_BASE} \
    && rm /opt/telink/tools.zip \
    && mv ${TELINK_TOOLS_BASE}/telink_riscv_linux_toolchain/* ${TELINK_TOOLS_BASE} \
    && rm -rf ${TELINK_TOOLS_BASE}/telink_riscv_linux_toolchain \
    && chmod +x ${TELINK_TOOLS_BASE}/flash/bin/SPI_burn \
    && chmod +x ${TELINK_TOOLS_BASE}/ice/ICEman \
    && : # last line

# Add path to Telink tools
ENV PATH="${TELINK_TOOLS_BASE}/flash/bin:${PATH}"
ENV PATH="${TELINK_TOOLS_BASE}/ice:${PATH}"

ARG ZEPHYR_PROJECT_DIR=/opt/telink/zephyrproject
ENV TELINK_ZEPHYR_BASE=${ZEPHYR_PROJECT_DIR}/zephyr
ENV ZEPHYR_BASE=${ZEPHYR_PROJECT_DIR}/zephyr
