#!/usr/bin/env bash

wallpaperDir="/storage/owncloud/Images/astronomy"

sleepTime=3600

if ! type -p bgswitch >/dev/null;then
	echo "Error: unable to find bgswitch command"
	exit 1
fi

echo "Gathering file list..."
readarray -t wallpapers < <(ls -d -1 --quoting-style=shell-escape-always $wallpaperDir/* | sort -R)

while true;do
	echo "Rotating..."
	for file in ${wallpapers[@]};do
		echo "Setting wallpaper to $file"
		echo bgswitch --all set "$file"
		echo sleep $sleepTime
	done
	exit
done
