MIC_VOLUME=$(amixer -c1 sget 'Capture')
LINE_VOLUME=$(amixer -c1 sget 'Headphone')

rm speakersettings
echo "0: " $LINE_VOLUME >> speakersettings
echo "1: " $MIC_VOLUME >> speakersettings