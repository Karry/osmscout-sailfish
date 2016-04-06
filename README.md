# OSM Scout for Sailfish OS

[OSM Scout](http://wiki.openstreetmap.org/wiki/Libosmscout) library provides API for **offline map** rendering
and **routing**. Goal of this project is port Scout demo application to native Sailfish OS look and feel.

## Howto build for Sailfish OS

Before you start, you should know that build process is little bit commplicated.
If something don't work for you, read carefully following articles:

 * [Sailing Code: Develop without QtCreator](http://nckweb.com.ar/sailing-code/2015/01/01/develop-without-qtcreator/)
 * [Building Sailfish OS packages manually](https://sailfishos.org/develop/tutorials/building-sailfish-os-packages-manually/)

### Steps:

 * Install [SailfishOS SDK](https://sailfishos.org/develop/sdk-overview/develop-installation-article/) 
   on your machine. This howto expect that it is installed in default location - your home directory.
 * Create projects directory and checkout *osmscout-sailfish*:
```
mkdir -p ~/SailfishOS/projects/
cd ~/SailfishOS/projects/
git co https://github.com/Karry/osmscout-sailfish.git osmscout-sailfish
cd osmscout-sailfish
```
  * Download dependencies (osmscout and marisa libraries):
```
./scripts/download-dependencies.sh
```
  * Start *Mer SDK* and *SailfishOS Emulator* virtual machines:
```
VBoxManage startvm "SailfishOS Emulator" 
VBoxHeadless -startvm "MerSDK"
```
 * Connect to SDK vm
```
ssh mersdk@localhost -p 2222 -i ~/SailfishOS/vmshare/ssh/private_keys/engine/mersdk
```
 * Install cmake in SDK vm:
```
sb2 -t SailfishOS-armv7hl -m sdk-install -R zypper in cmake
sb2 -t SailfishOS-i486 -m sdk-install -R zypper in cmake
```
 * Compile project inside SDK vm:
```
cd /home/src1/osmscout-sailfish/
mkdir -p build-SailfishOS-i486
cd build-SailfishOS-i486
sb2 -t SailfishOS-i486 -m sdk-build cmake ..
sb2 -t SailfishOS-i486 -m sdk-build make
```
 * Copy build directory to Emulator vm and run it!
```
tar -cf build-SailfishOS-i486.tar build-SailfishOS-i486
scp -P 2223 -i ~/SailfishOS/vmshare/ssh/private_keys/SailfishOS_Emulator/nemo  build-SailfishOS-i486.tar nemo@localhost:/home/nemo
ssh -v nemo@localhost -p 2223 -i ~/SailfishOS/vmshare/ssh/private_keys/SailfishOS_Emulator/nemo 
tar -xf build-SailfishOS-i486.tar
cd build-SailfishOS-i486
LD_LIBRARY_PATH=.:./dependencies/libosmscout/libosmscout:./dependencies/libosmscout/libosmscout-map:./dependencies/libosmscout/libosmscout-map-qt ./osmscout-sailfish
```

