An experimental [(n+1)sec](https://github.com/equalitie/np1sec) plugin for [Pidgin](https://www.pidgin.im/)
==================================================

**Important Warning**: This is a minimal test client for under-construction
software. It may crash and it may have undocumented security bugs. You should
not rely on it for security - or even stable communications.


# Try (n+1)sec!

We invite our community to try out (n+1)sec with us. We'll be hanging out in the
`np1sec` room on the `np1sec.equalit.ie` XMPP server
(`np1sec@conference.np1sec.equalit.ie`) between 14:00 and 17:00 UTC every Friday
until the end of 2016.

You'll need an account on the server to join us. Registrations are open and you
can create an account through the wizard that will open automatically when you
first launch the script to install this client.

* Protocol: XMPP
* Domain: np1sec.equalit.ie

Choose a user name you like and a strong password and check the "Create this new
account on the server" option before you click "Add". Confirm your user name and
password in the prompt, click OK and you'll have an account on the testing
Jabber server.

We look forward to chatting with you all securely!


# Setup for users

The (n+1)sec test client works on any Linux distribution.

## Dependencies

Before you run the script to install the np1sec-test-client, you should check
that the following packages are installed on your system, and install them if
they are not. You will also need a C++ compiler that supports C++14. In Debian
you can install the `build-essential` package.

```
cmake
git
intltool
libasan
libboost1.55-dev (or higher)
libboost-all-dev
libgcrypt-dev
libglib2.0-dev
libgnutls-dev
libgtk2.0-dev
libxml2-dev
wget
```

In Fedora you can install all dependencies using the following command (tested 
and working on Fedora 25):

```
dnf install cmake git intltool boost-devel libgcrypt-devel glib2-devel gnutls-devel gtk2-devel libasan libxml2-devel wget
```


## Build

np1sec-test-client comes with a convenient script, which will download and
build pidgin-2.11, the (n+1)sec library and this plugin for you.

This script builds a separate Pidgin instance with (n+1)sec support, which uses
a separate configuration from any Pidgin you may already be running on your
machine. This isn't strictly necessary and you could run (n+1)sec in your
day-to-day Pidgin if you like, but this solution will avoid several problems
with this test build and avoid your regular chat client being compromised by
any bugs in (n+1)sec.

To run it, download this file to an empty directory:
https://raw.githubusercontent.com/equalitie/np1sec-test-client/master/run-np1sec.sh

Check that the file has the right permissions, else make it executable with the
command:

```
chmod 775 run-np1sec.sh
```

Finally, within the directory containing the script, launch the following
command to run the install script:

```
./run-np1sec.sh
```

The script performs some automatic dependency resolution before building. This
should work well for users of Debian-based distributions, but users of other
distributions can disable this dependency resolution by executing the script with:
```
./run-np1sec.sh --force
```

On success, the script will start Pidgin, where you can go to `Tools > Plugins`
and enable the `(n+1)sec Secure messaging` plugin.

Subsequent Pidgin executions can be done by executing the same command again.
Note however, that executing it will update the np1sec and np1sec-test-client
projects to their latest versions. To avoid that one may instead execute:

```
./bin/bin/pidgin --config=pidgin-home
```


## Using the (n+1)sec plugin

Once you have enabled the plugin (`Tools > Plugins > (n+1)sec secure
messaging`), join the XMPP ("Jabber") room ("chat") where you are planning to
have your encrypted conversation [as you would normally with
Pidgin](https://developer.pidgin.im/wiki/Using%20Pidgin#ChatroomsConferences).

When you have joined the room you can chat in the clear (unencrypted) with
other people as normal.

Under the list of `n people in this room`, you will see a second list called
`(n+1)sec users` -- this shows everyone in the room who has installed and
enabled their (n+1)sec plugin.

To invite people to a new encrypted conversation, click the `Create
conversation` button. A new chat window will open. You can then select people
to invite by double-clicking their usernames in the `Invite` pane of the new
window (this list shows only those people who are (n+1)sec capable). Once
someone is invited, their username is moved to the `Invited` pane, and when
they accept the invitation they are moved to the `Joined` pane.

If you are invited to a conversation, a new window will open automatically.
The `Joined` list in that window tells you who is in this conversation already,
and the `Invited` list shows who has been invited to join. Find your username
in the `Invited` pane of that window and double-click it to accept the
invitation. Your username will move to the `Joined` pane.

When someone is first added to the `joined` pane, their username is suffixed
with a `!c` symbol, which indicates that the conversation is rotating its
encryption keys to include them. They won't be party to the conversation,
meaning they won't be able to read or write encrypted chat messages, until this
disappears to indicate the key exchange is complete.

Invitees who have not accepted their invitations are not party to an encrypted
conversation and cannot see its messages.

To leave an encrypted conversation, or to decline an invitation, simply close
its window.


# Known bugs

## User's chat "handle" must match their JID

When you "join a chat" (i.e an XMPP MUC room) in Pidgin, the client requires
you to set a "Handle" or nickname. (n+1)sec will not work unless you set this
to be exactly the same as the first part of your Jabber ID (the part before the `@`
symbol). This is case sensitive.

Example:

- JID: richard@np1sec.equalit.ie/resource
- Correct handle: "richard"


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
