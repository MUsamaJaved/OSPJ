#!/bin/bash

while true; do
	./sleep_until_modified.sh ".*\.(tex)|Makefile" || break
   	make
done

