FROM ghcr.io/zephyrproject-rtos/ci:v0.26.13

# Install gdbserver for enabling GUI debugging in VSCode
RUN apt-get -y update && \
	apt-get -y upgrade && \
	apt-get install --no-install-recommends -y \
	gdbserver

# See .github/workflows/compliance.yml *cries in requirements.txt*
RUN pip3 install python-magic lxml junitparser gitlint pylint pykwalify yamllint clang-format unidiff

RUN usermod -s /usr/bin/bash user

USER user

CMD /usr/bin/bash
