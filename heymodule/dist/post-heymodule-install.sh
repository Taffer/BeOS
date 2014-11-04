#!/bin/sh
#
# Post heymodule install script to display the docs.

if [ -d /boot/apps/heymodule-1.0 ] ; then
	if [ "$(alert "Found heymodule 1.0… you don't need it anymore.  Move it to the Trash?" "Yes" "No")" = "Yes" ] ; then
		mv /boot/apps/heymodule-1.0 /boot/home/Desktop/Trash
	fi
fi

if [ -e /boot/apps/heymodule-1.1/README.html ] ; then
	/boot/apps/NetPositive /boot/apps/heymodule-1.1/README.html
fi

exit 0
