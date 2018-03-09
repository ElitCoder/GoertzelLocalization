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