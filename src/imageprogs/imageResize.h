// Filename: imageResize.h
// Created by:  drose (13Mar03)
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

#ifndef IMAGERESIZE_H
#define IMAGERESIZE_H

#include "pandatoolbase.h"

#include "imageFilter.h"

////////////////////////////////////////////////////////////////////
//       Class : ImageResize
// Description : A program to read an image file and resize it to a
//               larger or smaller image file.
////////////////////////////////////////////////////////////////////
class ImageResize : public ImageFilter {
public:
  ImageResize();

  void run();

private:
  static bool dispatch_size_request(const string &opt, const string &arg, void *var);

  enum RequestType {
    RT_none,
    RT_pixel_size,
    RT_ratio,
  };
  class SizeRequest {
  public:
    INLINE SizeRequest();
    INLINE RequestType get_type() const;

    INLINE void set_pixel_size(int pixel_size);
    INLINE int get_pixel_size() const;
    INLINE int get_pixel_size(int orig_pixel_size) const;
    INLINE void set_ratio(double ratio);
    INLINE double get_ratio() const;
    INLINE double get_ratio(int orig_pixel_size) const;

  private:
    RequestType _type;
    union {
      int _pixel_size;
      double _ratio;
    } _e;
  };

  SizeRequest _x_size;
  SizeRequest _y_size;

  bool _use_gaussian_filter;
  double _filter_radius;
};

#include "imageResize.I"

#endif

