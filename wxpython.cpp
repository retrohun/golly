                        /*** /

This file is part of Golly, a Game of Life Simulator.
Copyright (C) 2008 Andrew Trevorrow and Tomas Rokicki.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 Web site:  http://sourceforge.net/projects/golly
 Authors:   rokicki@gmail.com  andrew@trevorrow.com

                        / ***/

/*
   Golly uses an embedded Python interpreter to execute scripts.
   Here is the official Python copyright notice:

   Copyright (c) 2001-2005 Python Software Foundation.
   All Rights Reserved.

   Copyright (c) 2000 BeOpen.com.
   All Rights Reserved.

   Copyright (c) 1995-2001 Corporation for National Research Initiatives.
   All Rights Reserved.

   Copyright (c) 1991-1995 Stichting Mathematisch Centrum, Amsterdam.
   All Rights Reserved.
*/

#include "wx/wxprec.h"     // for compilers that support precompilation
#ifndef WX_PRECOMP
   #include "wx/wx.h"      // for all others include the necessary headers
#endif

#include <limits.h>        // for INT_MAX

#include "bigint.h"
#include "lifealgo.h"
#include "qlifealgo.h"
#include "hlifealgo.h"
#include "readpattern.h"
#include "writepattern.h"

#include "wxgolly.h"       // for wxGetApp, mainptr, viewptr, statusptr
#include "wxmain.h"        // for mainptr->...
#include "wxedit.h"        // for Selection
#include "wxview.h"        // for viewptr->...
#include "wxstatus.h"      // for statusptr->...
#include "wxutils.h"       // for Warning, Note, GetString, etc
#include "wxprefs.h"       // for pythonlib, gollydir, etc
#include "wxinfo.h"        // for ShowInfo
#include "wxhelp.h"        // for ShowHelp
#include "wxundo.h"        // for currlayer->undoredo->...
#include "wxalgos.h"       // for *_ALGO, CreateNewUniverse, algobase
#include "wxlayer.h"       // for AddLayer, currlayer, currindex, etc
#include "wxscript.h"      // for inscript, abortmsg, GSF_*, etc
#include "wxpython.h"

// =============================================================================

// On Windows and Linux we need to load the Python library at runtime
// so Golly will start up even if Python isn't installed.
// Based on code from Mahogany (mahogany.sourceforge.net) and Vim (www.vim.org).

// wxMac bug??? why does dynlib.Load fail if given
// "/System/Library/Frameworks/Python.framework/Versions/2.3/Python"???
// #if 1
#ifndef __WXMAC__
   // load Python lib at runtime
   #define USE_PYTHON_DYNAMIC

   #ifdef __UNIX__
      // avoid warning on Linux
      #undef _POSIX_C_SOURCE
   #endif

   // prevent Python.h from adding Python library to link settings
   #define USE_DL_EXPORT
#endif

#include <Python.h>

#ifdef USE_PYTHON_DYNAMIC

#ifdef __WXMSW__
   // avoid warning on Windows
   #undef PyRun_SimpleString
#endif

#include "wx/dynlib.h"     // for wxDynamicLibrary

// declare G_* wrappers for the functions we want to use from Python lib
extern "C"
{
   // startup/shutdown
   void(*G_Py_Initialize)(void) = NULL;
   PyObject*(*G_Py_InitModule4)(char*, struct PyMethodDef*, char*, PyObject*, int) = NULL;
   void(*G_Py_Finalize)(void) = NULL;

   // errors
   PyObject*(*G_PyErr_Occurred)(void) = NULL;
   void(*G_PyErr_SetString)(PyObject*, const char*) = NULL;

   // ints
   long(*G_PyInt_AsLong)(PyObject*) = NULL;
   PyObject*(*G_PyInt_FromLong)(long) = NULL;
   PyTypeObject* G_PyInt_Type = NULL;

   // lists
   PyObject*(*G_PyList_New)(int size) = NULL;
   int(*G_PyList_Append)(PyObject*, PyObject*) = NULL;
   PyObject*(*G_PyList_GetItem)(PyObject*, int) = NULL;
   int(*G_PyList_SetItem)(PyObject*, int, PyObject*) = NULL;
   int(*G_PyList_Size)(PyObject*) = NULL;
   PyTypeObject* G_PyList_Type = NULL;

   // tuples
   PyObject*(*G_PyTuple_New)(int) = NULL;
   int(*G_PyTuple_SetItem)(PyObject*, int, PyObject*) = NULL;
   PyObject*(*G_PyTuple_GetItem)(PyObject*, int) = NULL;

   // misc
   int(*G_PyArg_Parse)(PyObject*, char*, ...) = NULL;
   int(*G_PyArg_ParseTuple)(PyObject*, char*, ...) = NULL;
   PyObject*(*G_PyImport_ImportModule)(const char*) = NULL;
   PyObject*(*G_PyDict_GetItemString)(PyObject*, const char*) = NULL;
   PyObject*(*G_PyModule_GetDict)(PyObject*) = NULL;
   PyObject*(*G_Py_BuildValue)(char*, ...) = NULL;
   PyObject*(*G_Py_FindMethod)(PyMethodDef[], PyObject*, char*) = NULL;
   int(*G_PyRun_SimpleString)(const char*) = NULL;
   PyObject* G__Py_NoneStruct = NULL;                    // used by Py_None
}

// redefine the Py* functions to their equivalent G_* wrappers
#define Py_Initialize         G_Py_Initialize
#define Py_InitModule4        G_Py_InitModule4
#define Py_Finalize           G_Py_Finalize
#define PyErr_Occurred        G_PyErr_Occurred
#define PyErr_SetString       G_PyErr_SetString
#define PyInt_AsLong          G_PyInt_AsLong
#define PyInt_FromLong        G_PyInt_FromLong
#define PyInt_Type            (*G_PyInt_Type)
#define PyList_New            G_PyList_New
#define PyList_Append         G_PyList_Append
#define PyList_GetItem        G_PyList_GetItem
#define PyList_SetItem        G_PyList_SetItem
#define PyList_Size           G_PyList_Size
#define PyList_Type           (*G_PyList_Type)
#define PyTuple_New           G_PyTuple_New
#define PyTuple_SetItem       G_PyTuple_SetItem
#define PyTuple_GetItem       G_PyTuple_GetItem
#define Py_BuildValue         G_Py_BuildValue
#define PyArg_Parse           G_PyArg_Parse
#define PyArg_ParseTuple      G_PyArg_ParseTuple
#define PyDict_GetItemString  G_PyDict_GetItemString
#define PyImport_ImportModule G_PyImport_ImportModule
#define PyModule_GetDict      G_PyModule_GetDict
#define PyRun_SimpleString    G_PyRun_SimpleString
#define _Py_NoneStruct        (*G__Py_NoneStruct)

#ifdef __WXMSW__
   #define PYTHON_PROC FARPROC
#else
   #define PYTHON_PROC void *
#endif
#define PYTHON_FUNC(func) { _T(#func), (PYTHON_PROC*)&G_ ## func },

// store function names and their addresses in Python lib
static struct PythonFunc
{
   const wxChar* name;     // function name
   PYTHON_PROC* ptr;       // function pointer
} pythonFuncs[] =
{
   PYTHON_FUNC(Py_Initialize)
   PYTHON_FUNC(Py_InitModule4)
   PYTHON_FUNC(Py_Finalize)
   PYTHON_FUNC(PyErr_Occurred)
   PYTHON_FUNC(PyErr_SetString)
   PYTHON_FUNC(PyInt_AsLong)
   PYTHON_FUNC(PyInt_FromLong)
   PYTHON_FUNC(PyInt_Type)
   PYTHON_FUNC(PyList_New)
   PYTHON_FUNC(PyList_Append)
   PYTHON_FUNC(PyList_GetItem)
   PYTHON_FUNC(PyList_SetItem)
   PYTHON_FUNC(PyList_Size)
   PYTHON_FUNC(PyList_Type)
   PYTHON_FUNC(PyTuple_New)
   PYTHON_FUNC(PyTuple_SetItem)
   PYTHON_FUNC(PyTuple_GetItem)
   PYTHON_FUNC(Py_BuildValue)
   PYTHON_FUNC(PyArg_Parse)
   PYTHON_FUNC(PyArg_ParseTuple)
   PYTHON_FUNC(PyDict_GetItemString)
   PYTHON_FUNC(PyImport_ImportModule)
   PYTHON_FUNC(PyModule_GetDict)
   PYTHON_FUNC(PyRun_SimpleString)
   PYTHON_FUNC(_Py_NoneStruct)
   { _T(""), NULL }
};

// imported exception objects -- we can't import the symbols from the
// lib as this can cause errors (importing data symbols is not reliable)
static PyObject* imp_PyExc_RuntimeError = NULL;
static PyObject* imp_PyExc_KeyboardInterrupt = NULL;

#define PyExc_RuntimeError imp_PyExc_RuntimeError
#define PyExc_KeyboardInterrupt imp_PyExc_KeyboardInterrupt

static void GetPythonExceptions()
{
   PyObject* exmod = PyImport_ImportModule("exceptions");
   PyObject* exdict = PyModule_GetDict(exmod);
   PyExc_RuntimeError = PyDict_GetItemString(exdict, "RuntimeError");
   PyExc_KeyboardInterrupt = PyDict_GetItemString(exdict, "KeyboardInterrupt");
   Py_XINCREF(PyExc_RuntimeError);
   Py_XINCREF(PyExc_KeyboardInterrupt);
   Py_XDECREF(exmod);
}

