
# sample build:
#
# docker run --rm -it -w /arm svendowideit/zephyr-esp32 sh -c "cmake -DBOARD=esp32 /zephyr/samples/hello_world && make flash"

FROM svendowideit/zephyr

RUN apt-get update \
    && apt-get install -yq git wget make libncurses-dev flex bison gperf python python-serial \
    && rm -rf /var/lib/apt/lists/*

ENV ZEPHYR_GCC_VARIANT="espressif"
ENV ESP_IDF_PATH="/opt/esp-idf"
ENV IDF_PATH="/opt/esp-idf"
ENV ESPRESSIF_TOOLCHAIN_PATH="/opt/xtensa-esp32-elf/"

# ESP-IDF SDK
RUN git clone --recursive https://github.com/espressif/esp-idf.git $ESP_IDF_PATH \
    && cd $ESP_IDF_PATH \
    git checkout dc8c33892e0

# xtensa esp32 toolchain
RUN wget -O /tmp/xtensa-esp32-linux64.tgz https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-73-ge28a011-5.2.0.tar.gz
RUN mkdir -p $ESPRESSIF_TOOLCHAIN_PATH \
    && cd $ESPRESSIF_TOOLCHAIN_PATH \
    && tar --strip-components=1 -zxvf /tmp/xtensa-esp32-linux64.tgz \
    && echo "export PATH=$PATH:${ESPRESSIF_TOOLCHAIN_PATH}bin" >> /etc/bash.bashrc

