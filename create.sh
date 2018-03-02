if [ $# -ne 0 ]; then
	make -j 9
	
	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	cd Localization/
	make -j 9
	
	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	cd ../SpeakerConfiguration/
	make -j 9
	
	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	#./GoertzelLocalization 2 172.25.9.27 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.200 172.25.13.250
	
	cd ../bin/
	until ./GoertzelLocalization 2 172.25.9.27 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.200 172.25.13.250; do echo "Running again"; sleep 1; done
	cd ../
	
	exit 0
fi

make clean
make -j 9

if [ $? -ne 0 ]; then
	exit 1
fi

cd Localization/
make clean
make -j 9

if [ $? -ne 0 ]; then
	exit 1
fi

cd ../SpeakerConfiguration/
make clean
make -j 9

if [ $? -ne 0 ]; then
	exit 1
fi