// handle for Python lib
static wxDllType pythondll = NULL;

static void FreePythonLib()
{
   if ( pythondll ) {
      wxDynamicLibrary::Unload(pythondll);
      pythondll = NULL;
   }
}

static bool LoadPythonLib()
{
   // load the Python library
   wxDynamicLibrary dynlib;

   // don't log errors in here
   wxLogNull noLog;

   // wxDL_GLOBAL corresponds to RTLD_GLOBAL on Linux (ignored on Windows) and
   // is needed to avoid an ImportError when importing some modules (eg. time)
   while ( !dynlib.Load(pythonlib, wxDL_NOW | wxDL_VERBATIM | wxDL_GLOBAL) ) {
      // prompt user for a different Python library;
      // on Windows pythonlib should be something like "python25.dll"
      // and on Linux it should be something like "libpython2.5.so"
      wxBell();
      wxString str = _("If Python isn't installed then you'll have to Cancel,");
      str +=         _("\notherwise change the version numbers and try again.");
      #ifdef __WXMSW__
         str +=      _("\nDepending on where you installed Python you might have");
         str +=      _("\nto enter a full path like C:\\Python25\\python25.dll.");
      #endif
      wxTextEntryDialog dialog( wxGetActiveWindow(), str,
                                _("Could not load the Python library"),
                                pythonlib, wxOK | wxCANCEL );
      if (dialog.ShowModal() == wxID_OK) {
         pythonlib = dialog.GetValue();
      } else {
         return false;
      }
   }

   if ( dynlib.IsLoaded() ) {
      // load all functions named in pythonFuncs
      void* funcptr;
      PythonFunc* pf = pythonFuncs;
      while ( pf->name[0] ) {
         funcptr = dynlib.GetSymbol(pf->name);
         if ( !funcptr ) {
            wxString err = _("The Python library does not have this symbol:\n");
            err += pf->name;
            Warning(err);
            return false;
         }
         *(pf++->ptr) = (PYTHON_PROC)funcptr;
      }
      pythondll = dynlib.Detach();
   }

   if ( pythondll == NULL ) {
      // should never happen
      Warning(_("Oh dear, the Python library is not loaded!"));
   }

   return pythondll != NULL;
}

#endif // USE_PYTHON_DYNAMIC

// =============================================================================

// some useful macros

#define RETURN_NONE Py_INCREF(Py_None); return Py_None

#define PYTHON_ERROR(msg) { PyErr_SetString(PyExc_RuntimeError, msg); return NULL; }

#if defined(__WXMAC__) && wxCHECK_VERSION(2, 7, 0)
   // use decomposed UTF8 so fopen will work
   #define FILENAME wxString(filename,wxConvLocal).fn_str()
#else
   #define FILENAME filename
#endif

// -----------------------------------------------------------------------------

void AbortPythonScript()
{
   // raise an exception with a special message
   PyErr_SetString(PyExc_KeyboardInterrupt, abortmsg);
}

// -----------------------------------------------------------------------------

bool PythonScriptAborted()
{
   if (allowcheck) wxGetApp().Poller()->checkevents();

   // if user hit escape key then AbortPythonScript has raised an exception
   // and PyErr_Occurred will be true; if so, caller must return NULL
   // otherwise Python can abort app with this message:
   // Fatal Python error: unexpected exception during garbage collection

   return PyErr_Occurred() != NULL;
}

// -----------------------------------------------------------------------------

static void AddTwoInts(PyObject* list, long x, long y)
{
   // append two ints to the given list -- these ints can be:
   // the x,y coords of a live cell in a one-state cell list,
   // or the x,y location of a rect, or the wd,ht of a rect
   PyObject* xo = PyInt_FromLong(x);
   PyObject* yo = PyInt_FromLong(y);
   PyList_Append(list, xo);
   PyList_Append(list, yo);
   // must decrement references to avoid Python memory leak
   Py_DECREF(xo);
   Py_DECREF(yo);
}

// -----------------------------------------------------------------------------

static void AddState(PyObject* list, long s)
{
   // append cell state (possibly dead) to a multi-state cell list
   PyObject* so = PyInt_FromLong(s);
   PyList_Append(list, so);
   Py_DECREF(so);
}

// -----------------------------------------------------------------------------

static void AddPadding(PyObject* list)
{
   // assume list is multi-state and add an extra int if necessary so the list
   // has an odd number of ints (this is how we distinguish multi-state lists
   // from one-state lists -- the latter always have an even number of ints)
   int len = PyList_Size(list);
   if (len == 0) return;         // always return [] rather than [0]
   if ((len & 1) == 0) {
      PyObject* padding = PyInt_FromLong(0L);
      PyList_Append(list, padding);
      Py_DECREF(padding);
   }
}

// -----------------------------------------------------------------------------

static bool ExtractCellList(PyObject* list, lifealgo* universe, bool shift = false)
{
   // extract cell list from given universe
   if ( !universe->isEmpty() ) {
      bigint top, left, bottom, right;
      universe->findedges(&top, &left, &bottom, &right);
      if ( viewptr->OutsideLimits(top, left, bottom, right) ) {
         PyErr_SetString(PyExc_RuntimeError, "Universe is too big to extract all cells!");
         return false;
      }
      bool multistate = universe->NumCellStates() > 2;
      int itop = top.toint();
      int ileft = left.toint();
      int ibottom = bottom.toint();
      int iright = right.toint();
      int cx, cy;
      int v = 0;
      int cntr = 0;
      for ( cy=itop; cy<=ibottom; cy++ ) {
         for ( cx=ileft; cx<=iright; cx++ ) {
            int skip = universe->nextcell(cx, cy, v);
            if (skip >= 0) {
               // found next live cell in this row
               cx += skip;
               if (shift) {
                  // shift cells so that top left cell of bounding box is at 0,0
                  AddTwoInts(list, cx - ileft, cy - itop);
               } else {
                  AddTwoInts(list, cx, cy);
               }
               if (multistate) AddState(list, v);
            } else {
               cx = iright;  // done this row
            }
            cntr++;
            if ((cntr % 4096) == 0 && PythonScriptAborted()) return false;
         }
      }
      if (multistate) AddPadding(list);
   }
   return true;
}

// =============================================================================

// The following py_* routines can be called from Python scripts; some are
// based on code in PLife's lifeint.cc (see http://plife.sourceforge.net/).

