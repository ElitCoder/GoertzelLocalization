#!/bin/bash

cores=`grep --count ^processor /proc/cpuinfo`

function build_nessh {
	cd nessh/
	make clean
	make -j $cores
	
	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	sudo make install
	
	cd cli/
	make clean
	make -j $cores
	
	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	sudo make install
}

function build_libssh {
	unar libssh-0.7.5.tar.xz
	cd libssh-0.7.5/
	mkdir build
	cd build/
	cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ..
	make -j $cores
	sudo make install
	cd ../../
}

function build_curlpp {
	unar curlpp-0.8.1.tar.gz
	cd curlpp-0.8.1/ && mkdir build; cd build/
	cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ..
	make -j $cores
	sudo make install
	cd ../../
}

cd dependencies/

if [ $# -ne 0 ]; then
	if [ $1 = "nessh" ]; then
		build_nessh
	fi
	
	if [ $1 = "libssh" ]; then
		build_libssh
	fi	
	
	if [ $1 = "curlpp" ]; then
		build_curlpp
	fi
	
	exit 0
fi

# build dependencies
sudo apt-get update && sudo apt-get install g++ unar cmake libssl-dev zlib1g-dev libcurl4-openssl-dev

# build all programs
build_libssh
build_curlpp
build_nessh