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

(left, top, right, bottom) = old_rect

for foo in range( 5, 105, 5 ):
	# Looks like you need to refresh the specifier every time you want
	# to use it... bug or feature?
	f = x.Specifier( "Frame of Window 0" )
	x.SetRect( f, ( ( left + foo ), ( top + foo ), ( right + foo ), ( bottom + foo ) ) )
	f = x.Specifier( "Frame of Window 0" )
	now_rect = x.Get( f )
	print "rect is now '%s'" % ( now_rect, )
	sleep( 1 )

f = x.Specifier( "Frame of Window 0" )
new_rect = x.Get( f )
print "x.Get( f ) returned '%s'" % ( new_rect, )

x.Quit()

p = Hey( "pe" )

wind = p.Specifier( "Window" )
num = p.Count( wind )

print "Pe has %d windows..." % ( num )

for i in range( num ):
	s = p.Specifier()
	s.Add( "Title" )
	s.Add( "Window", i )

	name = p.Get( s )

	print "Pe's window %d is named '%s'" % ( i, name )
