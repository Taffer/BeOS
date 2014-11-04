// Hey
//
// The Hey object is used by heymodule to exchange scripting messages
// with an application.
//
// Copyright © 1998 Chris Herborth (chrish@kagi.com)
//                  Arcane Dragon Software
//
// License:  You can do anything you want with this source code, including
//           incorporating it into commercial applications, as long as you
//           give me credit in the About box and documentation.
//
// $Id: Hey.cpp,v 1.1.1.1 1999/06/08 12:49:37 chrish Exp $

#include "Hey.h"
#include "Specifier.h"

#include <app/Messenger.h>
#include <app/Message.h>
#include <app/PropertyInfo.h>
#include <app/Roster.h>
#include <support/List.h>
#include <support/TypeConstants.h>
#include <interface/GraphicsDefs.h>
#include <string.h>

// ----------------------------------------------------------------------
// Free nose jobs for everyone!
static PyObject *IOError_file( char *error, char *path, status_t val = B_ERROR );
static PyObject *Launch_error( char *signature, status_t val = B_ERROR );
static PyObject *UnknownObj_error( type_code type, void *ptr, ssize_t size );

static PyObject *msg_to_dict( const BMessage &msg );
static PyObject *build_command_string( uint32 cmd );
static PyObject *build_specifier_string( uint32 spec );
static PyObject *build_property_tuple( const property_info& prop );
static PyObject *build_property_info_dict( const BPropertyInfo& pi );
static PyObject *build_suite_dict( const BMessage& msg );
static PyObject *obj_to_python( uint32 type, const void *ptr, ssize_t size );
static PyObject *get_message_data( const BMessage& msg, const char* name, int32 index );
static PyObject *build_message_list( const BMessage& msg, const char* name );
static int32 count_message_items( const BMessage& msg, const char* name );
static PyObject *explain_reply( const BMessage &reply );
static SpecifierObject *parse_specifier( PyObject* args );

// ----------------------------------------------------------------------
// Error handlers for common situations.
//
// IOError_file - can't get an entry_ref, etc. for the path
// Launch_error - can't launch the specified signature
// UnknownObj_error - can't convert this object to Python

static PyObject *IOError_file( char *error, char *path, status_t val )
{
	// Create a tuple for the exception, containing:
	// - error value
	// - strerror() for the error value
	// - error message
	// - path
	// == 4 items
	PyObject *ex = PyTuple_New( 4 );
	if( ex == NULL ) return PyErr_NoMemory();
	
	// TODO: If any of these fail, we should probably nuke the tuple and 
	// return PyErr_NoMemory().
	int idx = 0;
	(void)PyTuple_SetItem( ex, idx++, PyInt_FromLong( val ) );
	(void)PyTuple_SetItem( ex, idx++, PyString_FromString( strerror( val ) ) );
	(void)PyTuple_SetItem( ex, idx++, PyString_FromString( error ) );
	(void)PyTuple_SetItem( ex, idx++, PyString_FromString( path ) );

	PyErr_SetObject( PyExc_IOError, ex );
	return NULL;
}

static PyObject *Launch_error( char *signature, status_t val )
{
	// Create a tuple for the exception, containing:
	// - error value
	// - strerror() for the error value
	// - signature
	// == 3 items
	PyObject *ex = PyTuple_New( 3 );
	if( ex == NULL ) return PyErr_NoMemory();
	
	// TODO: If any of these fail, we should probably nuke the tuple and 
	// return PyErr_NoMemory().
	int idx = 0;
	(void)PyTuple_SetItem( ex, idx++, PyInt_FromLong( val ) );
	(void)PyTuple_SetItem( ex, idx++, PyString_FromString( strerror( val ) ) );
	(void)PyTuple_SetItem( ex, idx++, PyString_FromString( signature ) );

	PyErr_SetObject( PyExc_RuntimeError, ex );
	return NULL;
}

static PyObject *UnknownObj_error( type_code type, void *ptr, ssize_t size )
{
	// TODO: should return the "type" as an int and as a string, too.
#ifdef __MWERKS__
#pragma unused (type)
#else
	type = type;
#endif

	// Create a tuple for the exception, containing:
	// - "unknown object type; can't convert to Python"
	// - Python string of ptr, with specified size
	// == 2 items
	PyObject *ex = PyTuple_New( 2 );
	if( ex == NULL ) return PyErr_NoMemory();
	
	// TODO: If any of these fail, we should probably nuke the tuple and 
	// return PyErr_NoMemory().
	int idx = 0;
	(void)PyTuple_SetItem( ex, idx++, PyString_FromString( "unknown object type; can't convert to Python" ) );
	(void)PyTuple_SetItem( ex, idx++, 
					PyString_FromStringAndSize( (char *)ptr, size ) );

	PyErr_SetObject( PyExc_ValueError, ex );
	return NULL;
}

// ----------------------------------------------------------------------
// ODS 22-Jul-1999
// Convert a BMessage's contents into a dictionary, indexed by the name;
// every data comes as a list to handle multiple instances of that name.
static PyObject *msg_to_dict( const BMessage &msg )
{
	int count = msg.CountNames( B_ANY_TYPE );
	if( count == 0 ) {
		Py_INCREF( Py_None );
		return Py_None;
	}
	
	PyObject *dict = PyDict_New();
	if( dict == NULL ) return PyErr_NoMemory();

	// ODS 22-Jul-1999: "name" should be treated as const char.
	// An unfortunate oversight in the Be API allows you to get away
	// with this.
	char *name;
	uint32 type;
	for( int idx = 0; idx < count; idx++ ) {
		msg.GetInfo( B_ANY_TYPE, idx, &name, &type );
		PyObject *list = build_message_list(msg, name);
		PyObject *key = PyString_FromString( name );

		PyDict_SetItem( dict, key, list );
		
		// ODS 22-Jul-1999: Dictionaries obtain their own
		// references to stored objects, so we should
		// release our own.
		Py_XDECREF(key);
		Py_XDECREF(list);
	}

	return dict;
}

