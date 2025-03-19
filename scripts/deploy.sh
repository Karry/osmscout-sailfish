#!/bin/bash

if [ $# -lt 1 ] ; then
  echo "Too few arguments!"
  echo "Usage:"
  echo "  $0 build-type [device name]"
  echo 
  echo "build type can be emulator or phone"
  
  exit 1
fi

export TYPE="$1" # emulator

##################################################################
## configure SDK
PATH=$PATH:~/SailfishOS/bin/
export OS_VERSION=${OS_VERSION:-5.0.0.62}

# device may be configured in SailfishOS SDK
if [ $# -ge 2 ] ; then
  export DEV_DEVICE="$2"
else
  # use default device for TYPE
  if [ "$TYPE" = "i486" ] ; then
    export DEV_DEVICE="Sailfish OS Emulator ${OS_VERSION}"
  elif [ "$TYPE" = "emulator" ] ; then
    export DEV_DEVICE="Sailfish OS Emulator ${OS_VERSION}"
  elif [ "$TYPE" = "aarch64" ]; then
    export DEV_DEVICE="Xperia"
  else
    export DEV_DEVICE="Intex"
  fi
fi

if [ "$TYPE" = "i486" ] ; then
  export ARCHITECTURE=i486
elif [ "$TYPE" = "emulator" ] ; then
  export ARCHITECTURE=i486
elif [ "$TYPE" = "aarch64" ]; then
  export ARCHITECTURE=aarch64
elif [ "$TYPE" = "armv7hl" ]; then
  export ARCHITECTURE=armv7hl
else
  echo "Uknown build type: $TYPE" 1>&2
  exit 1
fi

sfdk config "target=SailfishOS-${OS_VERSION}-${ARCHITECTURE}"
if [ $? -ne 0 ] ; then
  echo
  echo "Available targets:"
  sfdk tools list 2> /dev/null
  exit 1;
fi

# ~/.config/SailfishSDK/libsfdk/devices.xml
# ~/SailfishOS/vmshare/devices.xml
if [ -n "$DEV_DEVICE" ] ; then
  sfdk config "device=${DEV_DEVICE}"
  if [ $? -ne 0 ] ; then
    echo
    echo "Available devices:"
    sfdk device list 2> /dev/null
    exit 1;
  fi
fi

sfdk config "no-fix-version"
if [ $? -ne 0 ] ; then
  echo
  echo "Failed to set no-fix-version"
  exit 1;
fi

##################################################################
echo
echo "build rpm..."

# HACK: gcc libs like libgomp.so are missing under the build target
sfdk engine exec sudo cp \
    /srv/mer/toolings/SailfishOS-${OS_VERSION}/opt/cross/armv7hl-meego-linux-gnueabi/lib/libgomp.so.1.0.0 \
    /srv/mer/targets/SailfishOS-${OS_VERSION}-armv7hl/usr/lib/libgomp.so.1

sfdk engine exec sudo cp \
    /srv/mer/toolings/SailfishOS-${OS_VERSION}/usr/lib/libgomp.so.1.0.0 \
    /srv/mer/targets/SailfishOS-${OS_VERSION}-i486/usr/lib/libgomp.so.1

sfdk --quiet  build --enable-debug
if [ $? -ne 0 ] ; then
  echo
  echo "Failed to build"
  exit 1;
fi

##################################################################
if [ -n "$DEV_DEVICE" ] ; then
  echo
  echo "deploy..."

  if [ "$TYPE" = "emulator" ] ; then
    sfdk emulator start "${DEV_DEVICE}"
  fi

  # to be able deploy to emulator with sfdk, there have to working connection from SDK VM/Docker to emulator VM.
  # For that there have to be NAT (on emulator) and DNAT (on SDK) properly configured
  # see forum topic for more details: https://forum.sailfishos.org/t/sdk-3-2-unable-to-deploy-package-to-emulator-with-docker-based-build-engine/1860/8
  sfdk deploy --sdk --debug
  if [ $? -ne 0 ] ; then
    echo
    echo "Failed to deploy"
    exit 1;
  fi

  echo
  echo "run"
  sfdk device exec -- /bin/sh -c "LC_ALL=en_US.UTF-8 /usr/bin/harbour-osmscout --log debug"
  if [ $? -ne 0 ] ; then
    echo
    echo "Failed to exec"
    exit 1;
  fi
fi
