#!/bin/sh
#
# Print the titles of windows owned by the specified application.

# Check to see if the user specified an application; if they didn't,
# or they asked for help, display a usage message and bail.

if [ "$1" = "" ]       || \
   [ "$1" = "-?" ]     || \
   [ "$1" = "-h" ]     || \
   [ "$1" = "--help" ] ;  then
    echo "usage:"
    echo "gettitles.sh application_name"
    echo
    echo "application_name is the application's name, as seen in the"
    echo "Deskbar, or the application's signature, as seen in FileTypes"
    exit 1
fi

# A little function to help us get the title from hey's output.

get_title()
{
    # Get the full output of hey.
    t="$(hey "$1" get Title of Window $2)"
    
    # Strip out everything but the result line.
    t="$(echo $t | egrep "result")"
    
    # Strip out everything but the title.
    t="$(echo $t | awk '{ print $6; }')"

    # Send back the title.
    echo $t
}

x=0
title="$(get_title $1 $x)"
while [ "$title" != "" ] ; do
    echo "Window $x: $title"

    let x+=1

    title="$(get_title $1 $x)"
done
