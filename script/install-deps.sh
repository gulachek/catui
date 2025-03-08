#!/bin/sh

set -e
set -x

. script/util.sh

cmake_build_install() {
	cmake --build build
	cmake --install build --prefix "$VENDOR"
	rm -rf build
}

SRC="$PWD"

# cJSON
CJSON_URL="https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.18.tar.gz"
CJSON="$VENDORSRC/cJSON"
md "$CJSON"

untar -u "$CJSON_URL" -d "$CJSON"
# cJSON CMakeLists.txt configures files that have full install paths. Must define prefix

cd "$CJSON"
cmake -DENABLE_CJSON_TEST=OFF "-DCMAKE_INSTALL_PREFIX=$VENDOR" -S . -B build
cmake_build_install

# msgstream
MSGSTREAM_URL="https://github.com/gulachek/msgstream/releases/download/v0.3.2/msgstream-0.3.2.tgz"
MSGSTREAM="$VENDORSRC/msgstream"
md "$MSGSTREAM"

untar -u "$MSGSTREAM_URL" -d "$MSGSTREAM"
cd "$MSGSTREAM"
cmake -S . -B build
cmake_build_install

# unixsocket
UNIX_URL="https://github.com/gulachek/unixsocket/releases/download/v0.1.1/unixsocket-0.1.1.tgz"
UNIX="$VENDORSRC/unixsocket"
md "$UNIX"

untar -u "$UNIX_URL" -d "$UNIX"
cd "$UNIX"
cmake -S . -B build
cmake_build_install
