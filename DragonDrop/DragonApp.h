/*
 * Copyright © 1999, Be Incorporated.  All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License
 * (see LICENSE in this directory).
 *
 * The DragonDrop application object, a sub-class of BApplication.
 */

#ifndef Dragon_APP_H
#define Dragon_APP_H

#include <app/Application.h>
#include <storage/ResourceStrings.h>

class DragonWindow;

class DragonApp : public BApplication
{
public:
	DragonApp( void );
	virtual ~DragonApp( void );
	
	// Starting and stopping
	virtual void ReadyToRun( void );
	virtual bool QuitRequested( void );
	
	// Messages
	virtual void AboutRequested( void );
	
	virtual void MessageReceived( BMessage *msg );
	virtual void RefsReceived( BMessage *msg );

	// We load all of our strings from resources; it's good to keep
	// this around in an accessible place.
	BResourceStrings *rsrc_strings;

private:
	DragonWindow *_wind;
};

#endif
