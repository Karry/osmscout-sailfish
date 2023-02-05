#!/bin/bash

for size in 86x86 108x108 128x128 172x172 256x256 ; do
  convert -background none graphics/harbour-osmscout-v2.svg -resize $size icons/$size/harbour-osmscout.png 
done
