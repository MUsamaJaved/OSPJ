#!/bin/bash

while true; do
	./sleep_until_modified.sh ".*\.(md|latex)|Makefile" || break
   	make
done

