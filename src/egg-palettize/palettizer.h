// Filename: palettizer.h
// Created by:  drose (01Dec00)
// 
////////////////////////////////////////////////////////////////////

#ifndef PALETTIZER_H
#define PALETTIZER_H

#include <pandatoolbase.h>

#include "txaFile.h"

#include <typedWriteable.h>

#include <vector>
#include <set>
#include <map>

class PNMFileType;
class EggFile;
class PaletteGroup;
class TextureImage;

////////////////////////////////////////////////////////////////////
// 	 Class : Palettizer
// Description : This is the main engine behind egg-palettize.  It
//               contains all of the program parameters, from the
//               command line or saved from a previous session, and
//               serves as the driving force in the actual palettizing
//               process.
////////////////////////////////////////////////////////////////////
class Palettizer : public TypedWriteable {
public:
  Palettizer();

  void report_pi() const;
  void read_txa_file(const Filename &txa_filename);
  void process_command_line_eggs(bool force_texture_read);
  void process_all(bool force_texture_read);
  void optimal_resize();
  void reset_images();
  void generate_images(bool redo_all);
  bool read_stale_eggs(bool redo_all);
  bool write_eggs();

  EggFile *get_egg_file(const string &name);
  PaletteGroup *get_palette_group(const string &name);
  PaletteGroup *test_palette_group(const string &name) const;
  PaletteGroup *get_default_group();
  TextureImage *get_texture(const string &name);

private:
  static const char *yesno(bool flag);

public:
  int _pi_version;

  enum RemapUV {
    RU_never,
    RU_group,
    RU_poly
  };

  // These values are not stored in the bam file, but are specific to
  // each session.
  TxaFile _txa_file;
  string _default_groupname;
  string _default_groupdir;

  // The following parameter values specifically relate to textures
  // and palettes.  These values are stored in the bam file for future
  // reference.
  string _map_dirname;
  Filename _rel_dirname;
  int _pal_x_size, _pal_y_size;
  int _margin;
  double _repeat_threshold;
  bool _force_power_2;
  bool _aggressively_clean_mapdir;
  bool _round_uvs;
  double _round_unit;
  double _round_fuzz;
  RemapUV _remap_uv;
  PNMFileType *_color_type;
  PNMFileType *_alpha_type;

private:
  typedef map<string, EggFile *> EggFiles;
  EggFiles _egg_files;

  typedef vector<EggFile *> CommandLineEggs;
  CommandLineEggs _command_line_eggs;

  typedef set<TextureImage *> CommandLineTextures;
  CommandLineTextures _command_line_textures;
  
  typedef map<string, PaletteGroup *> Groups;
  Groups _groups;

  typedef map<string, TextureImage *> Textures;
  Textures _textures;


  // The TypedWriteable interface follows.
public:
  static void register_with_read_factory();
  virtual void write_datagram(BamWriter *writer, Datagram &datagram); 
  virtual int complete_pointers(vector_typedWriteable &plist, 
				BamReader *manager);

protected:
  static TypedWriteable *make_Palettizer(const FactoryParams &params);
  void fillin(DatagramIterator &scan, BamReader *manager);

private:
  // These values are only filled in while reading from the bam file;
  // don't use them otherwise.
  int _num_egg_files;
  int _num_groups;
  int _num_textures;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedWriteable::init_type();
    register_type(_type_handle, "Palettizer",
		  TypedWriteable::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }

private:
  static TypeHandle _type_handle;

  friend class EggPalettize;
  friend class TxaLine;
};

extern Palettizer *pal;

#endif