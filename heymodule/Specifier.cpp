// Specifier
//
// The Specifier object is used by heymodule to target specific properties
// of an application.
//
// Copyright © 1998 Chris Herborth (chrish@kagi.com)
//                  Arcane Dragon Software
//
// The add_specifier() function at the end of this file was borrowed from
// code posted by Attila Mezei at http://w3.datanet.hu/~amezei/ in the
// "hey" utility.
//
// License:  You can do anything you want with this source code, including
//           incorporating it into commercial applications, as long as you
//           give me credit in the About box and documentation.
//
// $Id: Specifier.cpp,v 1.1.1.1 1999/06/08 12:49:38 chrish Exp $

#include "Specifier.h"

// Proto for func stolen from Attila's "hey"... code is at the bottom.
static status_t add_specifier( BMessage *to_message, char *argv[], int32 *argx );

// ======================================================================
// Specifier object
// ======================================================================

// ----------------------------------------------------------------------
// Create a new Specifier object.
SpecifierObject *newSpecifierObject( PyObject *arg )
{
	char *spec = NULL;
	if( !PyArg_ParseTuple( arg, "s", &spec ) ) {
		PyErr_Clear();

		if( !PyArg_ParseTuple( arg, "" ) ) {
			// Ignore Python's error so I can make my own.
			PyErr_Clear();

			PyErr_SetString( PyExc_TypeError,
					"invalid arguments; expected none or a specifier string" );
			return NULL;
		}
	}

	SpecifierObject *self;
	self = PyObject_NEW( SpecifierObject, &Specifier_Type );
	if( self == NULL ) {
		return NULL;
	}
	
	try {
		self->msg = new BMessage;
	} catch ( bad_alloc &ex ) {
		// TODO: we leak self here...
		return NULL;
	}

	if( spec ) {
		// Ow!  Evil string specifier given...
		char *tmp = strdup( spec );

		// Count the number of "words".
		int spec_argc = 1;
		for( size_t idx = 0; idx < strlen( tmp ); idx++ ) {
			if( tmp[idx] == ' ' ) spec_argc++;
		}
		
		char **spec_argv = (char **)malloc( sizeof( char * ) * ( spec_argc + 1 ) );
		if( spec_argv == NULL ) return (SpecifierObject *)PyErr_NoMemory();

		// Split the string up into "words".
		int arg = 0;
		char *ptr = strtok( tmp, " " );
		while( ptr ) {
			spec_argv[arg++] = strdup( ptr );
			ptr = strtok( NULL, " " );
		}
		spec_argv[arg] = NULL;

		int32 argx = 0;
		status_t retval = B_OK;
		while( retval == B_OK ) {
			retval = add_specifier( self->msg, spec_argv, &argx );
		}
		
		// Release all the memory we wasted.
		for( int idx = 0; spec_argv[idx] != NULL; idx++ ) {
			free( spec_argv[idx] );
		}
		free( spec_argv );
		free( tmp );
		
		// Now decide how well things went.
		switch( retval ) {
		case B_BAD_SCRIPT_SYNTAX:
			// TODO: we leak here...
			PyErr_SetString( PyExc_SyntaxError, "bad script syntax" );
			return NULL;
			break;
			
		case B_ERROR:	// That's OK, we're just at the end of our string.
			// Fall-through...
		case B_OK:		// That's OK too.
			break;

		default:
			// "The frogurt is also cursed."
			PyErr_SetString( PyExc_RuntimeError, "unknown specifier error" );
			break;
		}
	}

	return self;
}

// ----------------------------------------------------------------------
// Delete a Specifier object
static void Specifier_dealloc( SpecifierObject *self )
{
	delete self->msg;
	PyMem_DEL( self );
}

// ----------------------------------------------------------------------
// Add a specifier
//
// Call with:
//
// Add( "Frame" )
// Add( "Window", 0 )
// Add( "Window", -1 )
// Add( "Window", "Untitled" )
// Add( "Line", 1, 5 ) - range starts at 1, goes for 5 items
// Add( "Line", -1, 5 ) - reverse range
static PyObject *Specifier_Add( SpecifierObject *self, PyObject *arg )
{
	char *property, *name;
	int index, range_start, range_run;

	if( PyArg_ParseTuple( arg, "s", &property ) ) {
		// It _is_ just a string, so we've got a direct specifier.
		self->msg->AddSpecifier( property );
	} else if( PyArg_ParseTuple( arg, "ss", &property, &name ) ) {
		// It's a name specifier...
		self->msg->AddSpecifier( property, name );
	} else if( PyArg_ParseTuple( arg, "si", &property, &index ) ) {
		// Either index or reverse index.
		if( index < 0 ) {
			BMessage reverse( B_REVERSE_INDEX_SPECIFIER );
			reverse.AddString( "property", property );
			reverse.AddInt32( "index", -index );
			self->msg->AddSpecifier( &reverse );
		} else {
			self->msg->AddSpecifier( property, index );
		}
	} else if( PyArg_ParseTuple( arg, "sii", &property, &range_start, &range_run ) ) {
		// Either range or reverse range.
		if( range_start < 0 ) {
			if( range_run < 0 ) {
				PyErr_SetString( PyExc_ValueError, "range must not be negative" );
				return NULL;
			}

			BMessage reverse( B_REVERSE_RANGE_SPECIFIER );
			reverse.AddString( "property", property );
			reverse.AddInt32( "index", -range_start );
			reverse.AddInt32( "range", range_run );
			self->msg->AddSpecifier( &reverse );
		} else {
			self->msg->AddSpecifier( property, range_start, range_run );
		}
	} else {
		// Clear Python's error, then make our own exception.
		PyErr_Clear();

		PyErr_SetString( PyExc_ValueError, "invalid specifier" );
		return NULL;
	}

	PyErr_Clear();
	Py_INCREF( Py_None );
	return Py_None;
}

