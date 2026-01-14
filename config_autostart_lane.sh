#!/bin/bash

echo "copy logo.ico"
cp logo.ico /home/$LOGNAME/
echo "copy Lane.desktop"
sudo cp Lane.desktop /usr/share/applications/
echo "chmod +x Lane.desktop"
sudo chmod +x /usr/share/applications/Lane.desktop
#增加启动脚本
echo "copy launch_lane.sh"
cp launch_lane.sh /home/$LOGNAME/
echo "chmod +x launch_lane.sh"
chmod +x /home/$LOGNAME/launch_lane.sh
#增加开机启动脚本
if [ ! -d /home/$LOGNAME/.config/autostart ]; then
    echo "mkdir .config/autostart"
    mkdir -p /home/$LOGNAME/.config/autostart
fi
echo "copy autostart_launch_lane.sh.desktop"
cp autostart_launch_lane.sh.desktop /home/$LOGNAME/.config/autostart/
chmod +x /home/$LOGNAME/.config/autostart/autostart_launch_lane.sh.desktop
#去掉sudo密码认证
echo "$LOGNAME ALL=NOPASSWD: ALL" |sudo tee -a /etc/sudoers
echo "config autostart end"
