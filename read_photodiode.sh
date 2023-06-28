#!/bin/bash
while [ true ]; do
	HTMLPAGE=$(curl 133.11.177.102:6666)
	if [ $? -ne 0 ]; then
		sleep(3);
	else
		
		#<!DOCTYPE HTML><html><br>ADC counts: 1024<br>Voltage: 3.30V<br>Temperature: 117.19&deg;C<br></html>
	fi
done