// ----------------------------------------------------------------------
// Method table and whatnot for the Specifier object.
static PyMethodDef SpecifierObject_methods[] = {
	{ "Add",	(PyCFunction)Specifier_Add,	1,	"Add a specifier." },
	{ NULL, NULL }	// sentinel
};

static PyObject *Specifier_getattr( SpecifierObject *self, char *name )
{
	return Py_FindMethod( SpecifierObject_methods, (PyObject *)self, name );
}

PyTypeObject Specifier_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			// ob_size
	"Specifier",			// tp_name
	sizeof(SpecifierObject),	// tp_basicsize
	0,			// tp_itemsize
	//  methods 
	(destructor)Specifier_dealloc, // tp_dealloc
	0,			// tp_print
	(getattrfunc)Specifier_getattr, // tp_getattr
	0, //(setattrfunc)Xxo_setattr, // tp_setattr
	0,			// tp_compare
	0,			// tp_repr
	0,			// tp_as_number
	0,			// tp_as_sequence
	0,			// tp_as_mapping
	0,			// tp_hash
};

// ======================================================================
// Code borrowed from "hey" to parse a "hey" specifier line; it's not
// just _similar_ parsing, it's the real thing!
// returns B_OK if successful 
//         B_ERROR if no more specifiers
//         B_BAD_SCRIPT_SYNTAX if syntax error
static status_t add_specifier(BMessage *to_message, char *argv[], int32 *argx)
{

	char *property=argv[*argx];
	
	if(property==NULL) return B_ERROR;		// no more specifiers
	
	(*argx)++;
	
	if(strcasecmp(property, "to")==0){	// it is the 'to' string!!!
		return B_ERROR;	// no more specifiers
	}
	
	if(strcasecmp(property, "of")==0){		// skip "of", read real property
		property=argv[*argx];
		if(property==NULL) return B_BAD_SCRIPT_SYNTAX;		// bad syntax
		(*argx)++;
	}
	
	// decide the specifier

	char *specifier=argv[*argx];
	if(specifier==NULL){	// direct specifier
		to_message->AddSpecifier(property);
		return B_ERROR;		// no more specifiers
	}

	(*argx)++;

	if(strcasecmp(specifier, "of")==0){	// direct specifier
		to_message->AddSpecifier(property);
		return B_OK;	
	}

	if(strcasecmp(specifier, "to")==0){	// direct specifier
		to_message->AddSpecifier(property);
		return B_ERROR;		// no more specifiers	
	}


	if(specifier[0]=='['){	// index, reverse index or range
		char *end;
		int32 ix1, ix2;
		if(specifier[1]=='-'){	// reverse index
			ix1=strtoul(specifier+2, &end, 10);
			BMessage revspec(B_REVERSE_INDEX_SPECIFIER);
			revspec.AddString("property", property);
			revspec.AddInt32("index", ix1);
			to_message->AddSpecifier(&revspec);
		}else{	// index or range
			ix1=strtoul(specifier+1, &end, 10);
			if(end[0]==']'){	// it was an index
				to_message->AddSpecifier(property, ix1);
				return B_OK;	
			}else{
				specifier=argv[*argx];
				if(specifier==NULL){
					// I was wrong, it was just an index
					to_message->AddSpecifier(property, ix1);
					return B_OK;	
				}		
				(*argx)++;
				if(strcasecmp(specifier, "to")==0){
					specifier=argv[*argx];
					if(specifier==NULL){
						return B_BAD_SCRIPT_SYNTAX;		// wrong syntax
					}
					(*argx)++;
					ix2=strtoul(specifier, &end, 10);
					to_message->AddSpecifier(property, ix1, ix2-ix1>0 ? ix2-ix1+1 : 1);
					return B_OK;		
				}else{
					return B_BAD_SCRIPT_SYNTAX;		// wrong syntax
				}
			}
		}
	}else{	// name specifier
		// if it contains only digits, it will be an index...
		bool contains_only_digits=true;
		for(size_t i=0;i<strlen(specifier);i++){
			if(specifier[i]<'0' || specifier[i]>'9'){
				contains_only_digits=false;
				break;
			}
		}
		
		if(contains_only_digits){
			to_message->AddSpecifier(property, atol(specifier));
		}else{
			to_message->AddSpecifier(property, specifier);
		}
		
	}
	
	return B_OK;
}
