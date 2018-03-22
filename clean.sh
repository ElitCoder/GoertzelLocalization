cd DistanceCalculation/
make clean
cd ../Localization/
rm -rf live_localization.txt
make clean
cd ../dependencies/
./clean.sh
cd ../Localization3D/
rm -rf live_localization.txt
make clean
cd ../Server/
make clean
cd ../Client/
make clean