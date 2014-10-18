/*
 * Copyright © 1999, Be Incorporated.  All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License
 * (see LICENSE in this directory).
 */

#include "DragonView.h"
#include "DragonMenu.h"
#include "DragonApp.h"
#include "DragonStrings.h"
#include "DragonWindow.h"

#include <interface/Bitmap.h>
#include <interface/Screen.h>
#include <interface/Region.h>
#include <translation/TranslationUtils.h>
#include <translation/TranslatorRoster.h>
#include <translation/BitmapStream.h>
#include <storage/Directory.h>
#include <storage/File.h>
#include <storage/Mime.h>
#include <storage/NodeInfo.h>
#include <storage/Path.h>
#include <support/String.h>
#include <null.h>
#include <math.h>
#include <string.h>

// ----------------------------------------------------------------------
// Constructor/destructor

DragonView::DragonView( void )
	: BView( BRect( 0, 0, 50, 50 ), "dragon", B_FOLLOW_NONE, B_WILL_DRAW ),
	  _bits( NULL ),
	  _image_is_png( false ),
	  _bits_mutex( "bitmap mutex", true ),
	  _button_click( 0, 0 ),
	  _button_down( false ),
	  _drag_threshold( 0.0 ),
	  _drag_max_size( 320.0, 240.0 ),
	  _am_dragging( false )
{
	// This is a sort of light yellow color.
	rgb_color Background = { 255, 255, 203, 0 };
	SetViewColor( Background );
}

DragonView::~DragonView( void )
{
	// Lock the bitmap semaphore, then clean up by destroying the bitmap
	// and everything else.

	_bits_mutex.Lock();
	delete _bits;
}

// ----------------------------------------------------------------------
// Attached to a window

void DragonView::AttachedToWindow( void )
{
	// Once we've been attached to a window, we should load the default
	// picture (a nice PNG of a Toucan with an "interesting" alpha channel).
	DragonApp *app = dynamic_cast<DragonApp *>( be_app );

	BBitmap *default_bitmap = BTranslationUtils::GetBitmap( B_RAW_TYPE, "toucan" );
	if( default_bitmap ) {
		_SetNewImage( default_bitmap, 
					  app->rsrc_strings->FindString( RSRC_Toucan ) );
		_image_is_png = true;
	}
}

// ----------------------------------------------------------------------
// Drawing

void DragonView::Draw( BRect rect )
{
	// Draw the current bitmap; use alpha mode if it's a PNG (since they
	// often have an interesting alpha channel).  A "real" image-processing
	// application would be able to decide whether it should use B_OP_ALPHA
	// based on how interesting an image's alpha channel is, or how
	// transparent it is.

	_bits_mutex.Lock();

	if( _image_is_png ) {
		SetDrawingMode( B_OP_ALPHA );
	} else {
		SetDrawingMode( B_OP_OVER );
	}

	DrawBitmap( _bits, rect, rect );

	_bits_mutex.Unlock();
}

BBitmap *DragonView::_MakeNoneImage( void )
{
	// Draw an "empty" bitmap to represent "no image"; we'll use one
	// that tells the user what to do.
	BBitmap *bitmap = new BBitmap( BRect( 0, 0, 319, 199 ),
								   BScreen().ColorSpace(),
								   true );
	BView *view = new BView( bitmap->Bounds(),
							 "not a bitmap",
							 B_FOLLOW_ALL_SIDES, 0 );
	bitmap->AddChild( view );

	DragonApp *app = dynamic_cast<DragonApp *>( be_app );
	
	rgb_color White = { 255, 255, 255, 0 };
	rgb_color Black = { 0, 0, 0, 0 };
	
	bitmap->Lock();

	view->SetLowColor( White );
	view->SetViewColor( White );
	view->SetHighColor( Black );
	view->SetDrawingMode( B_OP_OVER );
	view->FillRect( view->Bounds(), B_SOLID_LOW );

	// Excercise for the reader here:  Read the old newsletter articles
	// about how to use the font metrics to find out how large a font is,
	// then center to font in the window dynamically no matter what font
	// settings the user has.

	view->SetFont( be_plain_font );
	view->MovePenTo( 5, 100 );
	view->DrawString( app->rsrc_strings->FindString( RSRC_Drop_an_image ) );
	view->Sync();
	
	bitmap->Unlock();

	return bitmap;
}

