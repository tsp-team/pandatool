// Filename: lwoToEggConverter.h
// Created by:  drose (17Apr01)
// 
////////////////////////////////////////////////////////////////////

#ifndef LWOTOEGGCONVERTER_H
#define LWOTOEGGCONVERTER_H

#include <pandatoolbase.h>

#include <somethingToEggConverter.h>
#include <lwoHeader.h>
#include <pointerTo.h>

#include <vector>
#include <map>

class CLwoLayer;
class CLwoClip;
class CLwoPoints;
class CLwoPolygons;
class CLwoSurface;
class LwoClip;

////////////////////////////////////////////////////////////////////
// 	 Class : LwoToEggConverter
// Description : This class supervises the construction of an EggData
//               structure from the data represented by the LwoHeader.
//               Reading and writing the egg and lwo structures is
//               left to the user.
////////////////////////////////////////////////////////////////////
class LwoToEggConverter : public SomethingToEggConverter {
public:
  LwoToEggConverter();
  LwoToEggConverter(const LwoToEggConverter &copy);
  virtual ~LwoToEggConverter();

  virtual SomethingToEggConverter *make_copy();

  virtual string get_name() const;
  virtual string get_extension() const;

  virtual bool convert_file(const Filename &filename);
  bool convert_lwo(const LwoHeader *lwo_header);

  CLwoLayer *get_layer(int number) const;
  CLwoClip *get_clip(int number) const;

  CLwoSurface *get_surface(const string &name) const;

  bool _make_materials;

private:
  void cleanup();

  void collect_lwo();
  void make_egg();
  void connect_egg();

  void slot_layer(int number);
  void slot_clip(int number);
  CLwoLayer *make_generic_layer();

  CPT(LwoHeader) _lwo_header;
  
  CLwoLayer *_generic_layer;
  typedef vector<CLwoLayer *> Layers;
  Layers _layers;

  typedef vector<CLwoClip *> Clips;
  Clips _clips;

  typedef vector<CLwoPoints *> Points;
  Points _points;

  typedef vector<CLwoPolygons *> Polygons;
  Polygons _polygons;

  typedef map<string, CLwoSurface *> Surfaces;
  Surfaces _surfaces;
};

#include "lwoToEggConverter.I"

#endif


