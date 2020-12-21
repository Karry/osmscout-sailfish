#!/bin/bash -x

# Build of QTCharts is expensive, and SFOS SDK compiler is crashing randomly
# Maybe because it is running in virtualbox as arm binary emulated by qemu...
# Anyway, when we are pushing hard, it succeed after some time.


PATH=$PATH:~/SailfishOS/bin/
export OS_VERSION=3.4.0.24
for ARCHITECTURE in armv7hl i486 aarch64 ; do

  TARGET="SailfishOS-${OS_VERSION}-${ARCHITECTURE}"
  sfdk config "target=${TARGET}"
  sfdk config "no-fix-version"

  if [ "$ARCHITECTURE" = "i486" ] ; then
    ARCH=i386
  fi
  if [ "$ARCHITECTURE" = "aarch64" ] ; then
    ARCH=aarch64
  fi
  if [ "$ARCHITECTURE" = "armv7hl" ] ; then
    ARCH=arm
  fi

  sfdk build
  if [ $? -ne 0 ] ; then
    ATTEMPT=0
    while [ $ATTEMPT -lt 1000 ] ; do
      # sfdk make VERBOSE=1 qtcharts
      echo "sb2 -t $TARGET -m sdk-build bash -c \"cd /home/mersdk/share/SailfishOS/projects/osmscout-sailfish/rpmbuilddir-${ARCH} && make VERBOSE=1 qtcharts\"" | ssh -i ~/SailfishOS/vmshare/ssh/private_keys/engine/mersdk mersdk@localhost -p 2222

      if [ $? -eq 0 ] ; then
        break
      fi
      ATTEMPT=$(( $ATTEMPT + 1 ))
      sleep 1
    done
  fi
done