// ----------------------------------------------------------------------
// Change the image to one from the resources

void DragonView::SetImage( uint32 which )
{
	// If we can't lock the window, we have no business executing this
	// code...
	if( !Window()->LockLooper() ) return;

	BBitmap *new_bitmap = NULL;

	// We'll need this for loading strings from the resources.
	DragonApp *app = dynamic_cast<DragonApp *>( be_app );

	switch( which ) {
	case MENU_IMAGE_NONE:
		new_bitmap = _MakeNoneImage();
		_image_is_png = false;

		if( new_bitmap ) {
			_SetNewImage( new_bitmap, 
						  app->rsrc_strings->FindString( RSRC_None ) );
		}
		break;

	case MENU_IMAGE_ICE:
		new_bitmap = BTranslationUtils::GetBitmap( B_RAW_TYPE, "ice" );
		_image_is_png = true;

		if( new_bitmap ) {
			_SetNewImage( new_bitmap, 
						  app->rsrc_strings->FindString( RSRC_Ice ) );
		}
		break;

	case MENU_IMAGE_OWL:
		new_bitmap = BTranslationUtils::GetBitmap( B_RAW_TYPE, "owl" );
		_image_is_png = true;

		if( new_bitmap ) {
			_SetNewImage( new_bitmap, 
						  app->rsrc_strings->FindString( RSRC_Owl ) );
		}
		break;

	case MENU_IMAGE_TOUCAN:
		new_bitmap = BTranslationUtils::GetBitmap( B_RAW_TYPE, "toucan" );
		_image_is_png = true;

		if( new_bitmap ) {
			_SetNewImage( new_bitmap, 
						  app->rsrc_strings->FindString( RSRC_Toucan ) );
		}
		break;

	default:
		break;
	}
		
	Window()->UnlockLooper();

	// Update the window to match the new image's dimensions and redraw.
	DragonWindow *window = dynamic_cast<DragonWindow *>( Window() );
	window->NewImage();
}

// ----------------------------------------------------------------------
// Handle the little details about setting a new image for the view;
// change the internal bitmap, change the window title.

void DragonView::_SetNewImage( BBitmap *bitmap, const char *name )
{
	_bits_mutex.Lock();

	if( _bits ) delete _bits;
	_bits = bitmap;

	ResizeTo( _bits->Bounds().Width(), _bits->Bounds().Height() );

	// If you stashed this in the resources instead of hard-coding it,
	// you could easily change the name of the application without
	// editing a single source file.

	BString new_title( "DragonDrop — " );
	new_title += name;

	// Note that the window is already locked when this gets called.
	Window()->SetTitle( new_title.String() );

	Invalidate();

	_bits_mutex.Unlock();
}

// ----------------------------------------------------------------------
// Drag-n-drop stuff and messaging

void DragonView::MessageReceived( BMessage *msg )
{
	switch( msg->what ) {
	case B_MOVE_TARGET:

		// Drag-n-drop: our bitmap was dropped somewhere, and it wants the
		//              image moved

		_CopyTarget( msg );
		SetImage( MENU_IMAGE_NONE );
		break;

	case B_COPY_TARGET:

		// Drag-n-drop: our bitmap was dropped somewhere, and it wants the
		//              image copied

		_CopyTarget( msg );
		break;

	case B_TRASH_TARGET:

		// Drag-n-drop: our bitmap was dropped somewhere, and it wants the
		//              image deleted

		SetImage( MENU_IMAGE_NONE );
		break;

	case B_SIMPLE_DATA:

		// Drag-n-drop: someone is dropping data onto us; if msg has "refs"
		// it was a file drop.

		if( msg->HasRef( "refs" ) ) {
			_SimpleDataFile( msg );
		} else {
			_SimpleData( msg );
		}
		break;

	case B_MIME_DATA:

		// Drag-n-drop: this is the payload of data we've requested from
		//              a drag message dropped on our view

		_MimeData( msg );
		break;

	case B_REFS_RECEIVED:
		msg->PrintToStream();
		break;

	default:
		// If it's not one of those, we don't know how to handle it... maybe
		// our parent class does.

		BView::MessageReceived( msg );
		break;
	}
}

