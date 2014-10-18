/*
 * Copyright © 1999, Be Incorporated.  All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License
 * (see LICENSE in this directory).
 *
 * The DragonDrop window object, a sub-class of BWindow.
 */

#ifndef Dragon_WINDOW_H
#define Dragon_WINDOW_H

#include <interface/Window.h>

class DragonView;
class DragonMenu;

class DragonWindow : public BWindow
{
public:
	// Constructor/destructor
	DragonWindow( void );
	~DragonWindow( void );

	// Messaging
	virtual void MessageReceived( BMessage *msg );
	virtual bool QuitRequested( void );

	// Called when we've got a new image
	void NewImage( void );

private:
	DragonView *_view;
	DragonMenu *_menu;
};

#endif
