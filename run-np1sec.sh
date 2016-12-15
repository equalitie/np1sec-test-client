#!/bin/sh

#
# Builds or updates a pidgin client with np1sec plugin, then runs it.
#

set -e

NPROC=1

if [ "$(expr substr $(uname -s) 1 5)" = "Linux" ]; then
	NPROC=`nproc`
fi

testheader() {
	RETURN=0
	echo "#include <$1>\nmain(){}" > conftest.c
	${CXX-c++} conftest.c -o conftest.a.out >/dev/null 2>/dev/null || RETURN=1
	rm -f conftest.c conftest.a.out
	return $RETURN
}

testasan() {
	RETURN=0
	echo "main(){}" > conftest.c
	${CXX-c++} conftest.c -fsanitize=address -o conftest.a.out >/dev/null 2>/dev/null || RETURN=1
	rm -f conftest.c conftest.a.out
	return $RETURN
}

libname() {
	# TODO: support os x
	echo "lib$1.so"
}

libdir() {
	# TODO: support os x
	echo lib
}

FORCE=false
if [ "$1" "=" "--force" ]; then
	FORCE=true
	shift
fi

WORKDIR="`pwd`"


DEPS=""

type ${CC-cc} >/dev/null || DEPS="${DEPS} c-compiler"
type ${CXX-c++} >/dev/null || DEPS="${DEPS} cxx-compiler"
type pkg-config >/dev/null || DEPS="${DEPS} pkg-config"

if [ -n "${DEPS}" ]; then
	echo "Missing dependencies: ${DEPS}"
	${FORCE} || echo "Ignore this warning with --force."
	${FORCE} || exit 1
	echo "Ignoring missing dependencies."
fi

type git >/dev/null || DEPS="${DEPS} git"
type wget >/dev/null || DEPS="${DEPS} wget"
type intltool-update >/dev/null || DEPS="${DEPS} intltool"
type cmake >/dev/null || DEPS="${DEPS} cmake"
pkg-config --exists glib-2.0 || DEPS="${DEPS} libglib2.0-dev"
pkg-config --exists gtk+-2.0 || pkg-config --exists gtk+-3.0 || DEPS="${DEPS} libgtk2.0-dev"
pkg-config --exists libxml-2.0 || DEPS="${DEPS} libxml2-dev"
pkg-config --exists gnutls || DEPS="${DEPS} libgnutls-dev"
testheader gcrypt.h || DEPS="${DEPS} libgcrypt-dev"
testheader boost/optional.hpp || DEPS="${DEPS} libboost-all-dev"

if [ -n "${DEPS}" ]; then
	echo "Missing dependencies: ${DEPS}"
	${FORCE} || echo "Ignore this warning with --force."
	${FORCE} || exit 1
	echo "Ignoring missing dependencies."
fi

if testasan; then
	export CFLAGS="-fsanitize=address -ggdb"
	export CXXFLAGS="-fsanitize=address -ggdb"
fi

PIDGIN_TAR_FILE=pidgin-2.11.0.tar.bz2
PIDGIN_SHA256=f72613440586da3bdba6d58e718dce1b2c310adf8946de66d8077823e57b3333

if [ ! -x bin/bin/pidgin ]; then
	rm -rf pidgin-build pidgin-2.11.0
	if [ ! -f ${PIDGIN_TAR_FILE} ]; then
		wget http://sourceforge.net/projects/pidgin/files/Pidgin/2.11.0/${PIDGIN_TAR_FILE}
	fi
	CHECKSUM=`sha256sum "${PIDGIN_TAR_FILE}" | awk '{ print $1; }'`
	if [ "${PIDGIN_SHA256}" != "${CHECKSUM}" ]; then
		echo "Pidgin: invalid checksum"
		exit 1
	fi
	tar xfj ${PIDGIN_TAR_FILE}
	mkdir pidgin-build
	cd pidgin-build
	../pidgin-2.11.0/configure --disable-missing-dependencies \
	                           --disable-consoleui \
	                           --with-dynamic-prpls=jabber \
	                           --prefix="${WORKDIR}"/bin
	make -j ${NPROC} ${MAKEOPTS}
	make install
	cd ..
fi

NP1SEC_BRANCH=api-docs
if [ ! -d np1sec ]; then
	rm -rf np1sec np1sec-build bin/"`libdir`"/"`libname np1sec`"
	git clone https://github.com/equalitie/np1sec.git np1sec
	cd np1sec
	git checkout "${NP1SEC_BRANCH}"
	cd ..
	mkdir np1sec-build
	cd np1sec-build
	cmake ../np1sec -DCMAKE_INSTALL_PREFIX="${WORKDIR}"/bin -DBUILD_TESTS=Off -DCMAKE_BUILD_TYPE=Debug
	make -j ${NPROC} ${MAKEOPTS}
	cp "`libname np1sec`" ../bin/"`libdir`"/
	cd ..
else
	cd np1sec
	git remote update
	if [ `git rev-parse @` != `git rev-parse origin/"${NP1SEC_BRANCH}"` -o ! -e ../bin/"`libdir`"/"`libname np1sec`" ]; then
		git checkout "${NP1SEC_BRANCH}"
		git pull origin "${NP1SEC_BRANCH}"
		cd ..
		cd np1sec-build
		make -j ${NPROC} ${MAKEOPTS}
		cp "`libname np1sec`" ../bin/"`libdir`"/
	fi
	cd ..
fi

NP1SEC_TEST_CLIENT_BRANCH=devel
if [ ! -d np1sec-test-client ]; then
	rm -rf np1sec-test-client np1sec-test-client-build
	git clone https://github.com/equalitie/np1sec-test-client.git np1sec-test-client
	cd np1sec-test-client
	git checkout "${NP1SEC_TEST_CLIENT_BRANCH}"
	cd ..
	mkdir np1sec-test-client-build
	cd np1sec-test-client-build
	cmake ../np1sec-test-client \
		-DPIDGIN_INC_DIR="${WORKDIR}"/bin/include \
		-DNP1SEC_LIB_DIR="${WORKDIR}"/bin/lib \
		-DNP1SEC_INC_DIR="${WORKDIR}"/np1sec \
		-DCMAKE_BUILD_TYPE=Debug
	make ${MAKEOPTS}
	cp "`libname np1sec-plugin`" ../bin/
	cd ..
else
	cd np1sec-test-client
	git remote update
	if [ `git rev-parse @` != `git rev-parse origin/"${NP1SEC_TEST_CLIENT_BRANCH}"` -o ! -e ../bin/"`libname np1sec-plugin`" ]; then
		git checkout "${NP1SEC_TEST_CLIENT_BRANCH}"
		git pull origin "${NP1SEC_TEST_CLIENT_BRANCH}"
		cd ..
		cd np1sec-test-client-build
		make ${MAKEOPTS}
		cp "`libname np1sec-plugin`" ../bin/
	fi
	cd ..
fi

if [ ! -e pidgin-home/plugins/"`libname np1sec-plugin`" ]; then
	mkdir pidgin-home
	mkdir pidgin-home/plugins
	ln -s ../../bin/"`libname np1sec-plugin`" pidgin-home/plugins/"`libname np1sec-plugin`"
fi

(cd np1sec && echo "Revision of np1sec: $(git rev-parse --short HEAD)")
(cd np1sec-test-client && echo "Revision of np1sec-test-client: $(git rev-parse --short HEAD)")

# Tell the np1sec test client to output debug log.
export NP1SEC_TEST_CLIENT_PRINT_LOG=true

bin/bin/pidgin --config=pidgin-home
