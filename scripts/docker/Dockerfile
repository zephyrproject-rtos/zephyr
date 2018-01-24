FROM ubuntu:16.04
ARG no_proxy
ARG http_proxy
ARG https_proxy
ENV no_proxy ${no_proxy}
ENV http_proxy ${http_proxy}
ENV https_proxy ${https_proxy}

ARG CMAKE_URL
ENV CMAKE_URL ${CMAKE_URL:-https://cmake.org/files/v3.8/cmake-3.8.2-Linux-x86_64.sh}
ARG ZSDK_VERSION
ENV ZSDK_VERSION ${ZSDK_VERSION:-0.9.2}

RUN apt-get -y update && \
  apt-get -y upgrade && \
  apt-get install --no-install-recommends -y git cmake ninja-build gperf \
    ccache doxygen dfu-util device-tree-compiler \
    python3-ply python3-pip python3-setuptools xz-utils \
    file make gcc-multilib wget sudo libglib2.0-dev pkg-config libpcap-dev \
    autoconf automake libtool socat iproute2 net-tools && \
  wget "https://github.com/zephyrproject-rtos/meta-zephyr-sdk/releases/download/${ZSDK_VERSION}/zephyr-sdk-${ZSDK_VERSION}-setup.run" && \
  sh "zephyr-sdk-${ZSDK_VERSION}-setup.run" && \
  rm "zephyr-sdk-${ZSDK_VERSION}-setup.run" && \
  useradd -m user && \
  echo 'user   ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

COPY . /home/user/zephyr

RUN chown -R user:user /home/user

USER user
WORKDIR /home/user

RUN mkdir $HOME/cmake && cd $HOME/cmake && \
  wget -O cmake.sh "$CMAKE_URL" && \
  yes | sh cmake.sh | cat && \
  rm cmake.sh && \
  export cmake="$(ls)" && \
  echo "export PATH=$PWD/${cmake}/bin:\$PATH" >> \
    $HOME/.zephyrrc && \
  pip3 install --user wheel && \
  pip3 install --user -r $HOME/zephyr/scripts/requirements.txt && \
  git clone https://github.com/zephyrproject-rtos/net-tools $HOME/net-tools && \
  cd $HOME/net-tools && \
  make tunslip6 && \
  echo 'export ZEPHYR_BASE=$HOME/zephyr' | \
    tee -a $HOME/.zephyrrc && \
  echo 'export ZEPHYR_GCC_VARIANT=zephyr' | \
    tee -a $HOME/.zephyrrc && \
  echo 'export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk' | \
    tee -a $HOME/.zephyrrc && \
  echo 'source $HOME/zephyr/zephyr-env.sh' >> $HOME/.bashrc.new && \
  cat $HOME/.bashrc >> $HOME/.bashrc.new && \
  mv $HOME/.bashrc.new $HOME/.bashrc && \
  sudo mv $HOME/zephyr/scripts/docker/entrypoint.sh /entrypoint.sh

# To build behind a proxy
# docker build -f scripts/docker/Dockerfile --build-arg no_proxy=$no_proxy --build-arg http_proxy=$http_proxy --build-arg https_proxy=$https_proxy -t z .
# docker build -f scripts/docker/Dockerfile -t z .
# docker run --rm -ti --device=/dev/net/tun --cap-add=NET_ADMIN --net=host z
ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]