// ----------------------------------------------------------------------
// ODS 22-Jul-1999
// Scripting suites: create a Python string that describes a scripting
// command. In a perfect world, you could turn around and use this
// object on a Hey-able object, e.g. window.SendCommand("Get", "Title").
static PyObject* build_command_string( uint32 cmd )
{
	const char* cmdStr;
	char cs[5];
	switch (cmd) {
	case B_GET_PROPERTY: cmdStr = "Get"; break;
	case B_SET_PROPERTY: cmdStr = "Set"; break;
	case B_CREATE_PROPERTY: cmdStr = "Create"; break;
	case B_DELETE_PROPERTY: cmdStr = "Delete"; break;
	case B_EXECUTE_PROPERTY: cmdStr = "Execute"; break;
	case B_COUNT_PROPERTIES: cmdStr = "Count"; break;
	case B_GET_SUPPORTED_SUITES: cmdStr = "GetSuites"; break;
	default:
		{
			uint32 scmd = B_HOST_TO_BENDIAN_INT32(cmd);
			memcpy(cs, &scmd, 4);
			cs[4] = '\0';
			cmdStr = cs;
		}
		break;
	}
	PyObject* obj = PyString_FromString(cmdStr);
	return obj;		
}

// ----------------------------------------------------------------------
// ODS 22-Jul-1999
// Scripting suites: create a Python string that describes a scripting
// specifier.
static PyObject* build_specifier_string( uint32 spec )
{
	const char* specStr;
	char ss[20];
	switch (spec) {
	case B_DIRECT_SPECIFIER: specStr = "Direct"; break;
	case B_INDEX_SPECIFIER: specStr = "Index"; break;
	case B_REVERSE_INDEX_SPECIFIER: specStr = "Reverse Index"; break;
	case B_RANGE_SPECIFIER: specStr = "Range"; break;
	case B_REVERSE_RANGE_SPECIFIER: specStr = "Reverse Range"; break;
	case B_NAME_SPECIFIER: specStr = "Name"; break;
	case B_ID_SPECIFIER: specStr = "ID"; break;
	default:
		{
			sprintf(ss, "Spec %#lx", spec);
			specStr = ss;
		}
		break;
	}
	
	PyObject* obj = PyString_FromString(specStr);
	return obj;
}

// ----------------------------------------------------------------------
// ODS 22-Jul-1999
// Scripting suites: create a Python tuple that contains all of the
// information for a scripting property.
static PyObject* build_property_tuple( const property_info& prop )
{
	int count, i;
	
	// how many properties are there?
	for (count=0; prop.commands[count]; count++);
	
	// Memory Management: note that PyTuple_SetItem and PyList_SetItem
	// assume ownsership of whatever you pass in, so we can safely
	// ignore the objects we've created once we've handed them off.
	// If it were a dictionary we'd have to decrement our own
	// reference counts to keep the count correct.
	PyObject* cmds = PyTuple_New(count);	
	for (i=0; i<count; i++) {
		PyTuple_SetItem(cmds, i, build_command_string(prop.commands[i]));
	}

	for (count=0; prop.specifiers[count]; count++);
	
	PyObject* specs = PyTuple_New(count);
	for (i=0; i<count; i++) {
		PyTuple_SetItem(specs, i, build_specifier_string(prop.specifiers[i]));
	}

	PyObject* tProp = PyTuple_New(4);
	PyTuple_SetItem(tProp, 0, cmds);
	PyTuple_SetItem(tProp, 1, specs);
	if (prop.usage)
		PyTuple_SetItem(tProp, 2, PyString_FromString(prop.usage));
	else {
		// no string for you!
		Py_INCREF(Py_None);
		PyTuple_SetItem(tProp, 2, Py_None);
	}
	PyTuple_SetItem(tProp, 3, PyLong_FromLong(prop.extra_data));
	
	return tProp;
}

// ----------------------------------------------------------------------
// ODS 22-Jul-1999
// Scripting suites: create a Python dictionary that contains all of the
// information for a scripting suite. Each entry in the dictionary is
// keyed by property name, and contains a Python list of all property
// info structs by that name.
static PyObject* build_property_info_dict( const BPropertyInfo& pi )
{
	PyObject* dict = PyDict_New();
	if (! dict) return PyErr_NoMemory();

	int32 count = pi.CountProperties();
	const property_info *props = pi.Properties();
	for (int32 i=0; i<count; i++) {
		PyObject* key = PyString_FromString(props[i].name);
		PyObject* tuple	 = build_property_tuple(props[i]);
		PyObject* list = PyDict_GetItem(dict, key);
		// Acquire a reference for the list we've just
		// grabbed from the dictionary -- when we call
		// SetItem, the dictionary will be releasing
		// its reference!
		Py_XINCREF(list);
		if (! list)
			list = PyList_New(0);
		PyList_Append(list, tuple);
		PyDict_SetItem(dict, key, list);
		// The dictionary creates its own references for these.
		// We're done with our own references, so get rid of 'em!
		Py_XDECREF(key);
		Py_XDECREF(tuple);
		Py_DECREF(list);
	}
	return dict;
}

