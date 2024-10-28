# Copyright 2022 Meta
# SPDX-License-Identifier: Apache-2.0

.PHONY: all clean

OS = $(shell uname -s)

CXXFLAGS :=
CXXFLAGS += -std=c++17

GEN_DIR = gen-cpp
GENSRC = $(GEN_DIR)/Hello.cpp $(GEN_DIR)/Hello.h $(GEN_DIR)/hello_types.h
GENHDR = $(filter %.h, $(GENSRC))
GENOBJ = $(filter-out %.h, $(GENSRC:.cpp=.o))

THRIFT_FLAGS :=
THRIFT_FLAGS += $(shell pkg-config --cflags thrift)
THRIFT_FLAGS += -I$(GEN_DIR)
ifeq ($(OS),Darwin)
# get Homebrew prefix
HOMEBREW_PREFIX := $(shell brew --prefix)
# get boost include path (no pkgconfig file)
BOOST_INCLUDE := $(shell find $(HOMEBREW_PREFIX) -path '*/Cellar/boost/*/include' -type d | head -n 1)
THRIFT_FLAGS += -I$(BOOST_INCLUDE)
THRIFT_FLAGS += $(shell pkg-config --cflags openssl)
endif
THRIFT_LIBS = $(shell pkg-config --libs thrift)

all: hello_client hello_client_compact hello_client_ssl hello_client_py.stamp

hello_client.stamp: ../hello.thrift
	thrift --gen cpp:no_skeleton $<

$(GENSRC): hello_client.stamp
	touch $@

%.o: %.cpp $(GENHDR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THRIFT_FLAGS) -o $@ -c $<

hello_client: src/main.cpp  $(GENOBJ) $(GENHDR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THRIFT_FLAGS) -o $@ $< $(GENOBJ) $(THRIFT_LIBS)

hello_client_compact: src/main.cpp  $(GENOBJ) $(GENHDR)
	$(CXX) -DCONFIG_THRIFT_COMPACT_PROTOCOL=1  $(CPPFLAGS) $(CXXFLAGS) $(THRIFT_FLAGS) -o $@ $< $(GENOBJ) $(THRIFT_LIBS)

hello_client_ssl: src/main.cpp  $(GENOBJ) $(GENHDR)
	$(CXX) -DCONFIG_THRIFT_SSL_SOCKET=1 $(CPPFLAGS) $(CXXFLAGS) $(THRIFT_FLAGS) -o $@ $< $(GENOBJ) $(THRIFT_LIBS)

hello_client_py.stamp: ../hello.thrift
	thrift --gen py $<
	touch $@

clean:
	rm -Rf hello_client hello_client_compact hello_client_ssl $(GEN_DIR) gen-py *.stamp
