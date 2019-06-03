#!/bin/sh

if [ ! "$EUID" = "0"]; then
	echo "Please run using 'sudo ./install.sh'"
	exit 1
fi

BASE_PATH=$(dirname $(readlink -f $0))

apt install python3-pip libjack-jackd2-dev
pip3 install python-rtmidi python-osc
install -m 644 $BASE_PATH/orac-bridge/99-orac-ctl-bridge.rules /etc/udev/rules.d/
install -m 644 $BASE_PATH/orac-bridge/orac-ctl-bridge.service /usr/lib/systemd/system/
systemctl daemon-reload
