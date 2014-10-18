/*
 * Copyright © 1999, Be Incorporated.  All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License
 * (see LICENSE in this directory).
 */

#include "DragonWindow.h"
#include "DragonView.h"
#include "DragonMenu.h"

#include <interface/Rect.h>
#include <app/Application.h>
#include <null.h>

// ----------------------------------------------------------------------
// Constructor/destructor

DragonWindow::DragonWindow( void )
	: BWindow( BRect( 50, 50, 150, 150 ), "DragonDrop",
			   B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			   B_NOT_ZOOMABLE | B_NOT_RESIZABLE |
			   B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS ),
	  _view( NULL ),
	  _menu( NULL )
{
	// Create the menu for this window.
	_menu = new DragonMenu( this );

	// Create the DragonDrop bitmap view for this window.
	_view = new DragonView;

	AddChild( _view );

	// Shuffle things around so the view doesn't overlap the menu.
	BRect menu_rect = _menu->bar->Bounds();
	_view->MoveTo( 0, menu_rect.Height() + 1.0 );

	// Adjust the size of the window to match its contents.
	NewImage();
}

DragonWindow::~DragonWindow( void )
{
	// Clean up: BWindow automatically destroys our views.
}

// ----------------------------------------------------------------------
// Messaging

void DragonWindow::MessageReceived( BMessage *msg )
{
	switch( msg->what ) {
	// Menu messages
	case MENU_FILE_THRESHOLD:
		{
			float new_threshold = msg->FindFloat( "threshold" );
			_view->SetDragThreshold( new_threshold );
		}
		break;
		
	case MENU_FILE_MAXDRAG:
		{
			BPoint new_maxdrag = msg->FindPoint( "max drag size" );
			_view->SetMaxDragSize( new_maxdrag );
		}
		break;

	case MENU_IMAGE_NONE:
	case MENU_IMAGE_ICE:
	case MENU_IMAGE_OWL:
	case MENU_IMAGE_TOUCAN:
		_view->SetImage( msg->what );
		NewImage();
		break;

	case B_REFS_RECEIVED:
		msg->PrintToStream();
		break;

	default:
		BWindow::MessageReceived( msg );
		break;
	}
}

bool DragonWindow::QuitRequested( void )
{
	// The application can handle closing us.
	be_app->PostMessage( B_QUIT_REQUESTED );
	return false;
}

// ----------------------------------------------------------------------
// What to do when we've got a new image

void DragonWindow::NewImage( void )
{
	BRect menu_rect = _menu->bar->Bounds();

	BRect view_rect = _view->Bounds();
	ResizeTo( view_rect.Width(),
			  menu_rect.Height() + view_rect.Height() + 1.0 );
}
