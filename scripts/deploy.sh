#!/bin/bash
# http://nckweb.com.ar/sailing-code/2015/01/01/develop-without-qtcreator/

if [ $# -lt 1 ] ; then
	echo "Too few arguments!"
	echo "Usage:"
	echo "  $0 build-type"
	echo 
	echo "build type can be emulator or phone"
	
	exit 1
fi

export TYPE="$1" # emulator
export PROJECT_NAME="osmscout-sailfish" 
export PROJECT_TARGET="harbour-osmscout"

##################################################################
## project root
export MER_SSH_SHARED_SRC="$HOME/SailfishOS/projects"
## SDK ROOT
export SDK_ROOT="$HOME/SailfishOS"
SDK_VERSION=3.2.0.12

## build & deploy target
## list of configured devices: ~/SailfishOS/vmshare/devices.xml 
if [ "$TYPE" = "emulator" ] ; then
	export MER_SSH_DEVICE_NAME="Sailfish OS Emulator ${SDK_VERSION}"
	# 	ssh nemo@localhost -p 2223 -i ~/SailfishOS/vmshare/ssh/private_keys/Sailfish_OS-Emulator-latest/nemo
	export DEV_SSH_USER=nemo
	export DEV_SSH_HOST=localhost
	export DEV_SSH_PORT=2223
	export MER_SSH_TARGET_NAME=SailfishOS-${SDK_VERSION}-i486
	export DEV_SSH_KEY="$SDK_ROOT/vmshare/ssh/private_keys/Sailfish_OS-Emulator-latest/nemo"
	export DEV_SSH_ROOT_KEY="$SDK_ROOT/vmshare/ssh/private_keys/Sailfish_OS-Emulator-latest/root"
	
	## emulator vm
	if [ `VBoxManage list runningvms | grep -c "$MER_SSH_DEVICE_NAME"` -eq 0 ] ; then
		echo "Starting emulator"
# 		VBoxSDL -startvm "$MER_SSH_DEVICE_NAME" & 
		VBoxManage startvm "$MER_SSH_DEVICE_NAME"  
# 		VBoxManage controlvm "SailfishOS Emulator" poweroff
		sleep 3
	else
		echo "Emulator is running already"
	fi	
else
	export MER_SSH_DEVICE_NAME="Jolla (ARM)"
	export DEV_SSH_USER=nemo
	export DEV_SSH_HOST=jolla
	export DEV_SSH_PORT=22
	export MER_SSH_TARGET_NAME=SailfishOS-${SDK_VERSION}-armv7hl
	export DEV_SSH_KEY="$SDK_ROOT/vmshare/ssh/private_keys/Jolla_(ARM)/nemo"
	export DEV_SSH_ROOT_KEY="$SDK_ROOT/vmshare/ssh/private_keys/Jolla_(ARM)/nemo"
fi

if [ $# -ge 2 ] ; then
	export DEV_SSH_HOST=$2
fi


##################################################################
export MER_SSH_PROJECT_PATH="$MER_SSH_SHARED_SRC/$PROJECT_NAME"
export PROJECT_FILE="$MER_SSH_PROJECT_PATH/$PROJECT_TARGET.pro"

export PROJECT_BUILD_DIR="$MER_SSH_PROJECT_PATH/build-$MER_SSH_TARGET_NAME"
export MER_SSH_SDK_TOOLS="$HOME/.config/SailfishBeta7/mer-sdk-tools/MerSDK/$MER_SSH_TARGET_NAME"

export MER_SSH_SHARED_HOME="$HOME"
export MER_SSH_SHARED_TARGET="$SDK_ROOT/mersdk/targets/"
export SDK_VM_NAME="Sailfish OS Build Engine"

## SDK ssh config
export MER_SSH_CMD="$SDK_ROOT/bin/merssh"
export MER_SSH_PRIVATE_KEY="$SDK_ROOT/vmshare/ssh/private_keys/engine/mersdk"
export MER_SSH_USERNAME=mersdk
export MER_SSH_PORT=2222
export SDK_SSH_HOST=localhost

##################################################################
## SDK vm
if [ `VBoxManage list runningvms | grep -c "$SDK_VM_NAME"` -eq 0 ] ; then
	echo "Starting SDK..."
	VBoxHeadless -startvm "$SDK_VM_NAME" &
	#VBoxSDL -startvm "$SDK_VM_NAME" &
# 	VBoxManage controlvm "$SDK_VM_NAME" poweroff
	sleep 3
else
	echo "SDK is running already"
fi

function sdk_cmd {
 echo "$@" | ssh \
    -q \
    -p $MER_SSH_PORT \
    -i "$MER_SSH_PRIVATE_KEY" \
    "$MER_SSH_USERNAME@$SDK_SSH_HOST"
}

##################################################################
echo
echo "Checking SDK connection..."
echo "ssh \
    -p $MER_SSH_PORT \
    -i \"$MER_SSH_PRIVATE_KEY\" \
    \"$MER_SSH_USERNAME@$SDK_SSH_HOST\""

echo
echo "sb2-config -l" | ssh \
    -p $MER_SSH_PORT \
    -i "$MER_SSH_PRIVATE_KEY" \
    "$MER_SSH_USERNAME@$SDK_SSH_HOST" || exit 1

##################################################################
## 
echo
echo "build rpm..."

# HACK: mb2 mapping don't contains gcc libs like libgomp.so
sdk_cmd "sudo su -c 'cp /srv/mer/toolings/SailfishOS-${SDK_VERSION}/opt/cross/armv7hl-meego-linux-gnueabi/lib/libgomp.so.1.0.0  /srv/mer/targets/SailfishOS-${SDK_VERSION}-armv7hl/usr/lib/libgomp.so.1'"
sdk_cmd "sudo su -c 'cp /srv/mer/toolings/SailfishOS-${SDK_VERSION}/usr/lib/libgomp.so.1.0.0  /srv/mer/targets/SailfishOS-${SDK_VERSION}-i486/usr/lib/libgomp.so.1'"

sdk_cmd "cd /home/mersdk/share/SailfishOS/projects/$PROJECT_NAME/ && mb2 -t $MER_SSH_TARGET_NAME build"
  
##################################################################
## 
echo
echo "deploy..."

RPM_PATH=`find "$MER_SSH_PROJECT_PATH/RPMS" -name '*.rpm' | head -1`
if [ ! -f "$RPM_PATH" ] ; then
  echo "Can't found rpm package"
  exit 1
fi
RPM=`basename "$RPM_PATH"`

echo "Copy $RPM_PATH to root@$DEV_SSH_HOST:/tmp/"
scp  \
  -i "$DEV_SSH_ROOT_KEY" \
  -P "$DEV_SSH_PORT" \
  "$RPM_PATH" \
  "root@$DEV_SSH_HOST:/tmp/"

echo "Installing /tmp/$RPM"
echo "rpm -iv --replacepkgs --force /tmp/$RPM && rm /tmp/$RPM" | ssh \
  -i "$DEV_SSH_ROOT_KEY" \
  -p "$DEV_SSH_PORT" \
  "root@$DEV_SSH_HOST"

#"$MER_SSH_CMD" \
#  deploy --sdk
  
echo 
echo "run"
# "$MER_SSH_CMD" \
#   ssh "/usr/bin/$PROJECT_NAME"
# cat "$SDK_ROOT/vmshare/devices.xml"
ssh $DEV_SSH_USER@$DEV_SSH_HOST \
  -i "$DEV_SSH_KEY" \
  -p "$DEV_SSH_PORT" \
  "/usr/bin/$PROJECT_TARGET"




