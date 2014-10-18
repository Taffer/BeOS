/*
 * Copyright © 1999, Be Incorporated.  All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License
 * (see LICENSE in this directory).
 */

/*
MenuBar

- Menu "File"
  - Menu "Drag Threshold"
    - MenuItem "0 pixels": target owner (default)
    - MenuItem "1 pixel": target owner
    - MenuItem "2 pixels": target owner
    - MenuItem "3 pixels": target owner
  - Menu "Max Drag Size"
    - MenuItem "Tiny (80 x 60)": target owner
    - MenuItem "Small (160 x 120)": target owner
    - MenuItem "Medium (320 x 240)": target owner (default)
    - MenuItem "Large (640 x 480)": target owner
  - separator
  - MenuItem "Quit": target be_app

- Menu "Images"
  - MenuItem "Ice": target owner
  - MenuItem "Owl": target owner
  - MenuItem "Toucan": target owner
  - separator
  - MenuItem "None": target owner

- Menu "Help"
  - MenuItem "How to…": target be_app
  - separator
  - MenuItem "About DragonDrop…": target be_app
*/

#include "DragonMenu.h"
#include "DragonStrings.h"
#include "DragonApp.h"

#include <interface/Window.h>
#include <interface/Menu.h>
#include <interface/MenuItem.h>

// ----------------------------------------------------------------------
// Constructor and destructor

DragonMenu::DragonMenu( BWindow *owner )
{
	// Build the menu bar, and add it to the owner window.

	// We'll need this for accessing the string resources.
	DragonApp *app = dynamic_cast<DragonApp *>( be_app );

	// The menu bar
	bar = new BMenuBar( owner->Bounds(), "menu bar" );

	// ----------------------------------------------------------------------
	// File menu
	BMenu *a_menu = new BMenu( app->rsrc_strings->FindString( RSRC_File ) );

	// File: Drag Threshold menu
	BMenu *sub_menu = new BMenu( app->rsrc_strings->FindString( RSRC_Threshold ) );
	sub_menu->SetRadioMode( true );

	// File: Drag Threshold: 0 pixels item
	BMessage threshold( MENU_FILE_THRESHOLD );
	threshold.AddFloat( "threshold", 0.0 );
	BMenuItem *an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_0_Pixels ),
										new BMessage( &threshold ) );
	an_item->SetTarget( owner );
	an_item->SetMarked( true );		// default
	sub_menu->AddItem( an_item );

	// File: Drag Threshold: 1 pixel item
	threshold.ReplaceFloat( "threshold", 1.0 );
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_1_Pixel ),
							 new BMessage( &threshold ) );
	an_item->SetTarget( owner );
	sub_menu->AddItem( an_item );

	// File: Drag Threshold: 2 pixels item
	threshold.ReplaceFloat( "threshold", 2.0 );
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_2_Pixels ),
							 new BMessage( &threshold ) );
	an_item->SetTarget( owner );
	sub_menu->AddItem( an_item );

	// File: Drag Threshold: 3 pixels item
	threshold.ReplaceFloat( "threshold", 3.0 );
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_3_Pixels ),
							 new BMessage( &threshold ) );
	an_item->SetTarget( owner );
	sub_menu->AddItem( an_item );

	a_menu->AddItem( sub_menu );

	// File: Maximum Drag Size menu
	sub_menu = new BMenu( app->rsrc_strings->FindString( RSRC_Max_Drag_Size ) );
	sub_menu->SetRadioMode( true );
	
	// File: Maximum Drag Size: Tiny item
	BMessage max_drag( MENU_FILE_MAXDRAG );
	max_drag.AddPoint( "max drag size", BPoint( 80, 60 ) );
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_Tiny_drag ),
							 new BMessage( &max_drag ) );
	an_item->SetTarget( owner );
	sub_menu->AddItem( an_item );
	
	// File: Maximum Drag Size: Small item
	max_drag.ReplacePoint( "max drag size", BPoint( 160, 120 ) );
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_Small_drag ),
							 new BMessage( &max_drag ) );
	an_item->SetTarget( owner );
	sub_menu->AddItem( an_item );
	
	// File: Maximum Drag Size: Medium item
	max_drag.ReplacePoint( "max drag size", BPoint( 320, 240 ) );
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_Medium_drag ),
							 new BMessage( &max_drag ) );
	an_item->SetTarget( owner );
	an_item->SetMarked( true );		// default
	sub_menu->AddItem( an_item );
	
	// File: Maximum Drag Size: Large item
	max_drag.ReplacePoint( "max drag size", BPoint( 640, 480 ) );
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_Large_drag ),
							 new BMessage( &max_drag ) );
	an_item->SetTarget( owner );
	sub_menu->AddItem( an_item );
	
	a_menu->AddItem( sub_menu );

	// File: separator
	a_menu->AddSeparatorItem();

	// File: Quit item
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_Quit ),
							 new BMessage( B_QUIT_REQUESTED ) );
	an_item->SetTarget( be_app );
	a_menu->AddItem( an_item );

	bar->AddItem( a_menu );

	// ----------------------------------------------------------------------
	// Images menu
	a_menu = new BMenu( app->rsrc_strings->FindString( RSRC_Images ) );
	a_menu->SetRadioMode( true );

	// Images: Ice item
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_Ice ),
							 new BMessage( MENU_IMAGE_ICE ) );
	an_item->SetTarget( owner );
	a_menu->AddItem( an_item );
	
	// Images: Owl item
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_Owl ),
							 new BMessage( MENU_IMAGE_OWL ) );
	an_item->SetTarget( owner );
	a_menu->AddItem( an_item );

	// Images: Toucan item
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_Toucan ),
							 new BMessage( MENU_IMAGE_TOUCAN ) );
	an_item->SetTarget( owner );
	an_item->SetMarked( true );		// default
	a_menu->AddItem( an_item );

	// Images: separator
	a_menu->AddSeparatorItem();

	// Images: None item
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_None ),
							 new BMessage( MENU_IMAGE_NONE ) );
	an_item->SetTarget( owner );
	a_menu->AddItem( an_item );

	bar->AddItem( a_menu );

	// ----------------------------------------------------------------------
	// Help menu
	a_menu = new BMenu( app->rsrc_strings->FindString( RSRC_Help ) );

	// Help: How to… item
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_How_to ),
							 new BMessage( MENU_HELP_HOW ) );
	an_item->SetTarget( be_app );
	a_menu->AddItem( an_item );

	// Help: separator
	a_menu->AddSeparatorItem();

	// Help: About item
	an_item = new BMenuItem( app->rsrc_strings->FindString( RSRC_About_DragonDrop ),
							 new BMessage( B_ABOUT_REQUESTED ) );
	an_item->SetTarget( be_app );
	a_menu->AddItem( an_item );
	
	bar->AddItem( a_menu );

	// Now that that unpleasantness is out of the way, add the menu to the
	// window.
	if( owner->LockLooper() ) {
		owner->AddChild( bar );
		owner->UnlockLooper();
	}
}

DragonMenu::~DragonMenu( void )
{
	// We don't do anything; the window will clean up all of its child
	// views when it quits.
}
