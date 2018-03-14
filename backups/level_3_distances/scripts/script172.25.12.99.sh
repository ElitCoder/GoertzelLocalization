systemctl stop audio*
arecord -Daudiosource -r 48000 -fS16_LE -c1 -d18 /tmp/cap172.25.12.99.wav &

sleep 7
aplay -Dlocalhw_0 -r 48000 -fS16_LE /tmp/testTone.wav

exit;
