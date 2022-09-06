#! /usr/bin/env bash

APPLICATION_NAME="GAD_ToolChain"
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SAFETYNETWEBHOOK=$(cat ${SCRIPTDIR}/safety_net_webhook.txt);
if [ -z "${SAFETYNETWEBHOOK}" ]; then
	echo "Webhook safety net not defined!"
	exit 1;
fi
set -x
if ! grep -wq "OFF\|ON" /home/pi/safety_net/safety_net_active.txt; then
    curl -X POST -H 'Content-type: application/json' --data '{"text":" :warning: :warning: :warning: GAD SAFETY NET ACTIVE FLAG IS NEITHER ON OR OFF - CHECK `/home/pi/GDConcMeasure/safety_net/safety_net_active.txt` FILE :warning: :warning: :warning:"}' ${SAFETYNETWEBHOOK}

elif grep -wq "ON" /home/pi/safety_net/safety_net_active.txt; then
    if [ $(ps aux | grep $APPLICATION_NAME | grep -v sudo | grep -v gdb | grep -v grep | grep -v defunct | wc -l) -lt 1 ]; then
	if [ $(cat /sys/class/gpio/gpio4/value) == 0 ]; then
	    echo "0" > /sys/class/gpio/gpio17/value
	    echo "0" > /sys/class/gpio/gpio15/value
	    echo "0" > /sys/class/gpio/gpio18/value
	    echo "1" > /sys/class/gpio/gpio4/value
	    curl -X POST -H 'Content-type: application/json' --data '{"text":" :warning: :warning: :warning: GAD POWER SUPPLY WAS ON WITHOUT TOOLCHAIN RUNNING -  POWER NOW OFF :warning: :warning: :warning:"}' ${SAFETYNETWEBHOOK}
	fi
	curl -X POST -H 'Content-type: application/json' --data '{"text":" :warning: :warning: :warning: GADToolChain IS NOT RUNNING :warning: :warning: :warning:"}' ${SAFETYNETWEBHOOK}
    fi
fi
