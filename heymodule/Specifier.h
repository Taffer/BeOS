// Specifier
//
// The Specifier object is used by heymodule to target specific properties
// of an application.
//
// Copyright © 1998 Chris Herborth (chrish@kagi.com)
//                  Arcane Dragon Software
//
// License:  You can do anything you want with this source code, including
//           incorporating it into commercial applications, as long as you
//           give me credit in the About box and documentation.
//
// $Id: Specifier.h,v 1.1.1.1 1999/06/08 12:49:38 chrish Exp $

#ifndef PyHey_Specifier_H
#define PyHey_Specifier_H

#include "Python.h"

#include <app/Message.h>

// The object:
typedef struct {
	PyObject_HEAD
	BMessage *msg;
} SpecifierObject;

// The object's type:
extern PyTypeObject Specifier_Type;

// Macro for checking the type:
#define SpecifierObject_Check(v) ((v)->ob_type == &Specifier_Type)

// Methods you can use.
SpecifierObject *newSpecifierObject( PyObject *arg );

#endif
