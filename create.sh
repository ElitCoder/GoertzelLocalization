make clean
make -j 9

cd bin/
until ./GoertzelLocalization 2 172.25.9.27 172.25.9.38 172.25.12.99 172.25.12.168 172.25.13.200 172.25.13.250; do echo "Running again"; sleep 2; done
cd ../