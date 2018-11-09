FROM ubuntu:18.04

ARG ZSDK_VERSION=0.9.5
ARG GCC_ARM_NAME=gcc-arm-none-eabi-7-2018-q2-update

RUN apt-get -y update && \
	apt-get -y upgrade && \
	apt-get install --no-install-recommends -y \
	git \
	git-core \
	build-essential \
	cmake \
	ninja-build \
	gperf \
	ccache \
	doxygen \
	dfu-util \
	device-tree-compiler \
	python3-ply \
	python3-pip \
	python3-setuptools \
	xz-utils \
	file \
	make \
	g++ \
	gcc \
	gcc-multilib \
	texinfo \
	wget \
	sudo \
	libglib2.0-dev \
	pkg-config \
	libpcap-dev \
	autoconf \
	automake \
	libtool \
	socat \
	iproute2 \
	net-tools \
 	gcovr \
	valgrind \
        ninja-build \
        lcov \
	qemu \
	locales && \
	rm -rf /var/lib/apt/lists/*

RUN wget -q https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/master/scripts/requirements.txt && \
	pip3 install wheel &&\
 	pip3 install -r requirements.txt && \
	pip3 install sh

RUN wget -q "https://github.com/zephyrproject-rtos/meta-zephyr-sdk/releases/download/${ZSDK_VERSION}/zephyr-sdk-${ZSDK_VERSION}-setup.run" && \
  sh "zephyr-sdk-${ZSDK_VERSION}-setup.run" --quiet -- -d /opt/toolchains/zephyr-sdk-${ZSDK_VERSION} && \
  rm "zephyr-sdk-${ZSDK_VERSION}-setup.run"

RUN wget -q https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/${GCC_ARM_NAME}-linux.tar.bz2  && \
    tar xf ${GCC_ARM_NAME}-linux.tar.bz2 && \
    rm -f ${GCC_ARM_NAME}-linux.tar.bz2 && \
    mv ${GCC_ARM_NAME} /opt/toolchains/${GCC_ARM_NAME}

RUN useradd -m -G plugdev user \
 && echo 'user ALL = NOPASSWD: ALL' > /etc/sudoers.d/user \
 && chmod 0440 /etc/sudoers.d/user

# Set the locale
RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8
ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/toolchains/zephyr-sdk-${ZSDK_VERSION}
ENV ZEPHYR_BASE=/workdir
ENV GNUARMEMB_TOOLCHAIN_PATH=/opt/toolchains/${GCC_ARM_NAME}

RUN chown -R user:user /home/user


CMD ["/bin/bash"]
USER user
WORKDIR /workdir
VOLUME ["/workdir"]