static PyObject* py_open(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* filename;
   int remember = 0;

   if (!PyArg_ParseTuple(args, "s|i", &filename, &remember)) return NULL;
   
   const char* err = GSF_open(filename, remember);
   if (err) PYTHON_ERROR(err);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_save(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* filename;
   char* format;
   int remember = 0;

   if (!PyArg_ParseTuple(args, "ss|i", &filename, &format, &remember)) return NULL;
   
   const char* err = GSF_save(filename, format, remember);
   if (err) PYTHON_ERROR(err);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_load(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* filename;

   if (!PyArg_ParseTuple(args, "s", &filename)) return NULL;

   // create temporary universe of same type as current universe
   lifealgo* tempalgo = CreateNewUniverse(currlayer->algtype, allowcheck);
   // readpattern will call setrule
   // tempalgo->setrule(currlayer->algo->getrule());

   // readpattern might change rule
   wxString oldrule = wxString(currlayer->algo->getrule(), wxConvLocal);

   // read pattern into temporary universe
   const char* err = readpattern(FILENAME, *tempalgo);
   if (err) {
      // try all other algos until readpattern succeeds
      for (int i = 0; i < NumAlgos(); i++) {
         if (i != currlayer->algtype) {
            delete tempalgo;
            tempalgo = CreateNewUniverse((algo_type) i, allowcheck);
            err = readpattern(FILENAME, *tempalgo);
            if (!err) break;
         }
      }
   }

   // restore rule
   currlayer->algo->setrule( oldrule.mb_str(wxConvLocal) );

   if (err) {
      delete tempalgo;
      PYTHON_ERROR(err);
   }

   // convert pattern into a cell list, shifting cell coords so that the
   // bounding box's top left cell is at 0,0
   PyObject* outlist = PyList_New(0);
   bool done = ExtractCellList(outlist, tempalgo, true);
   delete tempalgo;
   if (!done) {
      Py_DECREF(outlist);
      return NULL;
   }

   return outlist;
}

// -----------------------------------------------------------------------------

static PyObject* py_store(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   PyObject* inlist;
   char* filename;
   char* desc = NULL;      // the description string is currently ignored!!!

   if (!PyArg_ParseTuple(args, "O!s|s", &PyList_Type, &inlist, &filename, &desc))
      return NULL;

   // create temporary universe of same type as current universe
   lifealgo* tempalgo = CreateNewUniverse(currlayer->algtype, allowcheck);

   // copy cell list into temporary universe
   bool multistate = (PyList_Size(inlist) & 1) == 1;
   int ints_per_cell = multistate ? 3 : 2;
   int num_cells = PyList_Size(inlist) / ints_per_cell;
   for (int n = 0; n < num_cells; n++) {
      int item = ints_per_cell * n;
      long x = PyInt_AsLong( PyList_GetItem(inlist, item) );
      long y = PyInt_AsLong( PyList_GetItem(inlist, item + 1) );
      if (multistate) {
         long state = PyInt_AsLong( PyList_GetItem(inlist, item + 2) );
         if (tempalgo->setcell(x, y, state) < 0) {
            tempalgo->endofpattern();
            delete tempalgo;
            PYTHON_ERROR("store error: state value is out of range.");
         }
      } else {
         tempalgo->setcell(x, y, 1);
      }
      if ((n % 4096) == 0 && PythonScriptAborted()) {
         tempalgo->endofpattern();
         delete tempalgo;
         return NULL;
      }
   }
   tempalgo->endofpattern();

   // write pattern to given file in RLE/XRLE format
   bigint top, left, bottom, right;
   tempalgo->findedges(&top, &left, &bottom, &right);
   const char* err = writepattern(FILENAME, *tempalgo,
                        savexrle ? XRLE_format : RLE_format,
                        top.toint(), left.toint(), bottom.toint(), right.toint());
   delete tempalgo;
   if (err) PYTHON_ERROR(err);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_appdir(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("s", (const char*)gollydir.mb_str(wxConvLocal));
}

// -----------------------------------------------------------------------------

static PyObject* py_datadir(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("s", (const char*)datadir.mb_str(wxConvLocal));
}

// -----------------------------------------------------------------------------

static PyObject* py_new(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* title;

   if (!PyArg_ParseTuple(args, "s", &title)) return NULL;

   mainptr->NewPattern(wxString(title,wxConvLocal));
   DoAutoUpdate();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_cut(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   if (viewptr->SelectionExists()) {
      viewptr->CutSelection();
      DoAutoUpdate();
   } else {
      PYTHON_ERROR("cut error: no selection.");
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_copy(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   if (viewptr->SelectionExists()) {
      viewptr->CopySelection();
      DoAutoUpdate();
   } else {
      PYTHON_ERROR("copy error: no selection.");
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_clear(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int where;

   if (!PyArg_ParseTuple(args, "i", &where)) return NULL;

   if (viewptr->SelectionExists()) {
      if (where == 0)
         viewptr->ClearSelection();
      else
         viewptr->ClearOutsideSelection();
      DoAutoUpdate();
   } else {
      PYTHON_ERROR("clear error: no selection.");
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_paste(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int x, y;
   char* mode;

   if (!PyArg_ParseTuple(args, "iis", &x, &y, &mode)) return NULL;

   if (!mainptr->ClipboardHasText()) {
      PYTHON_ERROR("paste error: no pattern in clipboard.");
   }

   // temporarily change selection and paste mode
   Selection oldsel = currlayer->currsel;
   const char* oldmode = GetPasteMode();

   wxString modestr = wxString(mode, wxConvLocal);
   if      (modestr.IsSameAs(wxT("copy"), false)) SetPasteMode("Copy");
   else if (modestr.IsSameAs(wxT("or"), false))   SetPasteMode("Or");
   else if (modestr.IsSameAs(wxT("xor"), false))  SetPasteMode("Xor");
   else {
      PYTHON_ERROR("paste error: unknown mode.");
   }

   // create huge selection rect so no possibility of error message
   currlayer->currsel.SetRect(x, y, INT_MAX, INT_MAX);

   viewptr->PasteClipboard(true);      // true = paste to selection

   // restore selection and paste mode
   currlayer->currsel = oldsel;
   SetPasteMode(oldmode);

   DoAutoUpdate();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_shrink(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   if (viewptr->SelectionExists()) {
      viewptr->ShrinkSelection(false);    // false == don't fit in viewport
      DoAutoUpdate();
   } else {
      PYTHON_ERROR("shrink error: no selection.");
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_randfill(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int perc;

   if (!PyArg_ParseTuple(args, "i", &perc)) return NULL;

   if (perc < 1 || perc > 100) {
      PYTHON_ERROR("randfill error: percentage must be from 1 to 100.");
   }

   if (viewptr->SelectionExists()) {
      int oldperc = randomfill;
      randomfill = perc;
      viewptr->RandomFill();
      randomfill = oldperc;
      DoAutoUpdate();
   } else {
      PYTHON_ERROR("randfill error: no selection.");
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_flip(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int direction;

   if (!PyArg_ParseTuple(args, "i", &direction)) return NULL;

   if (viewptr->SelectionExists()) {
      viewptr->FlipSelection(direction != 0);    // 1 = top-bottom
      DoAutoUpdate();
   } else {
      PYTHON_ERROR("flip error: no selection.");
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_rotate(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int direction;

   if (!PyArg_ParseTuple(args, "i", &direction)) return NULL;

   if (viewptr->SelectionExists()) {
      viewptr->RotateSelection(direction == 0);    // 0 = clockwise
      DoAutoUpdate();
   } else {
      PYTHON_ERROR("rotate error: no selection.");
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_parse(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* s;

   // defaults for optional params
   long x0  = 0;
   long y0  = 0;
   long axx = 1;
   long axy = 0;
   long ayx = 0;
   long ayy = 1;

   if (!PyArg_ParseTuple(args, "s|llllll", &s, &x0, &y0, &axx, &axy, &ayx, &ayy))
      return NULL;

   PyObject* outlist = PyList_New(0);

   long x = 0, y = 0;

   if (strchr(s, '*')) {
      // parsing 'visual' format
      int c = *s++;
      while (c) {
         switch (c) {
         case '\n': if (x) { x = 0; y++; } break;
         case '.': x++; break;
         case '*':
            AddTwoInts(outlist, x0 + x * axx + y * axy, y0 + x * ayx + y * ayy);
            x++;
            break;
         }
         c = *s++;
      }
   } else {
      // parsing RLE format; first check if multi-state data is present
      bool multistate = false;
      char* p = s;
      while (*p) {
         char c = *p++;
         if ((c == '.') || ('p' <= c && c <= 'y') || ('A' <= c && c <= 'X')) {
            multistate = true;
            break;
         }
      }
      int prefix = 0;
      bool done = false;
      int c = *s++;
      while (c && !done) {
         if (isdigit(c))
            prefix = 10 * prefix + (c - '0');
         else {
            prefix += (prefix == 0);
            switch (c) {
            case '!': done = true; break;
            case '$': x = 0; y += prefix; break;
            case 'b': x += prefix; break;
            case '.': x += prefix; break;
            case 'o':
               for (int k = 0; k < prefix; k++, x++) {
                  AddTwoInts(outlist, x0 + x * axx + y * axy, y0 + x * ayx + y * ayy);
                  if (multistate) AddState(outlist, 1);
               }
               break;
            default:
               if (('p' <= c && c <= 'y') || ('A' <= c && c <= 'X')) {
                  // multistate must be true
                  int state;
                  if (c < 'p') {
                     state = c - 'A' + 1;
                  } else {
                     state = 24 * (c - 'p' + 1);
                     c = *s++;
                     if ('A' <= c && c <= 'X') {
                        state = state + c - 'A' + 1;
                     } else {
                        Py_DECREF(outlist);
                        PYTHON_ERROR("parse error: illegal multi-char state.");
                     }
                  }
                  for (int k = 0; k < prefix; k++, x++) {
                     AddTwoInts(outlist, x0 + x * axx + y * axy, y0 + x * ayx + y * ayy);
                     AddState(outlist, state);
                  }
               }
            }
            prefix = 0;
         }
         c = *s++;
      }
      if (multistate) AddPadding(outlist);
   }

   return outlist;
}

// -----------------------------------------------------------------------------

static PyObject* py_transform(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   PyObject* inlist;
   long x0, y0;

   // defaults for optional params
   long axx = 1;
   long axy = 0;
   long ayx = 0;
   long ayy = 1;

   if (!PyArg_ParseTuple(args, "O!ll|llll", &PyList_Type, &inlist, &x0, &y0, &axx, &axy, &ayx, &ayy))
      return NULL;

   PyObject* outlist = PyList_New(0);

   bool multistate = (PyList_Size(inlist) & 1) == 1;
   int ints_per_cell = multistate ? 3 : 2;
   int num_cells = PyList_Size(inlist) / ints_per_cell;
   for (int n = 0; n < num_cells; n++) {
      int item = ints_per_cell * n;
      long x = PyInt_AsLong( PyList_GetItem(inlist, item) );
      long y = PyInt_AsLong( PyList_GetItem(inlist, item + 1) );
      AddTwoInts(outlist, x0 + x * axx + y * axy,
                          y0 + x * ayx + y * ayy);
      if (multistate) {
         long state = PyInt_AsLong( PyList_GetItem(inlist, item + 2) );
         AddState(outlist, state);
      }
      if ((n % 4096) == 0 && PythonScriptAborted()) {
         Py_DECREF(outlist);
         return NULL;
      }
   }
   if (multistate) AddPadding(outlist);

   return outlist;
}

// -----------------------------------------------------------------------------

static PyObject* py_evolve(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int ngens = 0;
   PyObject* inlist;

   if (!PyArg_ParseTuple(args, "O!i", &PyList_Type, &inlist, &ngens)) return NULL;

   // create a temporary universe of same type as current universe
   lifealgo* tempalgo = CreateNewUniverse(currlayer->algtype, allowcheck);
   tempalgo->setrule(currlayer->algo->getrule());

   // copy cell list into temporary universe
   bool multistate = (PyList_Size(inlist) & 1) == 1;
   int ints_per_cell = multistate ? 3 : 2;
   int num_cells = PyList_Size(inlist) / ints_per_cell;
   for (int n = 0; n < num_cells; n++) {
      int item = ints_per_cell * n;
      long x = PyInt_AsLong( PyList_GetItem(inlist, item) );
      long y = PyInt_AsLong( PyList_GetItem(inlist, item + 1) );
      if (multistate) {
         long state = PyInt_AsLong( PyList_GetItem(inlist, item + 2) );
         if (tempalgo->setcell(x, y, state) < 0) {
            tempalgo->endofpattern();
            delete tempalgo;
            PYTHON_ERROR("evolve error: state value is out of range.");
         }
      } else {
         tempalgo->setcell(x, y, 1);
      }
      if ((n % 4096) == 0 && PythonScriptAborted()) {
         tempalgo->endofpattern();
         delete tempalgo;
         return NULL;
      }
   }
   tempalgo->endofpattern();

   // advance pattern by ngens
   mainptr->generating = true;
   tempalgo->setIncrement(ngens);
   tempalgo->step();
   mainptr->generating = false;

   // convert new pattern into a new cell list
   PyObject* outlist = PyList_New(0);
   bool done = ExtractCellList(outlist, tempalgo);
   delete tempalgo;
   if (!done) {
      Py_DECREF(outlist);
      return NULL;
   }

   return outlist;
}

// -----------------------------------------------------------------------------

static const char* BAD_STATE = "putcells error: state value is out of range.";

static PyObject* py_putcells(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   PyObject* list;

   // defaults for optional params
   long x0  = 0;
   long y0  = 0;
   long axx = 1;
   long axy = 0;
   long ayx = 0;
   long ayy = 1;
   // default for mode is 'or'; 'xor' mode is also supported;
   // 'copy' mode currently has the same effect as 'or' mode
   // because there is no bounding box to set OFF cells
   char* mode = "or";

   if (!PyArg_ParseTuple(args, "O!|lllllls", &PyList_Type, &list, &x0, &y0, &axx, &axy, &ayx, &ayy, &mode))
      return NULL;

   wxString modestr = wxString(mode, wxConvLocal);
   if ( !(modestr.IsSameAs(wxT("or"), false)
          || modestr.IsSameAs(wxT("xor"), false)
          || modestr.IsSameAs(wxT("copy"), false)
          || modestr.IsSameAs(wxT("not"), false)) ) {
      PYTHON_ERROR("putcells error: unknown mode.");
   }
   
   // save cell changes if undo/redo is enabled and script isn't constructing a pattern
   bool savecells = allowundo && !currlayer->stayclean;
   // use ChangeCell below and combine all changes due to consecutive setcell/putcells
   // if (savecells) SavePendingChanges();

   bool multistate = (PyList_Size(list) & 1) == 1;
   int ints_per_cell = multistate ? 3 : 2;
   int num_cells = PyList_Size(list) / ints_per_cell;
   bool abort = false;
   bool pattchanged = false;
   lifealgo* curralgo = currlayer->algo;
   
   if (modestr.IsSameAs(wxT("copy"), false)) {
      // TODO: find bounds of cell list and call ClearRect here (to be added to wxedit.cpp)
   }

   if (modestr.IsSameAs(wxT("xor"), false)) {
      // loop code is duplicated here to allow 'or' case to execute faster
      int numstates = curralgo->NumCellStates();
      for (int n = 0; n < num_cells; n++) {
         int item = ints_per_cell * n;
         long x = PyInt_AsLong( PyList_GetItem(list, item) );
         long y = PyInt_AsLong( PyList_GetItem(list, item + 1) );
         int newx = x0 + x * axx + y * axy;
         int newy = y0 + x * ayx + y * ayy;
         int oldstate = curralgo->getcell(newx, newy);
         int newstate;
         if (multistate) {
            // multi-state lists can contain dead cells so newstate might be 0
            newstate = PyInt_AsLong( PyList_GetItem(list, item + 2) );
            if (newstate == oldstate) {
               if (oldstate != 0) newstate = 0;
            } else {
               newstate = newstate ^ oldstate;
               // if xor overflows then don't change current state
               if (newstate >= numstates) newstate = oldstate;
            }
            if (newstate != oldstate) {
               // paste (possibly transformed) cell into current universe
               if (curralgo->setcell(newx, newy, newstate) < 0) {
                  PyErr_SetString(PyExc_RuntimeError, BAD_STATE);
                  abort = true;
                  break;
               }
               if (savecells) ChangeCell(newx, newy, oldstate, newstate);
               pattchanged = true;
            }
         } else {
            // one-state lists only contain live cells
            newstate = 1 - oldstate;
            // paste (possibly transformed) cell into current universe
            if (curralgo->setcell(newx, newy, newstate) < 0) {
               PyErr_SetString(PyExc_RuntimeError, BAD_STATE);
               abort = true;
               break;
            }
            if (savecells) ChangeCell(newx, newy, oldstate, newstate);
            pattchanged = true;
         }
         if ((n % 4096) == 0 && PythonScriptAborted()) {
            abort = true;
            break;
         }
      }
   } else {
      bool negate = modestr.IsSameAs(wxT("not"), false);
      int newstate = negate ? 0 : 1;
      int maxstate = curralgo->NumCellStates() - 1;
      for (int n = 0; n < num_cells; n++) {
         int item = ints_per_cell * n;
         long x = PyInt_AsLong( PyList_GetItem(list, item) );
         long y = PyInt_AsLong( PyList_GetItem(list, item + 1) );
         int newx = x0 + x * axx + y * axy;
         int newy = y0 + x * ayx + y * ayy;
         int oldstate = curralgo->getcell(newx, newy);
         if (multistate) {
            // multi-state lists can contain dead cells so newstate might be 0
            newstate = PyInt_AsLong( PyList_GetItem(list, item + 2) );
            if (negate) newstate = maxstate - newstate;
         }
         if (newstate != oldstate) {
            // paste (possibly transformed) cell into current universe
            if (curralgo->setcell(newx, newy, newstate) < 0) {
               PyErr_SetString(PyExc_RuntimeError, BAD_STATE);
               abort = true;
               break;
            }
            if (savecells) ChangeCell(newx, newy, oldstate, newstate);
            pattchanged = true;
         }
         if ((n % 4096) == 0 && PythonScriptAborted()) {
            abort = true;
            break;
         }
      }
   }

   if (pattchanged) {
      curralgo->endofpattern();
      MarkLayerDirty();
      DoAutoUpdate();
   }
   
   if (abort) return NULL;

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getcells(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   PyObject* rect_list;

   if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &rect_list)) return NULL;

   // convert pattern in given rect into a cell list
   PyObject* outlist = PyList_New(0);

   int numitems = PyList_Size(rect_list);
   if (numitems == 0) {
      // return empty cell list
   } else if (numitems == 4) {
      int ileft = PyInt_AsLong( PyList_GetItem(rect_list, 0) );
      int itop = PyInt_AsLong( PyList_GetItem(rect_list, 1) );
      int wd = PyInt_AsLong( PyList_GetItem(rect_list, 2) );
      int ht = PyInt_AsLong( PyList_GetItem(rect_list, 3) );
      // first check that wd & ht are > 0
      if (wd <= 0) {
         Py_DECREF(outlist);
         PYTHON_ERROR("getcells error: width must be > 0.");
      }
      if (ht <= 0) {
         Py_DECREF(outlist);
         PYTHON_ERROR("getcells error: height must be > 0.");
      }
      int iright = ileft + wd - 1;
      int ibottom = itop + ht - 1;
      int cx, cy;
      int v = 0;
      int cntr = 0;
      lifealgo* curralgo = currlayer->algo;
      bool multistate = curralgo->NumCellStates() > 2;
      for ( cy=itop; cy<=ibottom; cy++ ) {
         for ( cx=ileft; cx<=iright; cx++ ) {
            int skip = curralgo->nextcell(cx, cy, v);
            if (skip >= 0) {
               // found next live cell in this row
               cx += skip;
               if (cx <= iright) {
                  AddTwoInts(outlist, cx, cy);
                  if (multistate) AddState(outlist, v);
               }
            } else {
               cx = iright;  // done this row
            }
            cntr++;
            if ((cntr % 4096) == 0 && PythonScriptAborted()) {
               Py_DECREF(outlist);
               return NULL;
            }
         }
      }
      if (multistate) AddPadding(outlist);
   } else {
      Py_DECREF(outlist);
      PYTHON_ERROR("getcells error: arg must be [] or [x,y,wd,ht].");
   }

   return outlist;
}

// -----------------------------------------------------------------------------

static PyObject* py_join(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   PyObject* inlist1;
   PyObject* inlist2;

   if (!PyArg_ParseTuple(args, "O!O!", &PyList_Type, &inlist1, &PyList_Type, &inlist2))
      return NULL;

   bool multi1 = (PyList_Size(inlist1) & 1) == 1;
   bool multi2 = (PyList_Size(inlist2) & 1) == 1;
   bool multiout = multi1 || multi2;
   int ints_per_cell, num_cells;
   long x, y, state;
   PyObject* outlist = PyList_New(0);

   // append 1st list
   ints_per_cell = multi1 ? 3 : 2;
   num_cells = PyList_Size(inlist1) / ints_per_cell;
   for (int n = 0; n < num_cells; n++) {
      int item = ints_per_cell * n;
      x = PyInt_AsLong( PyList_GetItem(inlist1, item) );
      y = PyInt_AsLong( PyList_GetItem(inlist1, item + 1) );
      if (multi1) {
         state = PyInt_AsLong( PyList_GetItem(inlist1, item + 2) );
      } else {
         state = 1;
      }
      AddTwoInts(outlist, x, y);
      if (multiout) AddState(outlist, state);
      if ((n % 4096) == 0 && PythonScriptAborted()) {
         Py_DECREF(outlist);
         return NULL;
      }
   }

   // append 2nd list
   ints_per_cell = multi2 ? 3 : 2;
   num_cells = PyList_Size(inlist2) / ints_per_cell;
   for (int n = 0; n < num_cells; n++) {
      int item = ints_per_cell * n;
      x = PyInt_AsLong( PyList_GetItem(inlist2, item) );
      y = PyInt_AsLong( PyList_GetItem(inlist2, item + 1) );
      if (multi2) {
         state = PyInt_AsLong( PyList_GetItem(inlist2, item + 2) );
      } else {
         state = 1;
      }
      AddTwoInts(outlist, x, y);
      if (multiout) AddState(outlist, state);
      if ((n % 4096) == 0 && PythonScriptAborted()) {
         Py_DECREF(outlist);
         return NULL;
      }
   }

   if (multiout) AddPadding(outlist);
   
   return outlist;
}

// -----------------------------------------------------------------------------

static PyObject* py_hash(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   PyObject* rect_list;

   if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &rect_list)) return NULL;

   int numitems = PyList_Size(rect_list);
   if (numitems != 4) {
      PYTHON_ERROR("hash error: arg must be [x,y,wd,ht].");
   }

   int x  = PyInt_AsLong( PyList_GetItem(rect_list, 0) );
   int y  = PyInt_AsLong( PyList_GetItem(rect_list, 1) );
   int wd = PyInt_AsLong( PyList_GetItem(rect_list, 2) );
   int ht = PyInt_AsLong( PyList_GetItem(rect_list, 3) );
   // first check that wd & ht are > 0
   if (wd <= 0) PYTHON_ERROR("hash error: width must be > 0.");
   if (ht <= 0) PYTHON_ERROR("hash error: height must be > 0.");
   int right = x + wd - 1;
   int bottom = y + ht - 1;
   int cx, cy;
   int v = 0;
   int cntr = 0;
   
   // calculate a hash value for pattern in given rect
   int hash = 31415962;
   lifealgo* curralgo = currlayer->algo;
   for ( cy=y; cy<=bottom; cy++ ) {
      int yshift = cy - y;
      for ( cx=x; cx<=right; cx++ ) {
         int skip = curralgo->nextcell(cx, cy, v);
         if (skip >= 0) {
            // found next live cell in this row
            cx += skip;
            if (cx <= right) {
               //note that v is 1 in a two-state universe
               hash = (hash * 33 + yshift) ^ ((cx - x) * v);
            }
         } else {
            cx = right;  // done this row
         }
         cntr++;
         if ((cntr % 4096) == 0 && PythonScriptAborted()) {
            return NULL;
         }
      }
   }

   return Py_BuildValue("i", hash);
}

// -----------------------------------------------------------------------------

static PyObject* py_getclip(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   if (!mainptr->ClipboardHasText()) {
      PYTHON_ERROR("getclip error: no pattern in clipboard.");
   }

   // convert pattern in clipboard into a cell list, but where the first 2 items
   // are the pattern's width and height (not necessarily the minimal bounding box
   // because the pattern might have empty borders, or it might even be empty)
   PyObject* outlist = PyList_New(0);

   // create a temporary universe for storing clipboard pattern;
   // GetClipboardPattern assumes it is same type as current universe
   lifealgo* tempalgo = CreateNewUniverse(currlayer->algtype, allowcheck);
   tempalgo->setrule(currlayer->algo->getrule());

   // read clipboard pattern into temporary universe and set edges
   // (not a minimal bounding box if pattern is empty or has empty borders)
   bigint top, left, bottom, right;
   if ( viewptr->GetClipboardPattern(&tempalgo, &top, &left, &bottom, &right) ) {
      if ( viewptr->OutsideLimits(top, left, bottom, right) ) {
         Py_DECREF(outlist);
         PYTHON_ERROR("getclip error: pattern is too big.");
      }
      int itop = top.toint();
      int ileft = left.toint();
      int ibottom = bottom.toint();
      int iright = right.toint();
      int wd = iright - ileft + 1;
      int ht = ibottom - itop + 1;

      AddTwoInts(outlist, wd, ht);

      // extract cells from tempalgo
      bool multistate = tempalgo->NumCellStates() > 2;
      int cx, cy;
      int cntr = 0;
      int v = 0;
      for ( cy=itop; cy<=ibottom; cy++ ) {
         for ( cx=ileft; cx<=iright; cx++ ) {
            int skip = tempalgo->nextcell(cx, cy, v);
            if (skip >= 0) {
               // found next live cell in this row
               cx += skip;
               // shift cells so that top left cell of bounding box is at 0,0
               AddTwoInts(outlist, cx - ileft, cy - itop);
               if (multistate) AddState(outlist, v);
            } else {
               cx = iright;  // done this row
            }
            cntr++;
            if ((cntr % 4096) == 0 && PythonScriptAborted()) {
               delete tempalgo;
               Py_DECREF(outlist);
               return NULL;
            }
         }
      }
      // if no live cells then return [wd,ht] rather than [wd,ht,0]
      if (multistate && PyList_Size(outlist) > 2) {
         AddPadding(outlist);
      }

      delete tempalgo;
   } else {
      // assume error message has been displayed
      delete tempalgo;
      Py_DECREF(outlist);
      return NULL;
   }

   return outlist;
}

// -----------------------------------------------------------------------------

static PyObject* py_select(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   PyObject* rect_list;

   if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &rect_list)) return NULL;

   int numitems = PyList_Size(rect_list);
   if (numitems == 0) {
      // remove any existing selection
      GSF_select(0, 0, 0, 0);
   } else if (numitems == 4) {
      int x  = PyInt_AsLong( PyList_GetItem(rect_list, 0) );
      int y  = PyInt_AsLong( PyList_GetItem(rect_list, 1) );
      int wd = PyInt_AsLong( PyList_GetItem(rect_list, 2) );
      int ht = PyInt_AsLong( PyList_GetItem(rect_list, 3) );
      // first check that wd & ht are > 0
      if (wd <= 0) PYTHON_ERROR("select error: width must be > 0.");
      if (ht <= 0) PYTHON_ERROR("select error: height must be > 0.");
      // set selection rect
      GSF_select(x, y, wd, ht);
   } else {
      PYTHON_ERROR("select error: arg must be [] or [x,y,wd,ht].");
   }

   DoAutoUpdate();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getrect(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   PyObject* outlist = PyList_New(0);

   if (!currlayer->algo->isEmpty()) {
      bigint top, left, bottom, right;
      currlayer->algo->findedges(&top, &left, &bottom, &right);
      if ( viewptr->OutsideLimits(top, left, bottom, right) ) {
         Py_DECREF(outlist);
         PYTHON_ERROR("getrect error: pattern is too big.");
      }
      long x = left.toint();
      long y = top.toint();
      long wd = right.toint() - x + 1;
      long ht = bottom.toint() - y + 1;

      AddTwoInts(outlist, x, y);
      AddTwoInts(outlist, wd, ht);
   }

   return outlist;
}

// -----------------------------------------------------------------------------

static PyObject* py_getselrect(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   PyObject* outlist = PyList_New(0);

   if (viewptr->SelectionExists()) {
      if (currlayer->currsel.TooBig()) {
         Py_DECREF(outlist);
         PYTHON_ERROR("getselrect error: selection is too big.");
      }
      int x, y, wd, ht;
      currlayer->currsel.GetRect(&x, &y, &wd, &ht);

      AddTwoInts(outlist, x, y);
      AddTwoInts(outlist, wd, ht);
   }

   return outlist;
}

// -----------------------------------------------------------------------------

static PyObject* py_setcell(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int x, y, state;

   if (!PyArg_ParseTuple(args, "iii", &x, &y, &state)) return NULL;

   const char* err = GSF_setcell(x, y, state);
   if (err) PYTHON_ERROR(err);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getcell(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int x, y;

   if (!PyArg_ParseTuple(args, "ii", &x, &y)) return NULL;

   return Py_BuildValue("i", currlayer->algo->getcell(x, y));
}

// -----------------------------------------------------------------------------

static PyObject* py_setcursor(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int newindex;

   if (!PyArg_ParseTuple(args, "i", &newindex)) return NULL;

   int oldindex = CursorToIndex(currlayer->curs);
   wxCursor* curs = IndexToCursor(newindex);
   if (curs) {
      viewptr->SetCursorMode(curs);
      // see the cursor change, including in tool bar
      mainptr->UpdateUserInterface(mainptr->IsActive());
   } else {
      PYTHON_ERROR("setcursor error: bad cursor index.");
   }

   // return old index (simplifies saving and restoring cursor)
   return Py_BuildValue("i", oldindex);
}

// -----------------------------------------------------------------------------

static PyObject* py_getcursor(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("i", CursorToIndex(currlayer->curs));
}

// -----------------------------------------------------------------------------

static PyObject* py_empty(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("i", currlayer->algo->isEmpty() ? 1 : 0);
}

// -----------------------------------------------------------------------------

static PyObject* py_run(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int ngens;

   if (!PyArg_ParseTuple(args, "i", &ngens)) return NULL;

   if (ngens > 0 && !currlayer->algo->isEmpty()) {
      if (ngens > 1) {
         bigint saveinc = currlayer->algo->getIncrement();
         currlayer->algo->setIncrement(ngens);
         mainptr->NextGeneration(true);            // step by ngens
         currlayer->algo->setIncrement(saveinc);
      } else {
         mainptr->NextGeneration(false);           // step 1 gen
      }
      DoAutoUpdate();
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_step(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   if (!currlayer->algo->isEmpty()) {
      mainptr->NextGeneration(true);      // step by current increment
      DoAutoUpdate();
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_setstep(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int exp;

   if (!PyArg_ParseTuple(args, "i", &exp)) return NULL;

   mainptr->SetWarp(exp);
   DoAutoUpdate();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getstep(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("i", currlayer->warp);
}

// -----------------------------------------------------------------------------

static PyObject* py_setbase(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int base;

   if (!PyArg_ParseTuple(args, "i", &base)) return NULL;

   if (base < 2) base = 2;
   if (base > MAX_BASESTEP) base = MAX_BASESTEP;
   currlayer->algodata->algobase = base;
   mainptr->UpdateWarp();
   DoAutoUpdate();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getbase(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("i", currlayer->algodata->algobase);
}

// -----------------------------------------------------------------------------

static PyObject* py_advance(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int where, ngens;

   if (!PyArg_ParseTuple(args, "ii", &where, &ngens)) return NULL;

   if (ngens > 0) {
      if (viewptr->SelectionExists()) {
         while (ngens > 0) {
            ngens--;
            if (where == 0)
               currlayer->currsel.Advance();
            else
               currlayer->currsel.AdvanceOutside();
         }
         DoAutoUpdate();
      } else {
         PYTHON_ERROR("advance error: no selection.");
      }
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_reset(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   if (currlayer->algo->getGeneration() != currlayer->startgen) {
      mainptr->ResetPattern();
      DoAutoUpdate();
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_setgen(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* genstring = NULL;

   if (!PyArg_ParseTuple(args, "s", &genstring)) return NULL;

   const char* err = GSF_setgen(genstring);
   if (err) PYTHON_ERROR(err);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getgen(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char sepchar = '\0';

   if (!PyArg_ParseTuple(args, "|c", &sepchar)) return NULL;

   return Py_BuildValue("s", currlayer->algo->getGeneration().tostring(sepchar));
}

// -----------------------------------------------------------------------------

static PyObject* py_getpop(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char sepchar = '\0';

   if (!PyArg_ParseTuple(args, "|c", &sepchar)) return NULL;

   return Py_BuildValue("s", currlayer->algo->getPopulation().tostring(sepchar));
}

// -----------------------------------------------------------------------------

static PyObject* py_setalgo(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* algostring = NULL;

   if (!PyArg_ParseTuple(args, "s", &algostring)) return NULL;

   const char* err = GSF_setalgo(algostring);
   if (err) PYTHON_ERROR(err);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getalgo(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int index = currlayer->algtype;

   if (!PyArg_ParseTuple(args, "|i", &index)) return NULL;

   if (index < 0 || index >= NumAlgos()) {
      char msg[64];
      sprintf(msg, "Bad getalgo index: %d", index);
      PYTHON_ERROR(msg);
   }

   return Py_BuildValue("s", GetAlgoName(index));
}

// -----------------------------------------------------------------------------

static PyObject* py_setrule(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* rulestring = NULL;

   if (!PyArg_ParseTuple(args, "s", &rulestring)) return NULL;

   const char* err = GSF_setrule(rulestring);
   if (err) PYTHON_ERROR(err);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getrule(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("s", currlayer->algo->getrule());
}

// -----------------------------------------------------------------------------

static PyObject* py_numstates(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("i", currlayer->algo->NumCellStates());
}

// -----------------------------------------------------------------------------

static PyObject* py_numalgos(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("i", NumAlgos());
}

// -----------------------------------------------------------------------------

static PyObject* py_setpos(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* x;
   char* y;

   if (!PyArg_ParseTuple(args, "ss", &x, &y)) return NULL;

   const char* err = GSF_setpos(x, y);
   if (err) PYTHON_ERROR(err);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getpos(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char sepchar = '\0';

   if (!PyArg_ParseTuple(args, "|c", &sepchar)) return NULL;

   bigint bigx, bigy;
   viewptr->GetPos(bigx, bigy);

   // return position as x,y tuple
   PyObject* xytuple = PyTuple_New(2);
   PyTuple_SetItem(xytuple, 0, Py_BuildValue("s",bigx.tostring(sepchar)));
   PyTuple_SetItem(xytuple, 1, Py_BuildValue("s",bigy.tostring(sepchar)));
   return xytuple;
}

// -----------------------------------------------------------------------------

static PyObject* py_setmag(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int mag;

   if (!PyArg_ParseTuple(args, "i", &mag)) return NULL;

   viewptr->SetMag(mag);
   DoAutoUpdate();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getmag(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("i", viewptr->GetMag());
}

// -----------------------------------------------------------------------------

static PyObject* py_fit(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   viewptr->FitPattern();
   DoAutoUpdate();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_fitsel(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   if (viewptr->SelectionExists()) {
      viewptr->FitSelection();
      DoAutoUpdate();
   } else {
      PYTHON_ERROR("fitsel error: no selection.");
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_visrect(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   PyObject* rect_list;

   if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &rect_list)) return NULL;

   int numitems = PyList_Size(rect_list);
   if (numitems != 4) {
      PYTHON_ERROR("visrect error: arg must be [x,y,wd,ht].");
   }

   int x = PyInt_AsLong( PyList_GetItem(rect_list, 0) );
   int y = PyInt_AsLong( PyList_GetItem(rect_list, 1) );
   int wd = PyInt_AsLong( PyList_GetItem(rect_list, 2) );
   int ht = PyInt_AsLong( PyList_GetItem(rect_list, 3) );
   // check that wd & ht are > 0
   if (wd <= 0) PYTHON_ERROR("visrect error: width must be > 0.");
   if (ht <= 0) PYTHON_ERROR("visrect error: height must be > 0.");

   bigint left = x;
   bigint top = y;
   bigint right = x + wd - 1;
   bigint bottom = y + ht - 1;
   int visible = viewptr->CellVisible(left, top) &&
                 viewptr->CellVisible(right, bottom);

   return Py_BuildValue("i", visible);
}

// -----------------------------------------------------------------------------

static PyObject* py_update(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   GSF_update();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_autoupdate(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int flag;

   if (!PyArg_ParseTuple(args, "i", &flag)) return NULL;

   autoupdate = (flag != 0);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_addlayer(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   if (numlayers >= MAX_LAYERS) {
      PYTHON_ERROR("addlayer error: no more layers can be added.");
   } else {
      AddLayer();
      DoAutoUpdate();
   }

   // return index of new layer
   return Py_BuildValue("i", currindex);
}

// -----------------------------------------------------------------------------

static PyObject* py_clone(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   if (numlayers >= MAX_LAYERS) {
      PYTHON_ERROR("clone error: no more layers can be added.");
   } else {
      CloneLayer();
      DoAutoUpdate();
   }

   // return index of new layer
   return Py_BuildValue("i", currindex);
}

// -----------------------------------------------------------------------------

static PyObject* py_duplicate(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   if (numlayers >= MAX_LAYERS) {
      PYTHON_ERROR("duplicate error: no more layers can be added.");
   } else {
      DuplicateLayer();
      DoAutoUpdate();
   }

   // return index of new layer
   return Py_BuildValue("i", currindex);
}

// -----------------------------------------------------------------------------

static PyObject* py_dellayer(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   if (numlayers <= 1) {
      PYTHON_ERROR("dellayer error: there is only one layer.");
   } else {
      DeleteLayer();
      DoAutoUpdate();
   }

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_movelayer(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int fromindex, toindex;

   if (!PyArg_ParseTuple(args, "ii", &fromindex, &toindex)) return NULL;

   if (fromindex < 0 || fromindex >= numlayers) {
      char msg[64];
      sprintf(msg, "Bad movelayer fromindex: %d", fromindex);
      PYTHON_ERROR(msg);
   }
   if (toindex < 0 || toindex >= numlayers) {
      char msg[64];
      sprintf(msg, "Bad movelayer toindex: %d", toindex);
      PYTHON_ERROR(msg);
   }

   MoveLayer(fromindex, toindex);
   DoAutoUpdate();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_setlayer(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int index;

   if (!PyArg_ParseTuple(args, "i", &index)) return NULL;

   if (index < 0 || index >= numlayers) {
      char msg[64];
      sprintf(msg, "Bad setlayer index: %d", index);
      PYTHON_ERROR(msg);
   }

   SetLayer(index);
   DoAutoUpdate();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getlayer(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("i", currindex);
}

// -----------------------------------------------------------------------------

static PyObject* py_numlayers(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("i", numlayers);
}

// -----------------------------------------------------------------------------

static PyObject* py_maxlayers(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   return Py_BuildValue("i", MAX_LAYERS);
}

// -----------------------------------------------------------------------------

static PyObject* py_setname(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* name;
   int index = currindex;

   if (!PyArg_ParseTuple(args, "s|i", &name, &index)) return NULL;

   if (index < 0 || index >= numlayers) {
      char msg[64];
      sprintf(msg, "Bad setname index: %d", index);
      PYTHON_ERROR(msg);
   }

   GSF_setname(name, index);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_getname(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   int index = currindex;

   if (!PyArg_ParseTuple(args, "|i", &index)) return NULL;

   if (index < 0 || index >= numlayers) {
      char msg[64];
      sprintf(msg, "Bad getname index: %d", index);
      PYTHON_ERROR(msg);
   }

   // need to be careful converting Unicode wxString to char*
   wxCharBuffer name = GetLayer(index)->currname.mb_str(wxConvLocal);
   return Py_BuildValue("s", (const char*)name);
}

// -----------------------------------------------------------------------------

static PyObject* py_setoption(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* optname;
   int oldval, newval;

   if (!PyArg_ParseTuple(args, "si", &optname, &newval)) return NULL;

   if (!GSF_setoption(optname, newval, &oldval)) {
      PYTHON_ERROR("setoption error: unknown option.");
   }

   // return old value (simplifies saving and restoring settings)
   return Py_BuildValue("i", oldval);
}

// -----------------------------------------------------------------------------

static PyObject* py_getoption(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* optname;
   int optval;

   if (!PyArg_ParseTuple(args, "s", &optname)) return NULL;

   if (!GSF_getoption(optname, &optval)) {
      PYTHON_ERROR("getoption error: unknown option.");
   }

   return Py_BuildValue("i", optval);
}

// -----------------------------------------------------------------------------

static PyObject* py_setcolor(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* colname;
   int r, g, b;

   if (!PyArg_ParseTuple(args, "siii", &colname, &r, &g, &b)) return NULL;

   wxColor newcol(r, g, b);
   wxColor oldcol;

   if (!GSF_setcolor(colname, newcol, oldcol)) {
      PYTHON_ERROR("setcolor error: unknown color.");
   }

   // return old r,g,b values (simplifies saving and restoring colors)
   PyObject* rgbtuple = PyTuple_New(3);
   PyTuple_SetItem(rgbtuple, 0, Py_BuildValue("i",oldcol.Red()));
   PyTuple_SetItem(rgbtuple, 1, Py_BuildValue("i",oldcol.Green()));
   PyTuple_SetItem(rgbtuple, 2, Py_BuildValue("i",oldcol.Blue()));
   return rgbtuple;
}

// -----------------------------------------------------------------------------

static PyObject* py_getcolor(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* colname;
   wxColor color;

   if (!PyArg_ParseTuple(args, "s", &colname)) return NULL;

   if (!GSF_getcolor(colname, color)) {
      PYTHON_ERROR("getcolor error: unknown color.");
   }

   // return r,g,b tuple
   PyObject* rgbtuple = PyTuple_New(3);
   PyTuple_SetItem(rgbtuple, 0, Py_BuildValue("i",color.Red()));
   PyTuple_SetItem(rgbtuple, 1, Py_BuildValue("i",color.Green()));
   PyTuple_SetItem(rgbtuple, 2, Py_BuildValue("i",color.Blue()));
   return rgbtuple;
}

// -----------------------------------------------------------------------------

static PyObject* py_getstring(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* prompt;
   char* initial = "";
   char* title = "";

   if (!PyArg_ParseTuple(args, "s|ss", &prompt, &initial, &title))
      return NULL;

   wxString result;
   if ( !GetString(wxString(title,wxConvLocal), wxString(prompt,wxConvLocal),
                   wxString(initial,wxConvLocal), result) ) {
      // user hit Cancel button
      AbortPythonScript();
      return NULL;
   }

   return Py_BuildValue("s", (const char*)result.mb_str(wxConvLocal));
}

// -----------------------------------------------------------------------------

static PyObject* py_getkey(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);

   if (!PyArg_ParseTuple(args, "")) return NULL;

   char s[2];        // room for char + NULL
   GSF_getkey(s);

   return Py_BuildValue("s", s);
}

// -----------------------------------------------------------------------------

static PyObject* py_dokey(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* ascii = 0;

   if (!PyArg_ParseTuple(args, "s", &ascii)) return NULL;

   GSF_dokey(ascii);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_show(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* s = NULL;

   if (!PyArg_ParseTuple(args, "s", &s)) return NULL;

   inscript = false;
   statusptr->DisplayMessage(wxString(s,wxConvLocal));
   inscript = true;
   // make sure status bar is visible
   if (!showstatus) mainptr->ToggleStatusBar();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_error(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* s = NULL;

   if (!PyArg_ParseTuple(args, "s", &s)) return NULL;

   inscript = false;
   statusptr->ErrorMessage(wxString(s,wxConvLocal));
   inscript = true;
   // make sure status bar is visible
   if (!showstatus) mainptr->ToggleStatusBar();

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_warn(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* s = NULL;

   if (!PyArg_ParseTuple(args, "s", &s)) return NULL;

   Warning(wxString(s,wxConvLocal));

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_note(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* s = NULL;

   if (!PyArg_ParseTuple(args, "s", &s)) return NULL;

   Note(wxString(s,wxConvLocal));

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_help(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* htmlfile = NULL;

   if (!PyArg_ParseTuple(args, "s", &htmlfile)) return NULL;

   ShowHelp(wxString(htmlfile,wxConvLocal));

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_check(PyObject* self, PyObject* args)
{
   // if (PythonScriptAborted()) return NULL;
   // don't call checkevents() here otherwise we can't safely write code like
   //    if g.getlayer() == target:
   //       g.check(0)
   //       ... do stuff to target layer ...
   //       g.check(1)
   wxUnusedVar(self);
   int flag;

   if (!PyArg_ParseTuple(args, "i", &flag)) return NULL;

   allowcheck = (flag != 0);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyObject* py_exit(PyObject* self, PyObject* args)
{
   if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* err = NULL;

   if (!PyArg_ParseTuple(args, "|s", &err)) return NULL;

   GSF_exit(err);
   AbortPythonScript();

   // exception raised so must return NULL
   return NULL;
}

// -----------------------------------------------------------------------------

static PyObject* py_stderr(PyObject* self, PyObject* args)
{
   // probably safer not to call checkevents here
   // if (PythonScriptAborted()) return NULL;
   wxUnusedVar(self);
   char* s = NULL;

   if (!PyArg_ParseTuple(args, "s", &s)) return NULL;

   // accumulate stderr messages in global string (shown after script finishes)
   scripterr = wxString(s, wxConvLocal);

   RETURN_NONE;
}

// -----------------------------------------------------------------------------

static PyMethodDef py_methods[] = {
   // filing
   { "open",         py_open,       METH_VARARGS, "open given pattern file" },
   { "save",         py_save,       METH_VARARGS, "save pattern in given file using given format" },
   { "load",         py_load,       METH_VARARGS, "read pattern file and return cell list" },
   { "store",        py_store,      METH_VARARGS, "write cell list to a file (in RLE format)" },
   { "appdir",       py_appdir,     METH_VARARGS, "return location of Golly app" },
   { "datadir",      py_datadir,    METH_VARARGS, "return location of user-specific data" },
   // editing
   { "new",          py_new,        METH_VARARGS, "create new universe and set window title" },
   { "cut",          py_cut,        METH_VARARGS, "cut selection to clipboard" },
   { "copy",         py_copy,       METH_VARARGS, "copy selection to clipboard" },
   { "clear",        py_clear,      METH_VARARGS, "clear inside/outside selection" },
   { "paste",        py_paste,      METH_VARARGS, "paste clipboard pattern at x,y using given mode" },
   { "shrink",       py_shrink,     METH_VARARGS, "shrink selection" },
   { "randfill",     py_randfill,   METH_VARARGS, "randomly fill selection to given percentage" },
   { "flip",         py_flip,       METH_VARARGS, "flip selection top-bottom or left-right" },
   { "rotate",       py_rotate,     METH_VARARGS, "rotate selection 90 deg clockwise or anticlockwise" },
   { "parse",        py_parse,      METH_VARARGS, "parse RLE or Life 1.05 string and return cell list" },
   { "transform",    py_transform,  METH_VARARGS, "apply an affine transformation to cell list" },
   { "evolve",       py_evolve,     METH_VARARGS, "generate pattern contained in given cell list" },
   { "putcells",     py_putcells,   METH_VARARGS, "paste given cell list into current universe" },
   { "getcells",     py_getcells,   METH_VARARGS, "return cell list in given rectangle" },
   { "join",         py_join,       METH_VARARGS, "return concatenation of given cell lists" },
   { "hash",         py_hash,       METH_VARARGS, "return hash value for pattern in given rectangle" },
   { "getclip",      py_getclip,    METH_VARARGS, "return pattern in clipboard (as cell list)" },
   { "select",       py_select,     METH_VARARGS, "select [x, y, wd, ht] rectangle or remove if []" },
   { "getrect",      py_getrect,    METH_VARARGS, "return pattern rectangle as [] or [x, y, wd, ht]" },
   { "getselrect",   py_getselrect, METH_VARARGS, "return selection rectangle as [] or [x, y, wd, ht]" },
   { "setcell",      py_setcell,    METH_VARARGS, "set given cell to given state" },
   { "getcell",      py_getcell,    METH_VARARGS, "get state of given cell" },
   { "setcursor",    py_setcursor,  METH_VARARGS, "set cursor (returns old cursor)" },
   { "getcursor",    py_getcursor,  METH_VARARGS, "return current cursor" },
   // control
   { "empty",        py_empty,      METH_VARARGS, "return true if universe is empty" },
   { "run",          py_run,        METH_VARARGS, "run current pattern for given number of gens" },
   { "step",         py_step,       METH_VARARGS, "run current pattern for current step" },
   { "setstep",      py_setstep,    METH_VARARGS, "set step exponent" },
   { "getstep",      py_getstep,    METH_VARARGS, "return current step exponent" },
   { "setbase",      py_setbase,    METH_VARARGS, "set base step" },
   { "getbase",      py_getbase,    METH_VARARGS, "return current base step" },
   { "advance",      py_advance,    METH_VARARGS, "advance inside/outside selection by given gens" },
   { "reset",        py_reset,      METH_VARARGS, "restore starting pattern" },
   { "setgen",       py_setgen,     METH_VARARGS, "set current generation to given string" },
   { "getgen",       py_getgen,     METH_VARARGS, "return current generation as string" },
   { "getpop",       py_getpop,     METH_VARARGS, "return current population as string" },
   { "numstates",    py_numstates,  METH_VARARGS, "return number of cell states in current universe" },
   { "numalgos",     py_numalgos,   METH_VARARGS, "return number of algorithms" },
   { "setalgo",      py_setalgo,    METH_VARARGS, "set current algorithm using given string" },
   { "getalgo",      py_getalgo,    METH_VARARGS, "return name of given or current algorithm" },
   { "setrule",      py_setrule,    METH_VARARGS, "set current rule using given string" },
   { "getrule",      py_getrule,    METH_VARARGS, "return current rule" },
   // viewing
   { "setpos",       py_setpos,     METH_VARARGS, "move given cell to middle of viewport" },
   { "getpos",       py_getpos,     METH_VARARGS, "return x,y position of cell in middle of viewport" },
   { "setmag",       py_setmag,     METH_VARARGS, "set magnification (0=1:1, 1=1:2, -1=2:1, etc)" },
   { "getmag",       py_getmag,     METH_VARARGS, "return current magnification" },
   { "fit",          py_fit,        METH_VARARGS, "fit entire pattern in viewport" },
   { "fitsel",       py_fitsel,     METH_VARARGS, "fit selection in viewport" },
   { "visrect",      py_visrect,    METH_VARARGS, "return true if given rect is completely visible" },
   { "update",       py_update,     METH_VARARGS, "update display (viewport and status bar)" },
   { "autoupdate",   py_autoupdate, METH_VARARGS, "update display after each change to universe?" },
   // layers
   { "addlayer",     py_addlayer,   METH_VARARGS, "add a new layer" },
   { "clone",        py_clone,      METH_VARARGS, "add a cloned layer (shares universe)" },
   { "duplicate",    py_duplicate,  METH_VARARGS, "add a duplicate layer (copies universe)" },
   { "dellayer",     py_dellayer,   METH_VARARGS, "delete current layer" },
   { "movelayer",    py_movelayer,  METH_VARARGS, "move given layer to new index" },
   { "setlayer",     py_setlayer,   METH_VARARGS, "switch to given layer" },
   { "getlayer",     py_getlayer,   METH_VARARGS, "return index of current layer" },
   { "numlayers",    py_numlayers,  METH_VARARGS, "return current number of layers" },
   { "maxlayers",    py_maxlayers,  METH_VARARGS, "return maximum number of layers" },
   { "setname",      py_setname,    METH_VARARGS, "set name of given layer" },
   { "getname",      py_getname,    METH_VARARGS, "get name of given layer" },
   // miscellaneous
   { "setoption",    py_setoption,  METH_VARARGS, "set given option to new value (returns old value)" },
   { "getoption",    py_getoption,  METH_VARARGS, "return current value of given option" },
   { "setcolor",     py_setcolor,   METH_VARARGS, "set given color to new r,g,b (returns old r,g,b)" },
   { "getcolor",     py_getcolor,   METH_VARARGS, "return r,g,b values of given color" },
   { "getstring",    py_getstring,  METH_VARARGS, "display dialog box to get string from user" },
   { "getkey",       py_getkey,     METH_VARARGS, "return key hit by user or empty string if none" },
   { "dokey",        py_dokey,      METH_VARARGS, "pass given key to Golly's standard key handler" },
   { "show",         py_show,       METH_VARARGS, "show given string in status bar" },
   { "error",        py_error,      METH_VARARGS, "beep and show given string in status bar" },
   { "warn",         py_warn,       METH_VARARGS, "show given string in warning dialog" },
   { "note",         py_note,       METH_VARARGS, "show given string in note dialog" },
   { "help",         py_help,       METH_VARARGS, "show given HTML file in help window" },
   { "check",        py_check,      METH_VARARGS, "allow event checking?" },
   { "exit",         py_exit,       METH_VARARGS, "exit script with optional error message" },
   // for internal use (don't document)
   { "stderr",       py_stderr,     METH_VARARGS, "save Python error message" },
   { NULL, NULL, 0, NULL }
};

// =============================================================================

bool pyinited = false;     // InitPython has been successfully called?

bool InitPython()
{
   if (!pyinited) {
      #ifdef USE_PYTHON_DYNAMIC
         // try to load Python library
         if ( !LoadPythonLib() ) return false;
      #endif

      // only initialize the Python interpreter once, mainly because multiple
      // Py_Initialize/Py_Finalize calls cause leaks of about 12K each time!
      Py_Initialize();

      #ifdef USE_PYTHON_DYNAMIC
         GetPythonExceptions();
      #endif

      // allow Python to call the above py_* routines
      Py_InitModule("golly", py_methods);

      // catch Python messages sent to stderr and pass them to py_stderr
      if ( PyRun_SimpleString(
            "import golly\n"
            "import sys\n"
            "class StderrCatcher:\n"
            "   def __init__(self):\n"
            "      self.data = ''\n"
            "   def write(self, stuff):\n"
            "      self.data += stuff\n"
            "      golly.stderr(self.data)\n"
            "sys.stderr = StderrCatcher()\n"

            // also create dummy sys.argv so scripts can import Tkinter
            "sys.argv = ['golly-app']\n"
            // works, but Golly's menus get permanently changed on Mac
            ) < 0
         ) Warning(_("StderrCatcher code failed!"));

      // build absolute path to Scripts/Python folder and add to Python's
      // import search list so scripts can import glife from anywhere
      wxString scriptsdir = gollydir + _("Scripts");
      scriptsdir += wxFILE_SEP_PATH;
      scriptsdir += _("Python");
      // convert any \ to \\ and then convert any ' to \'
      scriptsdir.Replace(wxT("\\"), wxT("\\\\"));
      scriptsdir.Replace(wxT("'"), wxT("\\'"));
      wxString command = wxT("import sys ; sys.path.append('") + scriptsdir + wxT("')");
      if ( PyRun_SimpleString(command.mb_str(wxConvLocal)) < 0 )
         Warning(_("Failed to append Scripts path!"));

      // nicer to reload all modules in case changes were made by user;
      // code comes from http://pyunit.sourceforge.net/notes/reloading.html
      /* unfortunately it causes an AttributeError
      if ( PyRun_SimpleString(
            "import __builtin__\n"
            "class RollbackImporter:\n"
            "   def __init__(self):\n"
            "      self.previousModules = sys.modules.copy()\n"
            "      self.realImport = __builtin__.__import__\n"
            "      __builtin__.__import__ = self._import\n"
            "      self.newModules = {}\n"
            "   def _import(self, name, globals=None, locals=None, fromlist=[]):\n"
            "      result = apply(self.realImport, (name, globals, locals, fromlist))\n"
            "      self.newModules[name] = 1\n"
            "      return result\n"
            "   def uninstall(self):\n"
            "      for modname in self.newModules.keys():\n"
            "         if not self.previousModules.has_key(modname):\n"
            "            del(sys.modules[modname])\n"
            "      __builtin__.__import__ = self.realImport\n"
            "rollbackImporter = RollbackImporter()\n"
            ) < 0
         ) Warning(_("RollbackImporter code failed!"));
      */

      pyinited = true;
   } else {
      // Py_Initialize has already been successfully called
      if ( PyRun_SimpleString(
            // Py_Finalize is not used to close stderr so reset it here
            "sys.stderr.data = ''\n"

            // reload all modules in case changes were made by user
            /* this almost works except for strange error the 2nd time we run gun-demo.py
            "import sys\n"
            "for m in sys.modules.keys():\n"
            "   t = str(type(sys.modules[m]))\n"
            "   if t.find('module') < 0 or m == 'golly' or m == 'sys' or m[0] == '_':\n"
            "      pass\n"
            "   else:\n"
            "      reload(sys.modules[m])\n"
            */

            /* RollbackImporter code causes an error
            "if rollbackImporter: rollbackImporter.uninstall()\n"
            "rollbackImporter = RollbackImporter()\n"
            */
            ) < 0
         ) Warning(_("PyRun_SimpleString failed!"));
   }

   return true;
}

// -----------------------------------------------------------------------------

void RunPythonScript(const wxString &filepath)
{
   if (!InitPython()) return;

   // we must convert any backslashes to "\\" to avoid "\a" being treated as
   // escape char, then we must escape any apostrophes
   wxString fpath = filepath;
   fpath.Replace(wxT("\\"), wxT("\\\\"));
   fpath.Replace(wxT("'"), wxT("\\'"));

   // execute the given script
   wxString command = wxT("execfile('") + fpath + wxT("')");
   PyRun_SimpleString(command.mb_str(wxConvLocal));

   // note that PyRun_SimpleString returns -1 if an exception occurred;
   // the error message (in scripterr) is checked at the end of RunScript
}

// -----------------------------------------------------------------------------

void FinishPythonScripting()
{
   // Py_Finalize can cause an obvious delay, so best not to call it
   // if (pyinited) Py_Finalize();

   // probably don't really need this either
   #ifdef USE_PYTHON_DYNAMIC
      FreePythonLib();
   #endif
}
