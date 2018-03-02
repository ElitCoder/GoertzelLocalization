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

if [ $# -eq 0 ]; then
	exit 0
fi

cd ../bin/
until valgrind ./GoertzelLocalization 2 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.250; do echo "Running again"; sleep 1; done
cd ../

#./GoertzelLocalization 2 172.25.9.27 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.200 172.25.13.250