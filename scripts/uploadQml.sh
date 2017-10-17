#!/bin/bash

cd "`dirname $0`/.."

DEV_SSH_HOST=jolla
if [ $# -ge 1 ] ; then
	export DEV_SSH_HOST=$1
fi

scp -r ./qml root@$DEV_SSH_HOST:/usr/share/harbour-osmscout/

