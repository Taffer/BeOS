#!/bin/env python
#
# Print the titles of windows owned by the specified application.

import sys
import BeOS.hey

if len( sys.argv ) < 2 or \
   sys.argv[1] == "-?" or \
   sys.argv[1] == "-h" or \
   sys.argv[1] == "--help":
    print "usage:"
    print "gettitles.py application_name"
    print
    print "application_name is the application's name, as seen in the"
    print "Deskbar, or the application's signature, as seen in"
    print "FileTypes"
    sys.exit( 1 )

def get_title( app, count ):
    hey = BeOS.hey.Hey( app )
    specifier = BeOS.hey.Specifier()
    specifier.Add( "Title" )
    specifier.Add( "Window", x )

    try:
        return hey.Get( specifier )
    except:
        return ""

x = 0
title = get_title( sys.argv[1], x )
while title != "":
    print "Window %d: %s" % ( x, title )

    x = x + 1

    title = get_title( sys.argv[1], x )
