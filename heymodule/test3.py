#! /bin/env python
#
# Test the heymodule.

from BeOS.hey import Hey
from time import sleep

x = Hey( "text/plain" )

f = x.Specifier( "Frame of Window 0" )
t = x.Specifier( "Title of Window 0" )

old_rect = x.Get( f )
print "x.Get( f ) returned '%s'" % ( old_rect, )

a = x.Get( t )
print "x.Get( t ) returned '%s'" % ( a, )

x.Quit()
