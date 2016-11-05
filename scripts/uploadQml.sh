#!/bin/bash

cd "`dirname $0`/.."

scp -r ./qml root@jolla:/usr/share/harbour-osmscout/