// ----------------------------------------------------------------------
// The mouse has been pressed somewhere... this could be the start of
// a drag-n-drop operation.

void DragonView::MouseDown( BPoint where )
{
	BMessage *current_msg = Window()->CurrentMessage();
	uint32 buttons = current_msg->FindInt32( "buttons" );
	uint32 mod_keys = current_msg->FindInt32( "modifiers" );

	// We're only going to allow a drag when the primary mouse button is
	// down, and no modifiers are down.  If a different button is being
	// used or any of the modifiers are pressed, we return without doing
	// anything.
	
	if( buttons != B_PRIMARY_MOUSE_BUTTON || mod_keys != 0 ) return;

	// Ok, the mouse is down, so we need to take note of the click location;
	// this will let us check for dragging movement in our MouseMoved().

	_button_down = true;
	_button_click = where;

	// Turn on the event mask for all pointer events, so we'll know when
	// the user lets go of the mouse button.

	if( SetMouseEventMask( B_POINTER_EVENTS, 0 ) != B_OK ) {
		// That's bad... but unlikely.
		return;
	}
}

// ----------------------------------------------------------------------
// If the mouse has been released, we're no longer dragging...

void DragonView::MouseUp( BPoint where )
{
	_button_down = false;
	_am_dragging = false;
}

// ----------------------------------------------------------------------
// Track the mouse to see if the user is starting a drag; this takes into
// account the drag threshold.

void DragonView::MouseMoved( BPoint where, uint32 code, const BMessage *msg )
{
	// If we're already dragging, we don't care.

	if( _am_dragging ) return;

	// If the primary mouse button isn't down, or the mouse is just entering
	// the view, we just return... no drag is starting.

	if( !_button_down || code == B_ENTERED_VIEW ) return;

	// If the mouse has travelled "far enough", we start a drag.  In this
	// case, "far enough" is measured by the number of pixels the user's
	// chosen in the "Drag Threshold" submenu of the "File" menu.
	//
	// A real application would probably have a slider for picking the
	// threshold values.

	float dx = fabs( where.x - _button_click.x );
	float dy = fabs( where.y - _button_click.y );
	
	if( dx >= _drag_threshold || dy >= _drag_threshold ) {
		// Start a drag!
		
		_bits_mutex.Lock();

		_am_dragging = true;

		// Create our drag message.

		BMessage *drag_message = _MakeDragMessage();

		// Create a new bitmap or rectangle (if the bitmap would be too
		// large) to drag around.

		BRect drag_rect( _bits->Bounds() );
		BBitmap *drag_bitmap = _MakeDragBitmap();

		_bits_mutex.Unlock();

		if( drag_bitmap ) {
			DragMessage( drag_message, drag_bitmap, B_OP_ALPHA, where, this );
		} else {
			DragMessage( drag_message, drag_rect, this );
		}
		
		delete drag_message;
	}
}

// ----------------------------------------------------------------------
// Set the drag threshold

void DragonView::SetDragThreshold( float thresh )
{
	_drag_threshold = thresh;
}

// ----------------------------------------------------------------------
// Set the max image size for dragging

void DragonView::SetMaxDragSize( BPoint size )
{
	_drag_max_size = size;
}

// ----------------------------------------------------------------------
// Make a drag message

