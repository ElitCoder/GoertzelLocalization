make clean
cd Localization/
rm -rf live_localization.txt
make clean
cd ../dep/
./clean.sh
cd ../SpeakerConfiguration/
make clean
cd ../Localization3D/
rm -rf live_localization.txt
make clean
cd ../Server/
make clean
cd ../Client/
make clean
exit