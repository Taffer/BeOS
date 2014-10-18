/*
 * Copyright © 1999, Be Incorporated.  All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License
 * (see LICENSE in this directory).
 */

#include "DragonApp.h"
#include "DragonMenu.h"
#include "DragonStrings.h"
#include "DragonWindow.h"

#include <interface/Alert.h>
#include <null.h>

// ----------------------------------------------------------------------
// Constructor/destructor

DragonApp::DragonApp( void )
	: BApplication( "application/x-vnd.be.dragondrop" ),
	  rsrc_strings( NULL ),
	  _wind( NULL )
{
	// Create the new BResourceStrings object; all of the strings we load
	// from resources with this will stick around until it's destroyed in
	// the DragonApp destructor.

	rsrc_strings = new BResourceStrings;
}

DragonApp::~DragonApp( void )
{
	// Delete the BResourceStrings object; this closes the file and 
	// de-allocates all of the strings we loaded through it.

	delete rsrc_strings;
}

// ----------------------------------------------------------------------
// Starting and stopping

void DragonApp::ReadyToRun( void )
{
	// Create and show the window.

	_wind = new DragonWindow;
	_wind->Show();
}

bool DragonApp::QuitRequested( void )
{
	// Tell our only window to quit, and then exit the application.

	if( _wind->LockLooper() ) {
		_wind->Quit();
	}

	return true;
}

// ----------------------------------------------------------------------
// Messages

void DragonApp::AboutRequested( void )
{
	// Display the "About" dialog.

	BAlert *about = new BAlert( rsrc_strings->FindString( RSRC_About_DragonDrop ),
								rsrc_strings->FindString( RSRC_about_text ),
								rsrc_strings->FindString( RSRC_about_button ),
								NULL,
								NULL,
								B_WIDTH_AS_USUAL,
								B_IDEA_ALERT );
	about->Go();
}

void DragonApp::MessageReceived( BMessage *msg )
{
	// Handle messages we know about:

	switch( msg->what ) {
	case MENU_HELP_HOW:
		// Display the "How to..." dialog.
		{
			BAlert *how = new BAlert( rsrc_strings->FindString( RSRC_How_to ),
									  rsrc_strings->FindString( RSRC_how_to_text ),
									  rsrc_strings->FindString( RSRC_about_button ) );
			how->Go();
		}
		break;

	default:
		// Something else, send it to our parent class in case it knows how
		// to handle this.

		BApplication::MessageReceived( msg );
		break;
	}
}

void DragonApp::RefsReceived( BMessage *msg )
{
	// Handle dropped files
	msg->PrintToStream();
}