BMessage *DragonView::_MakeDragMessage( void )
{
	DragonApp *app = dynamic_cast<DragonApp *>( be_app );

	BMessage *msg = new BMessage( B_SIMPLE_DATA );

	// We can handle any image type that's supported by the currently
	// installed Translators.
	//
	// A "real" application would want to add the Translators in order,
	// from best to worst using the quality and capability data in the
	// Translator information structure.  It would also want to make sure
	// that there weren't any duplicate types in the drag message.
	
	BTranslatorRoster *translators = BTranslatorRoster::Default();
	translator_id *all_translators = NULL;
	int32 num_translators = 0;
	
	status_t retval = translators->GetAllTranslators( &all_translators,
													  &num_translators );
	if( retval == B_OK ) {
		// Only add translators that support appropriate inputs/outputs.
		
		for( int32 idx = 0; idx < num_translators; idx++ ) {
			const translation_format *in_formats = NULL;
			int32 num_in = 0;
			
			// Get the list of input formats for this Translator.
			retval = translators->GetInputFormats( all_translators[idx],
												   &in_formats,
												   &num_in );
			if( retval != B_OK ) continue;
			
			// Make sure it supports BBitmap inputs.
			
			for( int32 in = 0; in < num_in; in++ ) {
				if( !strcmp( in_formats[in].MIME, "image/x-be-bitmap" ) ) {
					// Add this translator's output formats to the message.
					
					const translation_format *out_formats = NULL;
					int32 num_out = 0;
					
					retval = translators->GetOutputFormats( all_translators[idx],
															&out_formats,
															&num_out );
					if( retval != B_OK ) break;
					
					for( int32 out = 0; out < num_out; out++ ) {
						// Add every type except "image/x-be-bitmap",
						// which won't be of any use to us.

						if( strcmp( out_formats[out].MIME, "image/x-be-bitmap" ) ) {
							msg->AddString( "be:types", 
											out_formats[out].MIME );
							msg->AddString( "be:filetypes", 
											out_formats[out].MIME );
							msg->AddString( "be:type_descriptions",
											out_formats[out].name );
						}
					}
				}
			}
		}
	}

	// We can also handle raw data.

	msg->AddString( "be:types", B_FILE_MIME_TYPE );
	msg->AddString( "be:filetypes", B_FILE_MIME_TYPE );
	msg->AddString( "be:type_descriptions", 
					app->rsrc_strings->FindString( RSRC_Raw_Data ) );

	// Add the actions that we'll support.  B_LINK_TARGET doesn't make much
	// sense in this context, so we'll leave it out.  B_MOVE_TARGET is a
	// B_COPY_TARGET followed by B_TRASH_TARGET...

	msg->AddInt32( "be:actions", B_COPY_TARGET );
	msg->AddInt32( "be:actions", B_TRASH_TARGET );
	msg->AddInt32( "be:actions", B_MOVE_TARGET );

	// A file name for dropping onto things (like the Tracker) that create
	// files.
	
	msg->AddString( "be:clip_name", "Dropped Bitmap" );

	return msg;
}

// ----------------------------------------------------------------------
// Make a drag bitmap

BBitmap *DragonView::_MakeDragBitmap( void )
{
	// If the currently displayed bitmap is too large to drag around,
	// we'll just drag around a rectangle.

	BRect drag_rect = _bits->Bounds();

	if( drag_rect.Width() > _drag_max_size.x ||
		drag_rect.Height() > _drag_max_size.y ) return NULL;

	// If we've got a PNG image, we'll assume that it's got
	// "interesting" alpha information.  The ones that are built
	// into DragonDrop's resources all have "interesting" alpha
	// channels.
			
	if( _image_is_png ) {
		BBitmap *drag_me = new BBitmap( _bits );
		memcpy( drag_me->Bits(), _bits->Bits(), _bits->BitsLength() );
		
		return drag_me;
	}

	// If you've made it here, we'll need to build a semi-transparent image
	// to drag around.  This magic is from Pavel Cisler, and it ensures that
	// you've got a drag bitmap that's translucent.
	
	BRect rect( _bits->Bounds() );
	BBitmap *bitmap = new BBitmap( rect, B_RGBA32, true );
	BView *view = new BView( rect, "drag view", B_FOLLOW_NONE, 0 );

	bitmap->Lock();
	bitmap->AddChild( view );
	
	BRegion new_clip;
	new_clip.Set( rect );
	view->ConstrainClippingRegion( &new_clip );
	
	view->SetHighColor( 0, 0, 0, 0 );
	view->FillRect( rect );
	view->SetDrawingMode( B_OP_ALPHA );
	
	view->SetHighColor( 0, 0, 0, 128 );
	view->SetBlendingMode( B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE );
	
	view->DrawBitmap( _bits );
	
	view->Sync();
	
	bitmap->Unlock();

	return bitmap;
}

