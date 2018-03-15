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
	
	cd ../Localization3D/
	make -j 9
	
	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	cd ../Server/
	make -j 9
	
	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	cd ../Client/
	make -j 9
	
	if [ $? -ne 0 ]; then
		exit 1
	fi
	
	#./GoertzelLocalization 2 172.25.9.27 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.200 172.25.13.250 <-- våning 3 testrum
	#./GoertzelLocalization 2 172.25.14.33 172.25.13.125 172.25.9.248 <-- våning 3 öppen yta
	#./GoertzelLocalization 2 172.25.9.27 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.250 172.25.15.12 172.25.14.27 <-- just nu våning 3
	#./GoertzelLocalization 2 172.25.45.134 172.25.45.70 172.25.45.157 172.25.45.220 172.25.45.222 172.25.45.152 172.25.45.141 172.25.45.245 <-- j0
	
	cd ../bin/
	until ./GoertzelLocalization 2 172.25.9.38 172.25.13.200; do echo "Running again"; sleep 1; done
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

cd ../Localization3D/
make clean
make -j 9

if [ $? -ne 0 ]; then
	exit 1
fi

cd ../Server/
make clean
make -j 9

if [ $? -ne 0 ]; then
	exit 1
fi

cd ../Client/
make clean
make -j 9

if [ $? -ne 0 ]; then
	exit 1
fi