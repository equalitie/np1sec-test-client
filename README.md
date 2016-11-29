[np1sec](https://github.com/equalitie/np1sec) plugin for [pidgin](https://www.pidgin.im/)
==================================================

# Setup for users

## Dependencies
```
cmake
git
wget
libboost1.58-dev (or higher)
libglib2.0-dev
libgtk2.0-dev
libxml2-dev
libgcrypt-dev
libgnutls-dev
```

## Build

np1sec-test-client comes with a convenience script to download and build
pidgin-2.11, the np1sec library and this plugin. To run it, invoke

```
./run-np1sec.sh
```

On success, the script shall start pidgin where you can go to
Tools > Plugins and enable the `(n+1)sec Secure messaging` plugin.

Subsequent pidgin executions can be done by executing the same command
again. Note however, note that executing it shall update the np1sec and
np1sec-test-client projects to their latest versions. To avoid that
one may instead execute

```
./bin/bin/pidgin --config=pidgin-home
```

# Setup for developers

For developers it is better to use a custom built pidgin. This is because
it can be built with the minimum functionality it needs to work and
thus causes less problems when (e.g.) multiple instances of the messenger
need to run on the same machine.

## Build pidgin

Download and extract pidgin source from [here](https://www.pidgin.im/download/source/).
Then:

```
cd <pidgin-directory>
./configure --disable-gtkspell --disable-gstreamer --disable-vv --disable-meanwhile --disable-avahi --disable-dbus --prefix=`pwd`/install
make install # Shall install into ./install
```

## Other dependencies
```
libboost1.58-dev
libgtk2.0-dev
```

## Build
```
cd <np1sec-test-client dir>
mkdir -p build
cd build
cmake .. -DNP1SEC_LIB_DIR=<where libnp1sec.so can be found> \
         -DNP1SEC_INC_DIR=<where np1sec headers can be found>
make
```

Make a link to the plugin where pidgin can find it:
```
cp libnp1sec-plugin.so ~/.purple/plugins/
```

## Test
To test it, you'll likely want to run pidgin multiple times on the
same machine.

```
mkdir -p ~/pidgin
<pidgin-directory>/install/bin/pidgin --config=~/pidgin/alice
<pidgin-directory>/install/bin/pidgin --config=~/pidgin/bob
<pidgin-directory>/install/bin/pidgin --config=~/pidgin/charlie
```

(Re)start pidgin, go to Tools > Plugins and enable the `(n+1)sec Secure messaging` plugin.