// ----------------------------------------------------------------------
// ODS 22-Jul-1999
// Scripting suites: build a dictionary that describes all of the
// scripting suites in a message. Each entry is keyed by suite signature,
// and contains a dictionary of the suite contents.
static PyObject* build_suite_dict( const BMessage& msg )
{
	PyObject* dict = PyDict_New();
	if (! dict) return PyErr_NoMemory();
	
	int32 numSuites = count_message_items(msg, "suites");
	for (int32 i=0; i<numSuites; i++) {
		const char* suiteName = msg.FindString("suites", i);
		PyObject* key = PyString_FromString(suiteName);
		
		const void* propData;
		ssize_t size;
		msg.FindData("messages", B_PROPERTY_INFO_TYPE, i, &propData, &size);
		BPropertyInfo pi;
		pi.Unflatten(B_PROPERTY_INFO_TYPE, propData, size);
		PyObject* value = build_property_info_dict(pi);	
		PyDict_SetItem(dict, key, value);
		// dictionary creates its own references;
		// we no longer need these
		Py_XDECREF(key);
		Py_XDECREF(value);
	}
	return dict;
}

// ----------------------------------------------------------------------
// Convert some data from a BMessage into something useful for Python.
static PyObject *obj_to_python( uint32 type, const void *ptr, ssize_t size )
{
	PyObject *obj;

	switch( type ) {
	case B_RECT_TYPE:
		{
			const BRect *rect = static_cast<const BRect *>(ptr);

			// Sent back as a tuple ( left, top, right, bottom ) == 4 items
			obj = PyTuple_New( 4 );
			if( obj == NULL ) return PyErr_NoMemory();
			
			int idx = 0;
			(void)PyTuple_SetItem( obj, idx++, 
						PyFloat_FromDouble( (double)rect->left ) );
			(void)PyTuple_SetItem( obj, idx++, 
						PyFloat_FromDouble( (double)rect->top ) );
			(void)PyTuple_SetItem( obj, idx++, 
						PyFloat_FromDouble( (double)rect->right ) );
			(void)PyTuple_SetItem( obj, idx++, 
						PyFloat_FromDouble( (double)rect->bottom ) );

			return obj;
		}
		
		// You won't get here.
		break;

	case B_POINT_TYPE:
		{
			const BPoint *point = static_cast<BPoint *>(ptr);
			
			// tuple (x,y) == 2 items
			obj = PyTuple_New( 2 );
			if( obj == NULL ) return PyErr_NoMemory();
			
			int idx = 0;
			(void)PyTuple_SetItem( obj, idx++,
						PyFloat_FromDouble( (double)point->x ) );
			(void)PyTuple_SetItem( obj, idx++,
						PyFloat_FromDouble( (double)point->y ) );

			return obj;
		}
		
		// You won't get here.
		break;

	case B_RGB_COLOR_TYPE:
		{
			const rgb_color *rgba = static_cast<rgb_color *>(ptr);

			// Sent back as a tuple ( r, g, b, a ) == 4 items
			obj = PyTuple_New( 4 );
			if( obj == NULL ) return PyErr_NoMemory();
			
			int idx = 0;
			(void)PyTuple_SetItem( obj, idx++, 
						PyInt_FromLong( (long)rgba->red ) );
			(void)PyTuple_SetItem( obj, idx++, 
						PyInt_FromLong( (long)rgba->green ) );
			(void)PyTuple_SetItem( obj, idx++, 
						PyInt_FromLong( (long)rgba->blue ) );
			(void)PyTuple_SetItem( obj, idx++, 
						PyInt_FromLong( (long)rgba->alpha ) );

			return obj;
		}
		
		// You won't get here.
		break;

	case B_STRING_TYPE:	// fall through
	case B_ASCII_TYPE:	// fall through
	case B_CHAR_TYPE:	// fall through
	case B_MIME_TYPE:	// fall through
	case B_RAW_TYPE:
		{
			const char* str = static_cast<const char *>(ptr);
			obj = PyString_FromStringAndSize( str, size );
			if( obj == NULL ) return PyErr_NoMemory();
			
			return obj;
		}
		
		// You won't get here.
		break;

	case B_FLOAT_TYPE:
		{
			const float *f = static_cast<const float *>(ptr);

			obj = PyFloat_FromDouble( *f );
			if( obj == NULL ) return PyErr_NoMemory();
			
			return obj;
		}
		
		// You won't get here.
		break;

	case B_DOUBLE_TYPE:
		{
			const double *d = static_cast<const double *>(ptr);

			obj = PyFloat_FromDouble( *d );
			if( obj == NULL ) return PyErr_NoMemory();
			
			return obj;
		}
		
		// You won't get here.
		break;

	case B_INT32_TYPE:		// fall through
	case B_UINT32_TYPE:		// fall through
	case B_SIZE_T_TYPE:		// fall through
	case B_SSIZE_T_TYPE:
		{
			const long *l = static_cast<const long *>(ptr);

			obj = PyInt_FromLong( *l );
			if( obj == NULL ) return PyErr_NoMemory();
			
			return obj;
		}
		
		// You won't get here.
		break;


	case B_INT16_TYPE:		// fall through
	case B_UINT16_TYPE:
		{
			const int16 *i = static_cast<const int16 *>(ptr);

			obj = PyInt_FromLong( (long)(*i) );
			if( obj == NULL ) return PyErr_NoMemory();
			
			return obj;
		}
		
		// You won't get here.
		break;


	case B_UINT8_TYPE:
	case B_INT8_TYPE:		// fall through
		{
			const int8 *i = static_cast<const int8 *>(ptr);

			obj = PyInt_FromLong( (long)(*i) );
			if( obj == NULL ) return PyErr_NoMemory();
			
			return obj;
		}
		
		// You won't get here.
		break;

	case B_MESSAGE_TYPE:
		{
			const BMessage* msg = static_cast<const BMessage*>(ptr);
			return msg_to_dict( *msg );
		}
		
		// You won't get here.
		break;

	case B_MESSENGER_TYPE:
		// ODS 21-Jul-1999: Wrap a 'hey' object around this messenger.
		{
			const BMessenger* m = static_cast<const BMessenger*>(ptr);
			HeyObject* self = PyObject_NEW(HeyObject, &Hey_Type);
			if (self) {
				self->target = new BMessenger(*m);
			}
			if( !self->target->IsValid() ) {
				delete self->target;

				PyErr_SetString( PyExc_RuntimeError,
			        "unable to create messenger" );

				// ODS 22-Jul-1999: is this the correct way to clean up self?
				Py_DECREF(self);
				self = NULL;
			}
			return (PyObject*)self;
		}
		
		// You won't get here.
		break;
		
	default:
		{
			// Unknown object; return a tuple:
			// - "UNKNOWN"
			// - string of the data
			// ODS 22-Jul-1999: Added type code so that, if we can't
			// figure it out, at least we give somebody else a fighting
			// chance at decoding the data. This will make it possible
			// for users to add support for custom data types.
			obj = PyTuple_New( 2 );
			if( obj == NULL ) return PyErr_NoMemory();	
			int idx = 0;
			PyTuple_SetItem( obj, idx++, PyString_FromString( "UNKNOWN" ) );
			PyTuple_SetItem( obj, idx++, PyInt_FromLong( type ) );
			PyTuple_SetItem( obj, idx++, 
						PyString_FromStringAndSize( (char *)ptr, size ) );

			return obj;
		}
		
		// You won't get here.
		break;
	}

	// "No soup for you!"
	Py_INCREF( Py_None );
	return Py_None;
}

