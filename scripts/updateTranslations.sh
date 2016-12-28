#!/bin/bash

cd "`dirname $0`/.."

find translations -name *.ts | while read ts ; do
  lupdate osmscout-sailfish.pro -ts "$ts"
done
