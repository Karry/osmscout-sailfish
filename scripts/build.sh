#!/bin/bash

cd "`dirname $0`"

if [ $# -lt 1 ] ; then
	echo "Usage:"
	echo "$0 build-target"
	exit 1
fi

TARGET=$1
sb2 -t $TARGET -m sdk-build cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr .. && sb2 -t $TARGET -m sdk-build make