// ----------------------------------------------------------------------
// Convert data in a message field to a Python object.
static PyObject* get_message_data( const BMessage& msg, const char* name, int32 index )
{
	status_t retval;
	const void* ptr;
	ssize_t size;
	type_code type = 0;

	retval = msg.GetInfo( name, &type );
	if( retval != B_OK ) {
		// If there's no reply, we return None.
		Py_INCREF( Py_None );
		return Py_None;
	}
			
	retval = msg.FindData( name, type, index, &ptr, &size );
	if( retval != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError, "error getting message data" );
		return NULL;
	}

	PyObject *thing = obj_to_python( type, ptr, size );
	return thing;
}

// ----------------------------------------------------------------------
// ODS 22-Jul-1999
// Returns a Python list of the message field's data members.
static PyObject* build_message_list( const BMessage& msg, const char* name )
{
	int32 count = count_message_items(msg, name);
	PyObject* things = PyList_New(count);
	if( things == NULL ) return PyErr_NoMemory();
	for (int32 i=0; i<count; i++) {
		PyObject* thing = get_message_data(msg, name, i);
		(void)PyList_SetItem(things, i, thing);
	}
	return things;
}

// ----------------------------------------------------------------------
// ODS 22-Jul-1999
// This tells you how many items are in a particular message field.
static int32 count_message_items( const BMessage& msg, const char* name )
{
	int32 count;
	type_code type;
	status_t retval = msg.GetInfo(name, &type, &count);
	return (retval == B_OK) ? count : 0;
}

// ----------------------------------------------------------------------
// Explain the reply message in terms useful to a Python programmer.
static PyObject *explain_reply( const BMessage &reply )
{
	switch( reply.what ) {
	case B_MESSAGE_NOT_UNDERSTOOD:
		// fall through
	case B_ERROR:
		{
			PyObject *ex = NULL;

			// Create a tuple for the exception, containing:
			// - reply's "error" value
			// - strerror() for reply's "error" value
			// - "message not understood" or "error"
			// - reply's "message" string, if there is one
			// == 4 items
			ex = PyTuple_New( 4 );
			if( ex == NULL ) return PyErr_NoMemory();

			// TODO: If any of these fail, we should probably nuke the 
			// tuple and return PyErr_NoMemory().
			int idx = 0;
			(void)PyTuple_SetItem( ex, idx++,
						PyInt_FromLong( reply.FindInt32( "error" ) ) );
			(void)PyTuple_SetItem( ex, idx++,
						PyString_FromString( strerror( reply.FindInt32( "error" ) ) ) );
			if( reply.what == B_MESSAGE_NOT_UNDERSTOOD ) {
				(void)PyTuple_SetItem( ex, idx++,
							PyString_FromString( "message not understood" ) );
			} else {
				(void)PyTuple_SetItem( ex, idx++,
							PyString_FromString( "error" ) );
			}
			if( reply.HasString( "message" ) ) {
				(void)PyTuple_SetItem( ex, idx++,
						PyString_FromString( reply.FindString( "message" ) ) );
			} else {
				(void)PyTuple_SetItem( ex, idx++,
						PyString_FromString( "(no message)" ) );
			}

			PyErr_SetObject( PyExc_RuntimeError, ex );
			return NULL;
		}

		break;	// you won't get here

	case B_REPLY:	// Yay, we got a reply!
		// Examine the reply message and return something useful to Python.
		//
		// ODS 22-Jul-1999: In the future, we might return a full dictionary
		// containing full tuples of the message's data.
		if (reply.FindString("suites")) {
			// ODS 22-Jul-1999: This message is a list of scripting suites.
			// Build a simple dictionary of the message's contents.
			return build_suite_dict(reply);
		} else {
			// A regular reply. Build a Python list out of the results.
			return build_message_list(reply, "result");
		}
		break;

	case B_NO_REPLY:
		break;

	default:
		// Beats me, I'll just spew to stdout...
		reply.PrintToStream();

		PyErr_SetString( PyExc_RuntimeError, "unknown reply type" );
		return NULL;
		
		// Won't get here.
		break;
	}

	// Attempt to return a dict of the message's contents.
	return msg_to_dict( reply );
}