// ----------------------------------------------------------------------
// Copy the bitmap to our drop target

void DragonView::_CopyTarget( BMessage *msg )
{
	// Try to translate the data.  If we can't manage to translate it, 
	// there's no point in continuing.

	char *translated_mime;
	BMallocIO *data_buffer = _TranslateBitmap( msg, &translated_mime );

	if( data_buffer == NULL ) {
		// Couldn't translate... d'oh!
		msg->SendReply( B_MESSAGE_NOT_UNDERSTOOD, this );
		return;
	}

	const char *type = msg->FindString( "be:types" );
	if( strcmp( type, B_FILE_MIME_TYPE ) ) {
		// It doesn't match B_FILE_MIME_TYPE, so they actually want some
		// data passed in a message... let's give it to them.

		BMessage data_reply( B_MIME_DATA );
		data_reply.AddData( translated_mime, B_MIME_TYPE,
							data_buffer->Buffer(),
							data_buffer->BufferLength() );

		msg->SendReply( &data_reply, this );
	} else {
		// If you're here, it's because the drop target wants us to create a
		// file.
		entry_ref dir_ref;

		if( msg->FindRef( "directory", &dir_ref ) != B_OK ) {
			// Something bad has happened; the message is corrupt or 
			// incomplete.
			msg->SendReply( B_MESSAGE_NOT_UNDERSTOOD, this );

			// Lots of people are against goto in "real" code; this is one
			// of the few good ways to use it... since we're using C++, we
			// could replace this with an exception, and clean up when we
			// catch the exception, but that introduces unecessary overhead
			// here... if we were using exceptions all over the program it
			// would make more sense.
			//
			// See Scott Meyers's More Effective C++ for a discussion about
			// the overhead introduced by exceptions.
			goto cleanup_and_return;
		}

		// See if we can get a BFile in the specified location
		const char *filename = msg->FindString( "name" );

		BDirectory bit_dir;
		if( bit_dir.SetTo( &dir_ref ) != B_OK ) {
			msg->SendReply( B_MESSAGE_NOT_UNDERSTOOD, this );

			goto cleanup_and_return;
		}

		BFile bit_file;
		if( bit_file.SetTo( &bit_dir, filename, 
							B_WRITE_ONLY | B_CREATE_FILE ) != B_OK ) {
			msg->SendReply( B_MESSAGE_NOT_UNDERSTOOD, this );

			goto cleanup_and_return;
		}

		// Write the data...
		ssize_t wrote_bytes = bit_file.Write( data_buffer->Buffer(),
											  data_buffer->BufferLength() );

		if( wrote_bytes != static_cast<ssize_t>( data_buffer->BufferLength() ) ) {
			// Then we didn't write all of the data. 
			msg->SendReply( B_MESSAGE_NOT_UNDERSTOOD, this );

			goto cleanup_and_return;
		}
		
		// We'll also be nice and set the file's MIME type to the type of
		// the translated data.
		BNodeInfo node_info( &bit_file );
		(void)node_info.SetType( translated_mime );
	}

cleanup_and_return:
	delete [] translated_mime;
	delete data_buffer;

	return;
}

// ----------------------------------------------------------------------
// Handle B_SIMPLE_DATA messages

