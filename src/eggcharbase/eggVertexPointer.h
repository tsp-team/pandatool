// Filename: eggVertexPointer.h
// Created by:  drose (26Feb01)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#ifndef EGGVERTEXPOINTER_H
#define EGGVERTEXPOINTER_H

#include "pandatoolbase.h"

#include "eggSliderPointer.h"

#include "eggGroup.h"
#include "pointerTo.h"

////////////////////////////////////////////////////////////////////
//       Class : EggVertexPointer
// Description : This stores a pointer back to a <Vertex>, or to a
//               particular pritimive like a <Polygon>, representing a
//               morph offset.
////////////////////////////////////////////////////////////////////
class EggVertexPointer : public EggSliderPointer {
public:
  EggVertexPointer(EggObject *egg_object);

  virtual int get_num_frames() const;
  virtual double get_frame(int n) const;

  virtual bool has_vertices() const;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    EggSliderPointer::init_type();
    register_type(_type_handle, "EggVertexPointer",
                  EggSliderPointer::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#endif


