// Filename: cLwoSurface.h
// Created by:  drose (25Apr01)
// 
////////////////////////////////////////////////////////////////////

#ifndef CLWOSURFACE_H
#define CLWOSURFACE_H

#include <pandatoolbase.h>

#include <lwoSurface.h>
#include <luse.h>

class LwoToEggConverter;
class EggPrimitive;

////////////////////////////////////////////////////////////////////
// 	 Class : CLwoSurface
// Description : This class is a wrapper around LwoSurface and stores
//               additional information useful during the
//               conversion-to-egg process.
////////////////////////////////////////////////////////////////////
class CLwoSurface {
public:
  CLwoSurface(LwoToEggConverter *converter, const LwoSurface *surface);

  INLINE const string &get_name() const;

  void apply_properties(EggPrimitive *egg_prim, float &smooth_angle);

  enum Flags {
    F_color        = 0x0001,
    F_diffuse      = 0x0002,
    F_luminosity   = 0x0004,
    F_specular     = 0x0008,
    F_reflection   = 0x0010,
    F_transparency = 0x0020,
    F_translucency = 0x0040,
    F_smooth_angle = 0x0080,
    F_backface     = 0x0100,
  };
  
  int _flags;
  RGBColorf _color;
  float _diffuse;
  float _luminosity;
  float _specular;
  float _reflection;
  float _transparency;
  float _translucency;
  float _smooth_angle;
  bool _backface;

  LwoToEggConverter *_converter;
  CPT(LwoSurface) _surface;
};

#include "cLwoSurface.I"

#endif