void DragonView::_SimpleData( BMessage *msg )
{
	// We only understand B_COPY_TARGET actions, so if there's no
	// B_COPY_TARGET in the drag message, we'll let the sender know we
	// didn't understand.
	
	int32 action = 0;
	int32 idx = 0;

	while( msg->FindInt32( "be:actions", idx++, &action ) == B_OK ) {
		if( action == B_COPY_TARGET ) break;
	}

	if( action != B_COPY_TARGET ) {
		msg->SendReply( B_MESSAGE_NOT_UNDERSTOOD, this );
		return;
	}

	// Pick a MIME type for the data we'll be receiving; we'll use the
	// first type supported by the installed Translators that isn't
	// B_FILE_MIME_TYPE.

	int type_idx = 0;
	const char *target_mime = msg->FindString( "be:types", type_idx );

	BTranslatorRoster *roster = BTranslatorRoster::Default();

	while( target_mime ) {
		if( strcmp( target_mime, B_FILE_MIME_TYPE ) &&
			strcmp( target_mime, "image/x-be-bitmap" ) ) {
			// Check to see if there's a translator that can handle this
			// type by attempting to find the Translation Kit type code
			// that matches this type.
			
			uint32 code = _Mime2TypeCode( roster, target_mime );

			if( code != 0 ) {
				// Found one.

				BMessage reply( B_COPY_TARGET );
				reply.AddString( "be:types", target_mime );

				// Send the reply!
				msg->SendReply( &reply, this );
				return;
			}
		}

		type_idx++;
		target_mime = msg->FindString( "be:types", type_idx );
	}

	// If you got here, we couldn't find a type we liked.
	msg->SendReply( B_MESSAGE_NOT_UNDERSTOOD, this );
}

// ----------------------------------------------------------------------
// Handle B_SIMPLE_DATA messages that are file drops

void DragonView::_SimpleDataFile( BMessage *msg )
{
	// Loading a bitmap from a file using the Translation Kit is very
	// easy, courtesy of BTranslationUtils.
	//
	// We loop through the refs in the message in case several files were
	// dropped on us.  If that happens, we'll end up displaying the last
	// one.  A real program would probably want to open new windows (or
	// some other container) for the dropped files.

	entry_ref ref;
	int32 idx = 0;
	while( msg->FindRef( "refs", idx++, &ref ) == B_OK ) {
		// Convert the entry_ref into a filename.
		BEntry ent( &ref, true );
		BPath path( &ent );

		// Load the bitmap via the Translation Kit
		BBitmap *new_bits = BTranslationUtils::GetBitmapFile( path.Path() );

		// Update the window contents
		if( Window()->LockLooper() ) {
			DragonApp *app = dynamic_cast<DragonApp *>( be_app );

			_SetNewImage( new_bits, app->rsrc_strings->FindString( RSRC_Dropped ) );

			dynamic_cast<DragonWindow *>( Window() )->NewImage();
			Window()->UnlockLooper();
		}
	}
}

// ----------------------------------------------------------------------
// Handle B_MIME_DATA messages

void DragonView::_MimeData( BMessage *msg )
{
	// Get the name of the B_MIME_TYPE data in this message; it'll be the
	// MIME type of the data.
	char *name;
	type_code type;
	int32 count;
	if( msg->GetInfo( B_MIME_TYPE, 0,
					  &name, &type, &count ) != B_OK ) {
		msg->SendReply( B_MESSAGE_NOT_UNDERSTOOD, this );
		return;
	}
	
	// Now that we know the name of the data, we can get
	// it out and pass it to the BTranslationUtils...
	ssize_t size;
	const void *data;
	if( msg->FindData( name, B_MIME_TYPE, 0,
					   &data, &size ) != B_OK ) {
		msg->SendReply( B_MESSAGE_NOT_UNDERSTOOD, this );
		return;
	}

	// Put the data into a BPositionIO (BMemoryIO is a BPositionIO on top
	// of a chunk of memory) object so the Translation Kit can process it.
	BMemoryIO in_buff( data, size );

	// Now we can convert it to a BBitmap.  Yes, it really is this easy.
	BBitmap *new_bits = BTranslationUtils::GetBitmap( &in_buff );

	// Update the window with the new image.
	if( Window()->LockLooper() ) {
		DragonApp *app = dynamic_cast<DragonApp *>( be_app );

		_SetNewImage( new_bits, app->rsrc_strings->FindString( RSRC_Dropped ) );
		if( !strcasecmp( name, "image/png" ) ) _image_is_png = true;
		dynamic_cast<DragonWindow *>( Window() )->NewImage();
		Window()->UnlockLooper();
	}
}

