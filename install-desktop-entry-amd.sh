DESKTOP_FILE=NotClassic.desktop
GAME_DIR=`pwd`

rm $DESKTOP_FILE

if [ -f "NCicon.png" ]
then
  echo "NCicon.png exists already. Skipping download."
else
  echo "NCicon.png doesn't exist. Attempting to download it.."
  wget "https://raw.githubusercontent.com/CutieQueenXZ/NotClassic/refs/heads/main/NCicon.png"
fi

echo 'Creating NotClassic.desktop..'
cat >> $DESKTOP_FILE << EOF
[Desktop Entry]
Type=Application
Comment=Minecraft Classic inspired sandbox game
Name=NotClassic
Exec=env DRI_PRIME=1 $GAME_DIR/NotClassic  
Icon=$GAME_DIR/NCicon.png
Path=$GAME_DIR
Terminal=false
Categories=Game;
Actions=singleplayer;resume;

[Desktop Action singleplayer]
Name=Start singleplayer
Exec=env DRI_PRIME=1 $GAME_DIR/NotClassic --singleplayer

[Desktop Action resume]
Name=Resume last server
Exec=env DRI_PRIME=1 $GAME_DIR/NotClassic --resume
EOF
chmod +x $DESKTOP_FILE

echo 'Installing NotClassic.desktop..'
sudo desktop-file-install --dir=/usr/share/applications NotClassic.desktop
sudo update-desktop-database /usr/share/applications
