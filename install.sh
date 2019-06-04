#!/bin/sh

if [ ! $(id -u) = "0" ]; then
	echo "Please run using 'sudo ./install.sh'"
	exit 1
fi

BASE_PATH=$(dirname $(readlink -f $0))

apt install python3-pip libjack-jackd2-dev
pip3 install python-rtmidi python-osc
install -v -m 644 $BASE_PATH/orac-bridge/99-orac-ctl-bridge.rules /etc/udev/rules.d/
install -v -m 644 $BASE_PATH/orac-bridge/orac-ctl-bridge.service /usr/lib/systemd/system/
install -v -m 755 $BASE_PATH/orac-bridge/OracCtlBridge.py /usr/local/bin/
systemctl daemon-reload
udevadm control --reload
