[np1sec](https://github.com/equalitie/np1sec) plugin for [pidgin](https://www.pidgin.im/)
==================================================

**Important Warning**: This is a test build for under-construction software. Do
not rely on it for secure communications.

# Setup for users

The (n+1)sec test client can be used in any Linux distribution.

## Dependencies

Before you run the script to install the np1sec-test-client, you should check
that the following packages are installed in your system and install them if
they are not.

```
cmake
git
wget
intltool
libboost1.55-dev (or higher)
libglib2.0-dev
libgtk2.0-dev
libxml2-dev
libgcrypt-dev
libgnutls-dev
intltool
```

You will also need a C++ compiler that supports C++14. In Debian you can install
the `build-essential` package. In Fedora you can use the following command:

```
yum install make gcc gcc-c++ kernel-devel
```

## Build

np1sec-test-client comes with a convenience script to download and build
pidgin-2.11, the (n+1)sec library and this plugin.

This script builds a separate Pidgin instance with (n+1)sec support, which uses
a separate configuration from any pidgin you may already be running in your
machine. This isn't strictly necessary and you could run (n+1)sec in your
day-to-day pidgin if you like, but this solution will avoid several problems
with this test build and prevent that your regular chat client is compromised by
any bugs in (n+1)sec.

To run it, download this file to an empty directory:
https://raw.githubusercontent.com/equalitie/np1sec-test-client/master/run-np1sec.sh

Check that the file has the right permissions, else make it executable with the
command:

```
chmod 774 run-np1sec.sh
```

Finally, within the directory containing the script, launch the following
command to run the install script:

```
./run-np1sec.sh
```

On success, the script shall start pidgin, where you can go to Tools > Plugins
and enable the `(n+1)sec Secure messaging` plugin.

Subsequent pidgin executions can be done by executing the same command again.
Note however, that executing it shall update the np1sec and np1sec-test-client
projects to their latest versions. To avoid that one may instead execute

```
./bin/bin/pidgin --config=pidgin-home
```

## How to start a group chat with the (n+1)sec plugin

Once you have enabled the plugin (Tools > Plugins > (n+1)sec secure messaging), you can access the chat where you are planning to have your encrypted group chat conversation as you normally do with
[Pidgin](https://developer.pidgin.im/wiki/Using%20Pidgin#ChatroomsConferences).

Under the list of "x people in this room", you will see a further window below listing "Users" -- here you will see everyone who has installed and enabled their (n+1)sec plugin.

To invite people to an encrypted chat, click the "Create conversation" button. A new chat window will open. You will need to select users to invite to the encrypted conversation, by double-clicking their username in the "Invite" window. Once a user is invited, they are moved to the "Invited" window. The invited user needs to accept by right-clicking on their username and selecting "Join". When all users have accepted your invitation, you can start the conversation.


## Try it out!

We invite our community to try out (n+1)sec with us. We'll be hanging out
in the 'np1sec' room (np1sec@conference.np1sec.equalit.ie) on the np1sec.equalit.ie
XMPP server between 15:00 and 18:00 CET every Friday until the end of the year.
We look forward to chatting with you all securely!


# Setup for developers

For developers it is better to use a custom built pidgin. This is because it can
be built with the minimum functionality it needs to work and thus causes less
problems when (e.g.) multiple instances of the messenger need to run on the same
machine.

## Build pidgin

Download and extract pidgin source from
[here](https://www.pidgin.im/download/source/).

Then:

```
cd <pidgin-directory>
./configure --disable-gtkspell --disable-gstreamer --disable-vv
--disable-meanwhile --disable-avahi --disable-dbus
--prefix=`pwd`/install
make install # Shall install into ./install
```

## Build (n+1)sec

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

To test it, you'll likely want to run pidgin multiple times on the same machine.

```
mkdir -p ~/pidgin
<pidgin-directory>/install/bin/pidgin --config=~/pidgin/alice
<pidgin-directory>/install/bin/pidgin --config=~/pidgin/bob
<pidgin-directory>/install/bin/pidgin --config=~/pidgin/charlie
```

(Re)start pidgin, go to Tools > Plugins and enable the `(n+1)sec Secure
messaging` plugin.
