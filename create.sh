#!/bin/bash

cores=`grep --count ^processor /proc/cpuinfo`

function build {
	cd Server/
	
	if [ $# -ne 0 ]; then
		make clean
	fi
	
	make -j $cores

	if [ $? -ne 0 ]; then
		exit 1
	fi

	cd ../Client/
	
	if [ $# -ne 0 ]; then
		make clean
	fi
	
	make -j $cores

	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	cd ../
}

if [ $# -eq 0 ]; then
	build
else
	if [ $1 = "clean" ]; then
		build clean
	else
		cd $1
		make -j $cores
		
		if [ $? -ne 0 ]; then
			exit 1
		fi
		
		cd ../
	fi
fi