// ======================================================================
// Hey object
// ======================================================================

// ----------------------------------------------------------------------
// Create a new Hey object.
//
// Can call with:
// - app signature
// - app/thread name
// - MIME type (will get the preferred handler for that type)
HeyObject *newHeyObject( PyObject *arg )
{
	HeyObject *self;
	self = PyObject_NEW( HeyObject, &Hey_Type );
	if( self == NULL ) {
		return NULL;
	}
	
	self->target = NULL;

	char *target_name = NULL;
	if( !PyArg_ParseTuple( arg, "s", &target_name ) ) {
		// TODO: we leak self here...
		return NULL;
	}

	// All this necessary to match names as well as signatures. Urk.
	BList team_list;
	team_id the_team_id;
	app_info the_app_info;
	thread_info the_thread_info;
	int32 cookie;

	// First, attempt to find it by matching thread names.
	be_roster->GetAppList( &team_list );
	for( int32 i = 0; i < team_list.CountItems(); i++ ) {
		the_team_id = (team_id)team_list.ItemAt( i );
		(void)be_roster->GetRunningAppInfo( the_team_id, &the_app_info );
		
		try {
			if( strcmp( the_app_info.signature, target_name ) == 0 ) {
				// Matched the app's signature.
				self->target = new BMessenger( the_app_info.signature );
				break;
			} else {
				// Try to match the name of one of the app's threads.
				cookie = 0L;
				if( get_next_thread_info( the_team_id, &cookie, &the_thread_info ) == B_OK ) {
					if( strcmp( the_thread_info.name, target_name ) == 0 ) {
						// Matched the thread's name.
						self->target = new BMessenger( NULL, the_team_id );
						break;
					}
				}
			}
		} catch ( bad_alloc& ex ) {
			// TODO: we leak self here...
			return (HeyObject *)PyErr_NoMemory();
		}
	}

	if( self->target == NULL ) {
		// Sure hope you passed in a signature...
		status_t retval = be_roster->Launch( target_name );
		if( retval == B_OK || retval == B_ALREADY_RUNNING ) {
			// Now take a round-about trip to find the application we
			// just launched...
			entry_ref ref;
			retval = be_roster->FindApp( target_name, &ref );
			if( retval != B_OK ) {
				return (HeyObject *)Launch_error( target_name, retval );
			}
			
			retval = be_roster->GetAppInfo( &ref, &the_app_info );
			if( retval != B_OK ) {
				return (HeyObject *)Launch_error( target_name, retval );
			}
			
			self->target = new BMessenger( the_app_info.signature );
		} else {
			// TODO: we leak self here...
			return (HeyObject *)Launch_error( target_name, retval );
		}
	}
		
	if( !self->target->IsValid() ) {
		delete self->target;

		PyErr_SetString( PyExc_RuntimeError,
			        "unable to create messenger" );

		// TODO: we leak self here...
		return NULL;
	}

	return self;
}

// ----------------------------------------------------------------------
// Delete a Hey object
static void Hey_dealloc( HeyObject *self )
{
	delete self->target;
	PyMem_DEL( self );
}

// ----------------------------------------------------------------------
// "quit" message
//
// TODO: seems to fail to quit the app now and then...
static PyObject *Hey_Quit( HeyObject *self, PyObject *args )
{
	// See if we got a specifier argument.
	SpecifierObject *spec = NULL;
	if( !PyArg_ParseTuple( args, "|O!", &Specifier_Type, &spec ) ) {
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier" );
		return NULL;
	}

	BMessage the_reply;

	if( spec ) {
		// TODO: this doesn't seem to work...
		spec->msg->what = B_QUIT_REQUESTED;

		if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
			PyErr_SetString( PyExc_RuntimeError,
				        "error sending Quit message" );

			return NULL;
		}
	} else {
		BMessage the_msg( B_QUIT_REQUESTED );

		if( self->target->SendMessage( &the_msg, &the_reply ) != B_OK ) {
			PyErr_SetString( PyExc_RuntimeError,
				        "error sending Quit message" );

			return NULL;
		}
	}

	return explain_reply( the_reply );
}

// ----------------------------------------------------------------------
// "save" message
//
// TODO: needs to make use of the specifier
static PyObject *Hey_Save( HeyObject *self, PyObject *args )
{
	// See if we got a specifier argument.
	SpecifierObject *spec = NULL;
	if( !PyArg_ParseTuple( args, "|O!", &Specifier_Type, &spec ) ) {
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier" );
		return NULL;
	}

	BMessage the_reply;

	if( spec ) {
		spec->msg->what = B_SAVE_REQUESTED;

		if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
			PyErr_SetString( PyExc_RuntimeError,
				        "error sending Save message" );

			return NULL;
		}
	} else {
		BMessage the_msg( B_SAVE_REQUESTED );

		if( self->target->SendMessage( &the_msg, &the_reply ) != B_OK ) {
			PyErr_SetString( PyExc_RuntimeError,
				        "error sending Save message" );

			return NULL;
		}
	}

	return explain_reply( the_reply );
}

