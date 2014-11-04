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
// $Id: Hey.h,v 1.1.1.1 1999/06/08 12:49:37 chrish Exp $

#ifndef PyHey_Hey_H
#define PyHey_Hey_H

#include "Python.h"

#include <app/Messenger.h>

// The object:
typedef struct {
	PyObject_HEAD
	BMessenger *target;
} HeyObject;

// The object's type:
extern PyTypeObject Hey_Type;

// Macro for checking the type:
#define HeyObject_Check(v)	((v)->ob_type == &Hey_Type)

// Methods you can use.
HeyObject *newHeyObject( PyObject *arg );

#endif
