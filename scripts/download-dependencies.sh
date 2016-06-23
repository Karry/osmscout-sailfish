#!/bin/bash

cd "`dirname $0`/.."

mkdir -p dependencies
cd dependencies

git clone -b master https://github.com/karry-space-with-my-second-forks/libosmscout.git libosmscout

wget https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/marisa-trie/marisa-0.2.4.tar.gz
zcat marisa-0.2.4.tar.gz | tar -xf -
