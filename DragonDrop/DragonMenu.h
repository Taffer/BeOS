/*
 * Copyright © 1999, Be Incorporated.  All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License
 * (see LICENSE in this directory).
 *
 * This class just builds a menu bar for us and keeps track of it; this
 * could easily be absorbed into the DragonWindow class.  Exercise for the
 * reader.
 */

#ifndef Dragon_MENU_H
#define Dragon_MENU_H

#include <interface/MenuBar.h>

class BWindow;

// Menu message constants.
enum {
	// File menu
	MENU_FILE_THRESHOLD = 'mnFt',	// targetted at the window
	MENU_FILE_MAXDRAG   = 'mnFm',	// targetted at the window

	// Image menu
	MENU_IMAGE_NONE   = 'mnIn',		// these are targetted at the window
	MENU_IMAGE_ICE    = 'mnIi',
	MENU_IMAGE_OWL    = 'mnIo',
	MENU_IMAGE_TOUCAN = 'mnIt',

	// Help menu
	MENU_HELP_HOW = 'mnHh'			// targetted at the application
};

class DragonMenu
{
public:
	DragonMenu( BWindow *owner );
	~DragonMenu( void );

	BMenuBar *bar;
};

#endif
