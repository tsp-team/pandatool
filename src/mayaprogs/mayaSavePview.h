// Filename: mayaSavePview.h
// Created by:  drose (27Oct03)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#ifndef MAYASAVEPVIEW_H
#define MAYASAVEPVIEW_H

// Maya will try to typedef bool unless this symbol is defined.
#ifndef _BOOL
#define _BOOL 1
#endif

#include <maya/MArgList.h>
#include <maya/MPxCommand.h>
#include <maya/MObject.h>

// Even though we don't include any Panda headers, it's safe to
// include this one, since it only defines some macros that we need to
// make this program platform-independent.
#include "dtool_config.h"


////////////////////////////////////////////////////////////////////
//       Class : MayaSavePview
// Description : This class serves as a plug-in to Maya to save the
//               scene and view it using the external pview program,
//               rather than linking in any part of Panda to a Maya
//               plugin.
//
//               Since it does not link with any Panda code, and hence
//               is a very lean plugin, it is less likely than
//               MayaPview to cause interoperability problems within
//               Maya.  However, it does force a save-to-disk and a
//               spawning of a separate executable, including a
//               complete reloading of all of the Maya libraries, so
//               it is quite a bit slower to execute.  And the
//               potential for interactive control is substantially
//               reduced.
////////////////////////////////////////////////////////////////////
class MayaSavePview : public MPxCommand {
public:
  MayaSavePview();
  virtual MStatus doIt(const MArgList &);

  static void *creator();
};

// Since we don't include any of the Panda headers (other than
// dtool_config.h), we have to define this macro ourselves, to tell
// Windows to export the following functions from the DLL.
#ifdef WIN32_VC
  #define EXPCL_MISC __declspec(dllexport)
#else
  #define EXPCL_MISC
#endif

EXPCL_MISC MStatus initializePlugin(MObject obj);
EXPCL_MISC MStatus uninitializePlugin(MObject obj);


#endif