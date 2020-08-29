#!/bin/bash -e

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
export OS_VERSION=3.3.0.16

# device may be configured in SailfishOS SDK
if [ $# -ge 2 ] ; then
  export DEV_DEVICE="$2"
else
  # use default device for TYPE
  if [ "$TYPE" = "emulator" ] ; then
    export DEV_DEVICE="Sailfish OS Emulator ${OS_VERSION}"
  else
    export DEV_DEVICE="Intex"
  fi
fi

if [ "$TYPE" = "emulator" ] ; then
  export ARCHITECTURE=i486
else
  export ARCHITECTURE=armv7hl
fi

sfdk config "target=SailfishOS-${OS_VERSION}-${ARCHITECTURE}"
if [ $? -ne 0 ] ; then
  echo
  echo "Available targets:"
  sfdk tools list 2> /dev/null
  exit 1;
fi

sfdk config "device=${DEV_DEVICE}"
if [ $? -ne 0 ] ; then
  echo
  echo "Available devices:"
  sfdk device list 2> /dev/null
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

sfdk --quiet build
  
##################################################################

if [ "$TYPE" = "emulator" ] ; then
  sfdk emulator start "${DEV_DEVICE}"

  # deploy to emulator with sfdk dont work for some reason :-(
  RPM_PATH=$(find RPMS -name '*i486.rpm' | head -1)
  echo "Copy $RPM_PATH to root@localhost:/tmp/"
  scp  \
    -i "~/SailfishOS/vmshare/ssh/private_keys/Sailfish_OS-Emulator-latest/root" \
    -P 2223 \
    "$RPM_PATH" \
    "root@localhost:/tmp/"

  RPM=$(basename "$RPM_PATH")
  echo "Installing /tmp/$RPM"
  echo "rpm -iv --replacepkgs --force /tmp/$RPM && rm /tmp/$RPM" | ssh \
    -i "~/SailfishOS/vmshare/ssh/private_keys/Sailfish_OS-Emulator-latest/root" \
    -p 2223 \
    "root@localhost"

else

  echo
  echo "deploy..."
  sfdk deploy --sdk
fi

echo
echo "run"
sfdk device exec -- /bin/sh -c "LC_ALL=en_US.UTF-8 /usr/bin/harbour-osmscout --log debug"
