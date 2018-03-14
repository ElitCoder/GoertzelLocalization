#!/bin/bash

# no need if no sudo
exit 0

cd dep/
unar libssh-0.7.5.tar.xz
cd libssh-0.7.5/
mkdir build
cd build/
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug -DWITH_STATIC_LIB=ON ..
make -j 9

if [ $? -ne 0 ]; then
	echo "Building failed, copy the prebuilts instead and hope for the best"
	
	cd ../../prebuilt/
	cd include/
	echo "Copying include/libssh.. "
	sudo cp -r libssh /usr/include
	cd ../lib/
	echo "Copying lib/.."
	sudo cp * /usr/lib/
	
	exit 0
fi

sudo make install
cd ../../

exit 0