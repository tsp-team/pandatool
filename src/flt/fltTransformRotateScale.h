// Filename: fltTransformRotateScale.h
// Created by:  drose (30Aug00)
// 
////////////////////////////////////////////////////////////////////

#ifndef FLTTRANSFORMROTATESCALE_H
#define FLTTRANSFORMROTATESCALE_H

#include <pandatoolbase.h>

#include "fltTransformRecord.h"

////////////////////////////////////////////////////////////////////
//       Class : FltTransformRotateScale
// Description : A combination rotation and scale.  This is sometimes
//               called "Rotate To Point" within MultiGen.
////////////////////////////////////////////////////////////////////
class FltTransformRotateScale : public FltTransformRecord {
public:
  FltTransformRotateScale(FltHeader *header);

  void set(const LPoint3d &center, const LPoint3d &reference_point,
           const LPoint3d &to_point, bool axis_scale);

  const LPoint3d &get_center() const;
  const LPoint3d &get_reference_point() const;
  const LPoint3d &get_to_point() const;
  float get_overall_scale() const;
  float get_axis_scale() const;
  float get_angle() const;

private:
  void recompute_matrix();

  LPoint3d _center;
  LPoint3d _reference_point;
  LPoint3d _to_point;
  float _overall_scale;
  float _axis_scale;
  float _angle;

protected:
  virtual bool extract_record(FltRecordReader &reader);
  virtual bool build_record(FltRecordWriter &writer) const;

public:
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    FltTransformRecord::init_type();
    register_type(_type_handle, "FltTransformRotateScale",
                  FltTransformRecord::get_class_type());
  }

private:
  static TypeHandle _type_handle;
};

#endif


