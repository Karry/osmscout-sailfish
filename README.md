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
 * Install cmake and elf tools in SDK vm:
```
sb2 -t SailfishOS-armv7hl -m sdk-install -R zypper in cmake patchelf chrpath
sb2 -t SailfishOS-i486    -m sdk-install -R zypper in cmake patchelf chrpath
```
 * Compile project inside SDK vm:
```
cd /home/src1/osmscout-sailfish/
mkdir -p build-SailfishOS-i486
cd build-SailfishOS-i486
sb2 -t SailfishOS-i486 -m sdk-build cmake ..
sb2 -t SailfishOS-i486 -m sdk-build make

cd /home/src1/osmscout-sailfish/
mkdir -p build-SailfishOS-armv7hl
cd build-SailfishOS-armv7hl
sb2 -t SailfishOS-armv7hl -m sdk-build cmake ..
sb2 -t SailfishOS-armv7hl -m sdk-build make
```
 * Build rpm packages inside SDK vm:
```
cd /home/src1/osmscout-sailfish/
mb2 -t SailfishOS-i486 build
mb2 -t SailfishOS-armv7hl build
```
## Local development

For development tasks like debugging, memory and performance profiling 
is helpful to run application locally. 
When you start application with `--desktop` switch, it will use 
`desktop.qml` UI. You don't need `Sailfish.Silica` components on your machine.

First, you need to install libsailfishapp locally:

```
git clone https://github.com/sailfish-sdk/libsailfishapp.git
cd libsailfishapp
mkdir build
cd build
qmake-qt5 ..
make -j `nproc`
sudo make install
```

Then you can compile osmscout-sailfish:

```
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DQT_QML_DEBUG=yes ..
make -j `nproc`
./harbour-osmscout --desktop 
```

### Tunning examples
 * Qml profiler
```
./harbour-osmscout --desktop --qmljsdebugger=port:12345
```
 * Memory leak detection
```
valgrind -v --track-origins=yes --leak-check=full --show-leak-kinds=all ./harbour-osmscout --desktop
```
 * Heap usage profiling
```
LD_PRELOAD=/usr/local/lib/libtcmalloc.so HEAPPROFILE=./heap-profile ./harbour-osmscout --desktop 
# convert all samples to svg
find  . -name \*.heap -exec bash -c "pprof -svg harbour-osmscout {} | tee {}.svg" \;
```

 * Debug Qt render timing
```
QSG_RENDER_TIMING=1 ./harbour-osmscout --desktop
```