// ----------------------------------------------------------------------
// "load" message
//
// Call with a string path.
//
// TODO: allow entry_ref tuples
static PyObject *Hey_Load( HeyObject *self, PyObject *args )
{
	// Should have an argument, the filename to load.
	char *filename;
	if( !PyArg_ParseTuple( args, "s", &filename ) ) {
		return NULL;
	}

	entry_ref fileref;
	status_t retval = get_ref_for_path( filename, &fileref );
	if( retval != B_OK ) {
		return IOError_file( "unable to create entry_ref", filename, retval );
	}
	
	BEntry entry;
	retval = entry.SetTo( &fileref );
	if( retval != B_OK ) {
		return IOError_file( "unable to create BEntry", filename, retval );
	}

	BMessage the_msg( B_REFS_RECEIVED );
	BMessage the_reply;

	// RefsReceived() wants "refs", scripting apparently wants "data".
	the_msg.AddRef( "refs", &fileref );
	the_msg.AddRef( "data", &fileref );
	
	if( self->target->SendMessage( &the_msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Load message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

// ----------------------------------------------------------------------
// ODS 22-Jul-1999
// Create a specifier object from arguments.
// This differs from standard specifier creation in that, if a specifier
// object is passed in, that specifier object is simply incremented and
// returned. Otherwise, a new specifier object is created and returned.
// In either case, this function increments the specifier's reference
// count, so when you're done with the specifier object, decrement it.
static SpecifierObject* parse_specifier(PyObject* args)
{
	// Get the specifier first.	
	PyObject* obj;
	if (! PyArg_ParseTuple( args, "O", &obj ) )
		return NULL;
	
	SpecifierObject *spec = NULL;
	// Mmm, crufty C casts! Gotta love that object-oriented C...
	PyTypeObject* type = reinterpret_cast<PyTypeObject*>(PyObject_Type( obj ));
	if (type == &Specifier_Type) {
		// we were passed in a specifier directly
		// recast and increment count to balance
		// the decrement later on
		spec = reinterpret_cast<SpecifierObject*>(obj);
		Py_INCREF(spec);
	} else {
		// we were passed in something else;
		// see if we can make a specifier out of it!
		spec = newSpecifierObject(args);
	}
	
	return spec;
}

// ----------------------------------------------------------------------
// "get" message
//
// Call with a Specifier object or with something that can be converted
// to a Specifier object.
static PyObject *Hey_Get( HeyObject *self, PyObject *args )
{
	SpecifierObject* spec = parse_specifier(args);
		
	if (! spec) {
		// We weren't given a specifier.
		return NULL;
	}
		
	spec->msg->what = B_GET_PROPERTY;
	BMessage the_reply;

	PyObject* obj;
	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Get message" );

		obj = NULL;
	} else {
		obj = explain_reply( the_reply );
	}
	
	// ODS 21-Jul-1999
	// If we've created a temporary spec object, this ought to free it.
	// If we received a spec object, we incremented earlier, so this
	// will result in no effect.
	Py_DECREF(spec);
	return obj;
}

// ----------------------------------------------------------------------
// "set" messages
static PyObject *Hey_SetString( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and something else.
	SpecifierObject *spec;
	char *str;
	if( !PyArg_ParseTuple( args, "O!s", &Specifier_Type, &spec, &str ) ) {
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier and a string" );
		return NULL;
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddString( "data", str );

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

static PyObject *Hey_SetPath( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and something else.
	SpecifierObject *spec;
	char *str;
	if( !PyArg_ParseTuple( args, "O!s", &Specifier_Type, &spec, &str ) ) {
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier and a string" );
		return NULL;
	}

	entry_ref file_ref;
	status_t retval = get_ref_for_path( str, &file_ref );
	if( retval != B_OK ) {
		return IOError_file( "can't get ref for path", str, retval );
	}

	BEntry entry;
	retval = entry.SetTo( &file_ref );
	if( retval != B_OK ) {
		return IOError_file( "can't make Entry for ref", str, retval );
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddRef( "data", &file_ref );

	if( spec->msg->HasData( "refs", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "refs" );
	}
	spec->msg->AddRef( "refs", &file_ref );	// for RefsReceived()

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

static PyObject *Hey_SetColor( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and at least rgb.
	SpecifierObject *spec;
	rgb_color colour;
	colour.alpha = 255;
	if( PyArg_ParseTuple( args, "O!(bbbb)", &Specifier_Type, &spec,
				&colour.red, &colour.green, &colour.blue, &colour.alpha ) ) {
		// That's good.
		;
	} else if( PyArg_ParseTuple( args, "O!(bbb)", &Specifier_Type, &spec,
				&colour.red, &colour.green, &colour.blue ) ) {
		// That's good.  Ignore the error from the first PyArg_ParseTuple().
		PyErr_Clear();
	} else if( !PyArg_ParseTuple( args, "O!bbb|b", &Specifier_Type, &spec,
				&colour.red, &colour.green, &colour.blue, &colour.alpha ) ) {
		// That's good.  Ignore the error from the first PyArg_ParseTuple()s.
		PyErr_Clear();
	} else {
		// That's bad.
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier and a color" );
		return NULL;
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddData( "data", B_RGB_COLOR_TYPE, &colour, sizeof( rgb_color ) );

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

static PyObject *Hey_SetColour( HeyObject *self, PyObject *args )
{
	// For those of us who can spell correctly...
	return Hey_SetColor( self, args );
}

static PyObject *Hey_SetRect( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and a BRect.
	SpecifierObject *spec;
	BRect rect;
	if( !PyArg_ParseTuple( args, "O!(ffff)", &Specifier_Type, &spec,
				&rect.left, &rect.top, &rect.right, &rect.bottom ) ) {
		// Try to look for ( spec, float, float, float, float ).
		PyErr_Clear();

		if( !PyArg_ParseTuple( args, "O!ffff", &Specifier_Type, &spec,
				&rect.left, &rect.top, &rect.right, &rect.bottom ) ) {
			// That's bad.
			PyErr_SetString( PyExc_TypeError,
					"invalid arguments; expected a Specifier and a rectangle" );
			return NULL;
		}
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddRect( "data", rect );

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

static PyObject *Hey_SetPoint( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and a BPoint.
	SpecifierObject *spec;
	BPoint point;
	if( !PyArg_ParseTuple( args, "O!(ff)", &Specifier_Type, &spec,
				&point.x, &point.y ) ) {
		// Try to look for ( spec, float, float ).
		PyErr_Clear();

		if( !PyArg_ParseTuple( args, "O!ff", &Specifier_Type, &spec,
				&point.x, &point.y ) ) {
			// That's bad.
			PyErr_SetString( PyExc_TypeError,
					"invalid arguments; expected a Specifier and a point" );
			return NULL;
		}
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddPoint( "data", point );

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

static PyObject *Hey_SetInt( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and an int.
	SpecifierObject *spec;
	int num;
	if( !PyArg_ParseTuple( args, "O!i", &Specifier_Type, &spec, &num ) ) {
		// That's bad.
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier and a number" );
		return NULL;
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddInt32( "data", num );

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

static PyObject *Hey_SetInt8( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and an int.
	SpecifierObject *spec;
	int8 num;
	if( !PyArg_ParseTuple( args, "O!b", &Specifier_Type, &spec, &num ) ) {
		// That's bad.
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier and a number" );
		return NULL;
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddInt8( "data", num );

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

static PyObject *Hey_SetInt16( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and an int.
	SpecifierObject *spec;
	int16 num;
	if( !PyArg_ParseTuple( args, "O!h", &Specifier_Type, &spec, &num ) ) {
		// That's bad.
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier and a number" );
		return NULL;
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddInt16( "data", num );

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

static PyObject *Hey_SetInt32( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and an int.
	SpecifierObject *spec;
	int32 num;
	if( !PyArg_ParseTuple( args, "O!i", &Specifier_Type, &spec, &num ) ) {
		// That's bad.
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier and a number" );
		return NULL;
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddInt32( "data", num );

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

static PyObject *Hey_SetFloat( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and an int.
	SpecifierObject *spec;
	float num;
	if( !PyArg_ParseTuple( args, "O!f", &Specifier_Type, &spec, &num ) ) {
		// That's bad.
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier and a number" );
		return NULL;
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddFloat( "data", num );

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

static PyObject *Hey_SetDouble( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and an int.
	SpecifierObject *spec;
	double num;
	if( !PyArg_ParseTuple( args, "O!d", &Specifier_Type, &spec, &num ) ) {
		// That's bad.
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier and a number" );
		return NULL;
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddDouble( "data", num );

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

static PyObject *Hey_SetBool( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument and an bool.
	SpecifierObject *spec;
	bool val;
	int num;
	char *str;

	if( PyArg_ParseTuple( args, "O!i", &Specifier_Type, &spec, &num ) ) {
		if( num ) {
			val = true;
		} else {
			val = false;
		}
	} else if( PyArg_ParseTuple( args, "O!s", &Specifier_Type, &spec, &str ) ) {
		PyErr_Clear();

		// Hmm, I wonder if "yes"/"no" are covered in C locale settings...
		if( strcasecmp( str, "true" ) == 0 || 
			strcasecmp( str, "yes" )  == 0 ||
			strcasecmp( str, "oui" )  == 0 ||
			strcasecmp( str, "da" )   == 0 ||
			strcasecmp( str, "hai" )  == 0 ) {
			val = true;
		} else {
			val = false;
		}
	} else {
		// That's bad.
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier and a number" );
		return NULL;
	}

	spec->msg->what = B_SET_PROPERTY;
	if( spec->msg->HasData( "data", B_ANY_TYPE ) ) {
		(void)spec->msg->RemoveName( "data" );
	}
	spec->msg->AddBool( "data", val );

	BMessage the_reply;

	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Set message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

// ----------------------------------------------------------------------
// "create" message
static PyObject *Hey_Create( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument.
	SpecifierObject *spec = parse_specifier(args);
	if( ! spec) {
		return NULL;
	}

	spec->msg->what = B_CREATE_PROPERTY;
	BMessage the_reply;

	PyObject* obj;
	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Create message" );

		obj = NULL;
	} else {
		obj = explain_reply( the_reply );
	}
	
	Py_DECREF(spec);
	return obj;
}

// ----------------------------------------------------------------------
// "delete" message
static PyObject *Hey_Delete( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument.
	SpecifierObject *spec = parse_specifier(args);
	if( ! spec) {
		return NULL;
	}

	spec->msg->what = B_DELETE_PROPERTY;
	BMessage the_reply;

	PyObject* obj;
	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Delete message" );

		obj = NULL;
	} else {
		obj = explain_reply( the_reply );
	}
	
	Py_DECREF(spec);
	return obj;
}

// ----------------------------------------------------------------------
// "count" message
//
// call with a Specifier object
static PyObject *Hey_Count( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument.
	SpecifierObject *spec = parse_specifier(args);
	if( ! spec) {
		return NULL;
	}

	spec->msg->what = B_COUNT_PROPERTIES;
	BMessage the_reply;

	PyObject* obj;
	if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending Count message" );

		obj = NULL;
	} else {
		obj = explain_reply( the_reply );
	}
	
	Py_DECREF(spec);
	return obj;
}

// ----------------------------------------------------------------------
// "getsuites" message
static PyObject *Hey_GetSuites( HeyObject *self, PyObject *args )
{
	// Make sure we got a specifier argument.
	SpecifierObject *spec = NULL;
	if( !PyArg_ParseTuple( args, "|O!", &Specifier_Type, &spec ) ) {
		PyErr_SetString( PyExc_TypeError,
				"invalid arguments; expected a Specifier" );
		return NULL;
	}

	BMessage the_reply;

	if( spec ) {
		spec->msg->what = B_GET_SUPPORTED_SUITES;
		if( self->target->SendMessage( spec->msg, &the_reply ) != B_OK ) {
			PyErr_SetString( PyExc_RuntimeError,
				        "error sending GetSuites message" );

			return NULL;
		}
	} else {
		BMessage the_msg( B_GET_SUPPORTED_SUITES );
		if( self->target->SendMessage( &the_msg, &the_reply ) != B_OK ) {
			PyErr_SetString( PyExc_RuntimeError,
				        "error sending GetSuites message" );

			return NULL;
		}
	}

	return explain_reply( the_reply );
}

// ----------------------------------------------------------------------
// send a specific message; can be string of 1-4 chars, or an int
//
// TODO: can't currently send any extra data along with this... we
// should take a dictionary and pack each item into the message;
// difficult to handle properly though.
static PyObject *Hey_Send( HeyObject *self, PyObject *args )
{
	// Make sure we got a "what".
	char *str;
	uint32 what;
	if( !PyArg_ParseTuple( args, "i", &what ) ) {
		// Try to look for ( spec, str ).
		PyErr_Clear();

		if( !PyArg_ParseTuple( args, "s", &str ) ) {
			// That's bad.
			PyErr_SetString( PyExc_TypeError,
					"invalid arguments; expected a message 'what'" );
			return NULL;
		} else {
			if( strlen( str ) != 4 ) {
				PyErr_SetString( PyExc_ValueError,
					"invalid message 'what'" );
				return NULL;
			}

			what = (((uint32)str[0]) << 24 ) +
				   (((uint32)str[1]) << 16 ) +
				   (((uint32)str[2]) <<  8 ) +
				   ((uint32)str[3]);
		}
	}

	BMessage msg( what );
	BMessage the_reply;

	if( self->target->SendMessage( &msg, &the_reply ) != B_OK ) {
		PyErr_SetString( PyExc_RuntimeError,
			        "error sending message" );

		return NULL;
	}

	return explain_reply( the_reply );
}

// ----------------------------------------------------------------------
// Create an empty specifier
static PyObject *Hey_Specifier( HeyObject *self, PyObject *args )
{
	return (PyObject *)newSpecifierObject( args );
}

// Method table for the Hey object
static PyMethodDef HeyObject_methods[] = {
	{ "Quit",	(PyCFunction)Hey_Quit,	1,	"Ask the target to quit." },
	{ "Save",	(PyCFunction)Hey_Save,	1,	"Ask the target to save the specified document." },
	{ "Load",	(PyCFunction)Hey_Load,	1,	"Ask the target to load the specified file." },
	{ "Create",	(PyCFunction)Hey_Create,	1,	"Create a new instance of a property." },
	{ "Delete",	(PyCFunction)Hey_Delete,	1,	"Delete an instance of a property." },
	{ "Get",	(PyCFunction)Hey_Get,	1,	"Get the given specifier from the target." },
	{ "GetSuites",	(PyCFunction)Hey_GetSuites,	1,	"Get the supported suites for the given specifier from the target." },
	{ "SetString",	(PyCFunction)Hey_SetString,	1,	"Set the given specifier on the target to a string." },
	{ "SetPath",	(PyCFunction)Hey_SetPath,	1,	"Set the given specifier on the target to a path." },
	{ "SetColor",	(PyCFunction)Hey_SetColor,	1,	"Set the given specifier on the target to a color." },
	{ "SetColour",	(PyCFunction)Hey_SetColour,	1,	"Set the given specifier on the target to a colour." },
	{ "SetRect",	(PyCFunction)Hey_SetRect,	1,	"Set the given specifier on the target to a rectangle." },
	{ "SetPoint",	(PyCFunction)Hey_SetPoint,	1,	"Set the given specifier on the target to a point." },
	{ "SetInt",	(PyCFunction)Hey_SetInt,	1,	"Set the given specifier on the target to a number." },
	{ "SetFloat",	(PyCFunction)Hey_SetFloat,	1,	"Set the given specifier on the target to a floating-point number." },
	{ "SetBool",	(PyCFunction)Hey_SetBool,	1,	"Set the given specifier on the target to 'true' or 'false'." },
	{ "SetInt8",	(PyCFunction)Hey_SetInt8,	1,	"Set the given specifier on the target to an 8-bit number." },
	{ "SetInt16",	(PyCFunction)Hey_SetInt16,	1,	"Set the given specifier on the target to a 16-bit number." },
	{ "SetInt32",	(PyCFunction)Hey_SetInt32,	1,	"Set the given specifier on the target to a 32-bit number." },
	{ "SetDouble",	(PyCFunction)Hey_SetDouble,	1,	"Set the given specifier on the target to a double-precision floating-point number." },
	{ "Count",	(PyCFunction)Hey_Count,	1,	"Count properties in the target." },
	{ "Send",	(PyCFunction)Hey_Send,	1,	"Send any message to the target." },
	{ "Specifier",	(PyCFunction)Hey_Specifier,	1,	"Create a Specifier for this target." },
	{ NULL,		NULL }		// sentinel
};

static PyObject *Hey_getattr( HeyObject *self, char *name )
{
	return Py_FindMethod( HeyObject_methods, (PyObject *)self, name );
}

staticforward PyTypeObject Hey_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			// ob_size
	"Hey",			// tp_name
	sizeof(HeyObject),	// tp_basicsize
	0,			// tp_itemsize
	//  methods 
	(destructor)Hey_dealloc, // tp_dealloc
	0,			// tp_print
	(getattrfunc)Hey_getattr, // tp_getattr
	0, //(setattrfunc)Xxo_setattr, // tp_setattr
	0,			// tp_compare
	0,			// tp_repr
	0,			// tp_as_number
	0,			// tp_as_sequence
	0,			// tp_as_mapping
	0,			// tp_hash
};
