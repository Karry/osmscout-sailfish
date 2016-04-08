#!/bin/bash

# this function is just verbose equivalent of export
function exprt {
	echo "export $1"
	export "$1"
}

##################################################################
## project configuration
exprt PROJECT_NAME=osmscout-sailfish

exprt SDK_ROOT="$HOME/SailfishOS"
exprt SDK_SHARED_SRC="$SDK_ROOT/projects"
exprt SDK_SHARED_MNT="/home/src1"
exprt SDK_VM_NAME=MerSDK
exprt SDK_SSH_HOST=localhost
exprt SDK_SSH_PORT=2222
exprt SDK_SSH_USER=mersdk
exprt SDK_SSH_ID="$SDK_ROOT/vmshare/ssh/private_keys/engine/$SDK_SSH_USER"

##################################################################
## SDK vm
if [ `VBoxManage list runningvms | grep -c "$SDK_VM_NAME"` -eq 0 ] ; then
	echo "# Starting SDK..."
	VBoxHeadless -startvm "$SDK_VM_NAME" &
	#VBoxSDL -startvm "$SDK_VM_NAME" &
# 	VBoxManage controlvm "$SDK_VM_NAME" poweroff
	sleep 3
else
	echo "# SDK is running already"
fi

function sdk_cmd {
 echo "$@" | ssh \
    -q \
    -p $SDK_SSH_PORT \
    -i "$SDK_SSH_ID" \
    "$SDK_SSH_USER@$SDK_SSH_HOST"
}

function build_target {
	TARGET=$1
	echo "# building target $TARGET"
	BUILD_DIR="$SDK_SHARED_SRC/$PROJECT_NAME/build-$TARGET"
	mkdir -p "$BUILD_DIR"
	cp "$SDK_SHARED_SRC/$PROJECT_NAME/scripts/build.sh" "$BUILD_DIR"
	sdk_cmd "$SDK_SHARED_MNT/$PROJECT_NAME/build-$TARGET/build.sh $TARGET"
}

sdk_cmd sdk-assistant list | while read TARGET ; do 
	build_target $TARGET
done