// ----------------------------------------------------------------------
// Translate our bitmap into one of the formats requested.
//
// We keep trying translations until:
// - one succeeds
// - we run out of requested types
//
// Will return NULL (and type_out will be NULL) if we can't successfully
// translate, otherwise it'll return a BMallocIO containing the translated
// data, and type_out will contain the MIME type of translated data.
//
// The caller owns the returned BMallocIO and the C string returned in
// type_out; delete these when you're done with them.

BMallocIO *DragonView::_TranslateBitmap( BMessage *msg, char **type_out )
{
	// We'll need to access the Translation Kit's BTranslatorRoster...

	BTranslatorRoster *roster = BTranslatorRoster::Default();
	
	// We'll start by looking through be:types; if we need to look in
	// be:filetypes, we'll reset these.

	int type_idx = 0;
	const char *field_name = "be:types";

	const char *target_mime = msg->FindString( field_name, type_idx );
	if( !strcasecmp( target_mime, B_FILE_MIME_TYPE ) ) {
		// If target_mime == B_FILE_MIME_TYPE then we should be checking
		// in be:filetypes... this will end up being written to a file.

		field_name = "be:filetypes";
		target_mime = msg->FindString( field_name, type_idx );
	}

	while( target_mime ) {
		// Can any of the Translators give us data in this type?  First
		// we'll need to convert the MIME type to a Translation Kit type
		// code.

		uint32 target_code = _Mime2TypeCode( roster, target_mime );

		if( target_code != 0 ) {
			// We've found a translation type; let's try translating the
			// data.

			_bits_mutex.Lock();

			BMallocIO *bit_buff = new BMallocIO;
			BBitmapStream bit_stream( _bits );

			status_t retval = roster->Translate( &bit_stream, NULL, NULL,
												 bit_buff, target_code );

			BBitmap *detached;
			(void)bit_stream.DetachBitmap( &detached );

			_bits_mutex.Unlock();

			if( retval == B_OK ) {
				// Successful translation!  Woo-hoo!
				//
				// Make a copy of the MIME type we translated to, and then
				// send back the data.

				*type_out = new char[strlen( target_mime ) + 1];
				strcpy( *type_out, target_mime );

				return bit_buff;
			} else {
				delete bit_buff;
			}
		}

		// That didn't work... let's try the next one.

		type_idx++;
		target_mime = msg->FindString( field_name, type_idx );
	}

	// If you got here, we didn't manage to translate the data.  Dang.

	*type_out = NULL;
	return NULL;
}

// ----------------------------------------------------------------------
// Convert a MIME type to a Translation Kit type code, if one matches.
//
// This is almost directly from the Translation Kit documentation.

uint32 DragonView::_Mime2TypeCode( BTranslatorRoster *roster, 
								   const char *mime )
{
	translator_id *translators;
	int32 num_translators;

	if( roster->GetAllTranslators( &translators, &num_translators ) != B_OK ) return 0;

	for( int32 i = 0; i < num_translators; i++ ) {
		const translation_format *fmts;
		int32 num_fmts;

		if( roster->GetOutputFormats( translators[i], &fmts, &num_fmts ) != B_OK ) continue;

		for( int32 j = 0; j < num_fmts; j++ ) {
			if( !strcasecmp( fmts[j].MIME, mime ) ) return fmts[j].type;
		} 
	} 

	return 0;
}
