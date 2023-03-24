#!/bin/bash
while [ true ]; do
	./change_sol_valves.sh
	STATE=$(cat /sys/class/gpio/gpio15/value)
	if [ ${STATE} -eq 0 ]; then
		sleep 600
	else
		sleep 300
	fi
done
