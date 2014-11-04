// heymodule.cpp
//
// A Python module that looks and behaves a little like Attila Mezei's
// "hey" command-line utility.  Yes, this lets Python do BeOS application
// scripting.
//
// Copyright © 1998 Chris Herborth (chrish@qnx.com)
//
// The add_specifier() function at the end of this file was borrowed from
// code posted by Attila Mezei at http://w3.datanet.hu/~amezei/ in the
// "hey" utility.
//
// License:  You can do anything you want with this source code, including
//           incorporating it into commercial applications, as long as you
//           give me credit in the About box and documentation.
//
// $Id: heymodule.cpp,v 1.1.1.1 1999/06/08 12:49:38 chrish Exp $

// Thinking something like this:
//
// x = Hey( "text/html" )
// s1 = x.Specifier( "Frame of Window 0" )
// rect1 = x.Get( s1 )
//
// s2 = x.Specifier()
// s2.Add( "Frame" )
// s2.Add( "Window", 0 )
// rect2 = x.Get( s2 )
//
// rect1 == rect2

#include "Python.h"

#include "Specifier.h"
#include "Hey.h"

#include <app/Application.h>

// So we can talk with the animals...
static BApplication app( "application/x-vnd.ads-pyhey" );

// --------------------------------------------------------------------- 
// hey module contents

// Convenience function
static PyObject *Hey_new( PyObject *self, PyObject *args )
{
	HeyObject *rv;
	
	rv = newHeyObject( args );

	if ( rv == NULL )
	    return NULL;

	return (PyObject *)rv;
}

// Convenience function
static PyObject *Specifier_new( PyObject *self, PyObject *args )
{
	SpecifierObject *rv;
	
	rv = newSpecifierObject( args );

	if ( rv == NULL )
	    return NULL;

	return (PyObject *)rv;
}

//  List of functions defined in the module 
static PyMethodDef hey_methods[] = {
	{ "Hey",		Hey_new,		1,	"create a new Hey object" },
	{ "Specifier",	Specifier_new,	1,	"create a new Specifier object" },
	{ NULL,		NULL }		//  sentinel 
};

// ----------------------------------------------------------------------
//  Initialization function for the module
extern "C" void inithey( void );

extern "C" void inithey( void )
{
	PyObject *m, *d;

	// Create the hey module.
	m = Py_InitModule( "hey", hey_methods );

	// Add some extra goodies to the hey module; a docstring, and an
	// __rsc_id__ with the RCS info in it.
	d = PyModule_GetDict( m );
	PyDict_SetItemString( d, "__doc__", 
		PyString_FromString( 
			"BeOS Application scripting, like the `hey' utility.\n"
			"\n"
			"Create a new Hey object:\n"
			"\to = Hey( application_signature )\n"
			"\to = Hey( MIME_type )\n"
			"\to = Hey( thread_name )\n"
			"\n"
			"Create a new Specifier object:\n"
			"\ts = Specifier()\t# Empty specifier\n"
			"\ts = Specifier( hey_specifier_string )" ) );

	PyDict_SetItemString( d, "__rcs_id__", 
		PyString_FromString( "$Id: heymodule.cpp,v 1.1.1.1 1999/06/08 12:49:38 chrish Exp $" ) );

	PyDict_SetItemString( d, "__version__", 
		PyString_FromString( "heymodule 1.1" ) );
}
