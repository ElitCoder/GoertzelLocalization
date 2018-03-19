#!/bin/bash

function build_nessh {
	cd nessh/
	make clean
	make -j 9
	
	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	sudo make install
	
	cd cli/
	make clean
	make -j 9
	
	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	sudo make install
}

cd dependencies/

if [ $# -ne 0 ]; then
	if [ $1 = "nessh" ]; then
		build_nessh
	fi
	
	exit 0
fi 		

unar libssh-0.7.5.tar.xz
cd libssh-0.7.5/
mkdir build
cd build/
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug -DWITH_STATIC_LIB=ON ..
make -j 9
sudo make install
cd ../../

build_nessh