#!/bin/bash

function build {
	if [ $# -ne 0 ]; then
		make clean
	fi
	
	make -j 9

	if [ $? -ne 0 ]; then
		exit 1
	fi

	cd Localization/
	
	if [ $# -ne 0 ]; then
		make clean
	fi

	make -j 9

	if [ $? -ne 0 ]; then
		exit 1
	fi

	cd ../SpeakerConfiguration/
	
	if [ $# -ne 0 ]; then
		make clean
	fi

	make -j 9

	if [ $? -ne 0 ]; then
		exit 1
	fi

	cd ../Localization3D/

	if [ $# -ne 0 ]; then
		make clean
	fi
	
	make -j 9

	if [ $? -ne 0 ]; then
		exit 1
	fi

	cd ../Server/
	
	if [ $# -ne 0 ]; then
		make clean
	fi
	
	make -j 9

	if [ $? -ne 0 ]; then
		exit 1
	fi

	cd ../Client/
	
	if [ $# -ne 0 ]; then
		make clean
	fi
	
	make -j 9

	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	cd ../
}

function run {
	#./GoertzelLocalization 2 172.25.9.27 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.200 172.25.13.250 <-- våning 3 testrum
	#./GoertzelLocalization 2 172.25.14.33 172.25.13.125 172.25.9.248 <-- våning 3 öppen yta
	#./GoertzelLocalization 2 172.25.9.27 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.250 172.25.15.12 172.25.14.27 <-- just nu våning 3
	#./GoertzelLocalization 2 172.25.45.134 172.25.45.70 172.25.45.157 172.25.45.220 172.25.45.222 172.25.45.152 172.25.45.141 172.25.45.245 <-- j0
	
	cd bin/
	until ./GoertzelLocalization -p 2 -er 0 -f ../speaker_ips; do echo "Running again"; sleep 1; done
	cd ../
}

if [ $# -eq 0 ]; then
	build
else
	if [ $1 = "clean" ]; then
		build clean
	else
		build
		run
	fi
fi