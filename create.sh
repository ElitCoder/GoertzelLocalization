make clean
make -j 9

if [ $? -ne 0 ]; then
	exit 1
fi

cd Localization/
make clean
make -j 9
cd ../

if [ $? -ne 0 ]; then
	exit 1
fi

cd bin/
./GoertzelLocalization 2 172.25.9.27 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.200 172.25.13.250
#until valgrind ./GoertzelLocalization 2 172.25.9.27 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.200 172.25.13.250; do echo "Running again"; sleep 1; done
cd ../

#cd Localization/
#./Localization < live_localization.txt
#cd ../

# ./GoertzelLocalization 2 172.25.9.27 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.200 172.25.13.250