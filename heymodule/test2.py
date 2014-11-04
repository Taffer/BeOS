#! /bin/env python

from BeOS.hey import Hey, Specifier

app = Hey("StyledEdit")
win_list = app.Get("Windows")

for win in win_list:
	[title] = win.Get("Title")
	print "Window found:", title
	suite = win.Get("Suites")
	print "Window suite:", suite
