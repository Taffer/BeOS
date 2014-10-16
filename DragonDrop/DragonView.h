/*
 * Copyright © 1999, Be Incorporated.  All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License
 * (see LICENSE in this directory).
 *
 * The DragonDrop view object, a sub-class of BView.
 *
 * This is where the action is.
 */

#ifndef Dragon_VIEW_H
#define Dragon_VIEW_H

#include <interface/View.h>
#include <interface/Point.h>
#include <translation/TranslationDefs.h>
#include <support/Locker.h>

class BBitmap;
class BMallocIO;

class DragonView : public BView
{
public:
	DragonView( void );
	~DragonView( void );

	// Attached to a window
	virtual void AttachedToWindow( void );

	// Drawing
	virtual void Draw( BRect rect );

	// Change the image to one of the internal images
	void SetImage( uint32 which );

	// Drag-n-drop stuff
	virtual void MessageReceived( BMessage *msg );

	virtual void MouseDown( BPoint where );
	virtual void MouseUp( BPoint where );
	virtual void MouseMoved( BPoint where, uint32 code, const BMessage *msg );

	// Change the drag threshold
	void SetDragThreshold( float thresh );
	
	// Change the max image size for dragging
	void SetMaxDragSize( BPoint size );

private:
	BBitmap *_bits;
	bool _image_is_png;
	BLocker _bits_mutex;

	BPoint _button_click;
	bool _button_down;

	float _drag_threshold;
	BPoint _drag_max_size;
	
	bool _am_dragging;

	// Display a new bitmap and window title.
	void _SetNewImage( BBitmap *bitmap, const char *name );

	// Create the bitmap that represents "no image".
	BBitmap *_MakeNoneImage( void );

	// Build a drag message and build a drag bitmap.
	BMessage *_MakeDragMessage( void );
	BBitmap *_MakeDragBitmap( void );

	// Handlers for different kinds of messages involved in the drag-n-drop
	// protocol.  We farm these out so the MessageReceived() function
	// doesn't get too huge and messy.
	void _CopyTarget( BMessage *msg );
	void _SimpleData( BMessage *msg );
	void _SimpleDataFile( BMessage *msg );
	void _MimeData( BMessage *msg );

	// Translate a bitmap and return the MIME type of the translated data.
	BMallocIO *_TranslateBitmap( BMessage *msg, char **type_out );
	
	// Convert a MIME type to a Translation Kit type code
	uint32 _Mime2TypeCode( BTranslatorRoster *roster, const char *mime );
};

#endif
