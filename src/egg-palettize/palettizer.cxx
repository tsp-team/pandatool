// Filename: palettizer.cxx
// Created by:  drose (01Dec00)
// 
////////////////////////////////////////////////////////////////////

#include "palettizer.h"
#include "eggFile.h"
#include "textureImage.h"
#include "string_utils.h"
#include "paletteGroup.h"
#include "filenameUnifier.h"

#include <pnmImage.h>
#include <pnmFileTypeRegistry.h>
#include <pnmFileType.h>
#include <eggData.h>
#include <datagram.h>
#include <datagramIterator.h>
#include <bamReader.h>
#include <bamWriter.h>

Palettizer *pal = (Palettizer *)NULL;

TypeHandle Palettizer::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
Palettizer::
Palettizer() {
  // This number is written out as the first number to the pi file, to
  // indicate the version of egg-palettize that wrote it out.  This
  // allows us to easily update egg-palettize to write out additional
  // information to its pi file, without having it increment the bam
  // version number for all bam and boo files anywhere in the world.
  _pi_version = 0;

  _map_dirname = "%g";
  _margin = 2;
  _repeat_threshold = 250.0;
  _aggressively_clean_mapdir = true;
  _force_power_2 = true;
  _color_type = PNMFileTypeRegistry::get_ptr()->get_type_from_extension("rgb");
  _alpha_type = (PNMFileType *)NULL;
  _pal_x_size = _pal_y_size = 512;

  _round_uvs = true;
  _round_unit = 0.1;
  _round_fuzz = 0.01;
  _remap_uv = RU_poly;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::report_pi
//       Access: Public
//  Description: Output a verbose description of all the palettization
//               information to standard output, for the user's
//               perusal.
////////////////////////////////////////////////////////////////////
void Palettizer::
report_pi() const {
  cout 
    << "\nparams\n"
    << "  map directory: " << _map_dirname << "\n"
    << "  egg relative directory: " 
    << FilenameUnifier::make_user_filename(_rel_dirname) << "\n"
    << "  palettize size: " << _pal_x_size << " by " << _pal_y_size << "\n"
    << "  margin: " << _margin << "\n"
    << "  repeat threshold: " << _repeat_threshold << "%\n"
    << "  force textures to power of 2: " << yesno(_force_power_2) << "\n"
    << "  aggressively clean the map directory: "
    << yesno(_aggressively_clean_mapdir) << "\n"
    << "  round UV area: " << yesno(_round_uvs) << "\n";
  if (_round_uvs) {
    cout << "  round UV area to nearest " << _round_unit << " with fuzz "
	 << _round_fuzz << "\n";
  }
  cout << "  remap UV's: ";
  switch (_remap_uv) {
  case RU_never:
    cout << "never\n";
    break;

  case RU_group:
    cout << "per group\n";
    break;

  case RU_poly:
    cout << "per polygon\n";
    break;
  }

  if (_color_type != (PNMFileType *)NULL) {
    cout << "  generate image files of type: " 
	 << _color_type->get_suggested_extension();
    if (_alpha_type != (PNMFileType *)NULL) {
      cout << "," << _alpha_type->get_suggested_extension();
    }
    cout << "\n";
  }

  cout << "\ntexture source pathnames and sizes\n";
  Textures::const_iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    cout << "  " << texture->get_name() << ":\n";
    texture->write_source_pathnames(cout, 4);
  }

  cout << "\negg files and textures referenced\n";
  EggFiles::const_iterator ei;
  for (ei = _egg_files.begin(); ei != _egg_files.end(); ++ei) {
    EggFile *egg_file = (*ei).second;
    egg_file->write_description(cout, 2);
    egg_file->write_texture_refs(cout, 4);
  }

  cout << "\npalette groups\n";
  Groups::const_iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    if (gi != _groups.begin()) {
      cout << "\n";
    }
    cout << "  " << group->get_name() << ": "
	 << group->get_groups() << "\n";
    group->write_image_info(cout, 4);
  }

  cout << "\ntextures\n";
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    texture->write_scale_info(cout, 2);
  }

  cout << "\nsurprises\n";
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    if (texture->is_surprise()) {
      cout << "  " << texture->get_name() << "\n";
    }
  }
  for (ei = _egg_files.begin(); ei != _egg_files.end(); ++ei) {
    EggFile *egg_file = (*ei).second;
    if (egg_file->is_surprise()) {
      cout << "  " << egg_file->get_name() << "\n";
    }
  }

  cout << "\n";
}


////////////////////////////////////////////////////////////////////
//     Function: Palettizer::read_txa_file
//       Access: Public
//  Description: Reads in the .txa file and keeps it ready for
//               matching textures and egg files.
////////////////////////////////////////////////////////////////////
void Palettizer::
read_txa_file(const Filename &txa_filename) {
  // Clear out the group dependencies, in preparation for reading them
  // again from the .txa file.
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->clear_depends();
  }

  if (!_txa_file.read(txa_filename)) {
    exit(1);
  }

  if (_color_type == (PNMFileType *)NULL) {
    nout << "No valid output image file type available; cannot run.\n"
	 << "Use :imagetype command in .txa file.\n";
    exit(1);
  }

  // Compute the correct dependency level for each group.  This will
  // help us when we assign the textures to their groups.
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->set_dependency_level(1);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::process_command_line_eggs
//       Access: Public
//  Description: Processes all the textures named in the
//               _command_line_eggs, placing them on the appropriate
//               palettes or whatever needs to be done with them.
//
//               If force_texture_read is true, it forces each texture
//               image file to be read (and thus legitimately checked
//               for grayscaleness etc.) before placing.
////////////////////////////////////////////////////////////////////
void Palettizer::
process_command_line_eggs(bool force_texture_read) {
  _command_line_textures.clear();

  // Start by scanning all the egg files we read up on the command
  // line.
  CommandLineEggs::const_iterator ei;
  for (ei = _command_line_eggs.begin();
       ei != _command_line_eggs.end();
       ++ei) {
    EggFile *egg_file = (*ei);

    egg_file->scan_textures();
    egg_file->get_textures(_command_line_textures);

    egg_file->pre_txa_file();
    _txa_file.match_egg(egg_file);
    egg_file->post_txa_file();
  }

  // Now that all of our egg files are read in, build in all the cross
  // links and back pointers and stuff.
  EggFiles::const_iterator efi;
  for (efi = _egg_files.begin(); efi != _egg_files.end(); ++efi) {
    (*efi).second->build_cross_links();
  }

  // Now match each of the textures mentioned in those egg files
  // against a line in the .txa file.
  CommandLineTextures::iterator ti;
  for (ti = _command_line_textures.begin(); 
       ti != _command_line_textures.end();
       ++ti) {
    TextureImage *texture = *ti;

    if (force_texture_read) {
      texture->read_source_image();
    }

    texture->pre_txa_file();
    _txa_file.match_texture(texture);
    texture->post_txa_file();
  }

  // And now, assign each of the current set of textures to an
  // appropriate group or groups.
  for (ti = _command_line_textures.begin(); 
       ti != _command_line_textures.end();
       ++ti) {
    TextureImage *texture = *ti;
    texture->assign_groups();
  }

  // And then the egg files need to sign up for a particular
  // TexturePlacement, so we can determine some more properties about
  // how the textures are placed (for instance, how big the UV range
  // is for a particular TexturePlacement).
  for (efi = _egg_files.begin(); efi != _egg_files.end(); ++efi) {
    (*efi).second->choose_placements();
  }

  // Now that *that's* done, we need to make sure the various
  // TexturePlacements require the right size for their textures.
  for (ti = _command_line_textures.begin(); 
       ti != _command_line_textures.end();
       ++ti) {
    TextureImage *texture = *ti;
    texture->determine_placement_size();
  }

  // Now that each texture has been assigned to a suitable group,
  // make sure the textures are placed on specific PaletteImages.
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->place_all();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::process_all
//       Access: Public
//  Description: Reprocesses all textures known.
//
//               If force_texture_read is true, it forces each texture
//               image file to be read (and thus legitimately checked
//               for grayscaleness etc.) before placing.
////////////////////////////////////////////////////////////////////
void Palettizer::
process_all(bool force_texture_read) {
  // If there *were* any egg files on the command line, deal with
  // them.
  CommandLineEggs::const_iterator ei;
  for (ei = _command_line_eggs.begin();
       ei != _command_line_eggs.end();
       ++ei) {
    EggFile *egg_file = (*ei);

    egg_file->scan_textures();
    egg_file->get_textures(_command_line_textures);
  }

  // Then match up all the egg files we know about with the .txa file.
  EggFiles::const_iterator efi;
  for (efi = _egg_files.begin(); efi != _egg_files.end(); ++efi) {
    EggFile *egg_file = (*efi).second;

    egg_file->pre_txa_file();
    _txa_file.match_egg(egg_file);
    egg_file->post_txa_file();
  }

  // Now that all of our egg files are read in, build in all the cross
  // links and back pointers and stuff.
  for (efi = _egg_files.begin(); efi != _egg_files.end(); ++efi) {
    (*efi).second->build_cross_links();
  }

  // Now match each of the textures in the world against a line in the
  // .txa file.
  Textures::iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;

    if (force_texture_read) {
      texture->read_source_image();
    }

    texture->pre_txa_file();
    _txa_file.match_texture(texture);
    texture->post_txa_file();
  }

  // And now, assign each texture to an appropriate group or groups.
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    texture->assign_groups();
  }

  // And then the egg files need to sign up for a particular
  // TexturePlacement, so we can determine some more properties about
  // how the textures are placed (for instance, how big the UV range
  // is for a particular TexturePlacement).
  for (efi = _egg_files.begin(); efi != _egg_files.end(); ++efi) {
    (*efi).second->choose_placements();
  }

  // Now that *that's* done, we need to make sure the various
  // TexturePlacements require the right size for their textures.
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    texture->determine_placement_size();
  }

  // Now that each texture has been assigned to a suitable group,
  // make sure the textures are placed on specific PaletteImages.
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->place_all();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::optimal_resize
//       Access: Public
//  Description: Attempts to resize each PalettteImage down to its
//               smallest possible size.
////////////////////////////////////////////////////////////////////
void Palettizer::
optimal_resize() {
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->optimal_resize();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::reset_images
//       Access: Public
//  Description: Throws away all of the current PaletteImages, so that
//               new ones may be created (and the packing made more
//               optimal).
////////////////////////////////////////////////////////////////////
void Palettizer::
reset_images() {
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->reset_images();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::generate_images
//       Access: Public
//  Description: Actually generates the appropriate palette and
//               unplaced texture images into the map directories.  If
//               redo_all is true, this forces a regeneration of each
//               image file.
////////////////////////////////////////////////////////////////////
void Palettizer::
generate_images(bool redo_all) {
  Groups::iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    PaletteGroup *group = (*gi).second;
    group->update_images(redo_all);
  }

  Textures::iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    TextureImage *texture = (*ti).second;
    texture->copy_unplaced(redo_all);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::read_stale_eggs
//       Access: Public
//  Description: Reads in any egg file that is known to be stale, even
//               if it was not listed on the command line, so that it
//               may be updated and written out when write_eggs() is
//               called.  If redo_all is true, this even reads egg
//               files that were not flagged as stale.
//
//               Returns true if successful, or false if there was
//               some error.
////////////////////////////////////////////////////////////////////
bool Palettizer::
read_stale_eggs(bool redo_all) {
  bool okflag = true;

  EggFiles::iterator ei;
  for (ei = _egg_files.begin(); ei != _egg_files.end(); ++ei) {
    EggFile *egg_file = (*ei).second;
    if (!egg_file->has_data() && 
	(egg_file->is_stale() || redo_all)) {
      if (!egg_file->read_egg()) {
	okflag = false;

      } else {
	egg_file->scan_textures();
	egg_file->choose_placements();
      }
    }
  }

  return okflag;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::write_eggs
//       Access: Public
//  Description: Adjusts the egg files to reference the newly
//               generated textures, and writes them out.  Returns
//               true if successful, or false if there was some error.
////////////////////////////////////////////////////////////////////
bool Palettizer::
write_eggs() {
  bool okflag = true;

  EggFiles::iterator ei;
  for (ei = _egg_files.begin(); ei != _egg_files.end(); ++ei) {
    EggFile *egg_file = (*ei).second;
    if (egg_file->has_data()) {
      egg_file->update_egg();
      if (!egg_file->write_egg()) {
	okflag = false;
      }
    }
  }

  return okflag;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::get_egg_file
//       Access: Public
//  Description: Returns the EggFile with the given name.  If there is
//               no EggFile with the indicated name, creates one.
//               This is the key name used to sort the egg files,
//               which is typically the basename of the filename.
////////////////////////////////////////////////////////////////////
EggFile *Palettizer::
get_egg_file(const string &name) {
  EggFiles::iterator ei = _egg_files.find(name);
  if (ei != _egg_files.end()) {
    return (*ei).second;
  }

  EggFile *file = new EggFile;
  file->set_name(name);
  _egg_files.insert(EggFiles::value_type(name, file));
  return file;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::get_palette_group
//       Access: Public
//  Description: Returns the PaletteGroup with the given name.  If
//               there is no PaletteGroup with the indicated name,
//               creates one.
////////////////////////////////////////////////////////////////////
PaletteGroup *Palettizer::
get_palette_group(const string &name) {
  Groups::iterator gi = _groups.find(name);
  if (gi != _groups.end()) {
    return (*gi).second;
  }

  PaletteGroup *group = new PaletteGroup;
  group->set_name(name);
  _groups.insert(Groups::value_type(name, group));
  return group;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::test_palette_group
//       Access: Public
//  Description: Returns the PaletteGroup with the given name.  If
//               there is no PaletteGroup with the indicated name,
//               returns NULL.
////////////////////////////////////////////////////////////////////
PaletteGroup *Palettizer::
test_palette_group(const string &name) const {
  Groups::const_iterator gi = _groups.find(name);
  if (gi != _groups.end()) {
    return (*gi).second;
  }

  return (PaletteGroup *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::get_default_group
//       Access: Public
//  Description: Returns the default group to which an egg file should
//               be assigned if it is not mentioned in the .txa file.
////////////////////////////////////////////////////////////////////
PaletteGroup *Palettizer::
get_default_group() {
  PaletteGroup *default_group = get_palette_group(_default_groupname);
  if (!_default_groupdir.empty() && !default_group->has_dirname()) {
    default_group->set_dirname(_default_groupdir);
  }
  return default_group;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::get_texture
//       Access: Public
//  Description: Returns the TextureImage with the given name.  If
//               there is no TextureImage with the indicated name,
//               creates one.  This is the key name used to sort the
//               textures, which is typically the basename of the
//               primary filename.
////////////////////////////////////////////////////////////////////
TextureImage *Palettizer::
get_texture(const string &name) {
  Textures::iterator ti = _textures.find(name);
  if (ti != _textures.end()) {
    return (*ti).second;
  }

  TextureImage *image = new TextureImage;
  image->set_name(name);
  //  image->set_filename(name);
  _textures.insert(Textures::value_type(name, image));
  return image;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::yesno
//       Access: Private, Static
//  Description: A silly function to return "yes" or "no" based on a
//               bool flag for nicely formatted output.
////////////////////////////////////////////////////////////////////
const char *Palettizer::
yesno(bool flag) {
  return flag ? "yes" : "no";
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::register_with_read_factory
//       Access: Public, Static
//  Description: Registers the current object as something that can be
//               read from a Bam file.
////////////////////////////////////////////////////////////////////
void Palettizer::
register_with_read_factory() {
  BamReader::get_factory()->
    register_factory(get_class_type(), make_Palettizer);
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::write_datagram
//       Access: Public, Virtual
//  Description: Fills the indicated datagram up with a binary
//               representation of the current object, in preparation
//               for writing to a Bam file.
////////////////////////////////////////////////////////////////////
void Palettizer::
write_datagram(BamWriter *writer, Datagram &datagram) {
  datagram.add_int32(_pi_version);
  datagram.add_string(_map_dirname);
  datagram.add_string(FilenameUnifier::make_bam_filename(_rel_dirname));
  datagram.add_int32(_pal_x_size);
  datagram.add_int32(_pal_y_size);
  datagram.add_int32(_margin);
  datagram.add_float64(_repeat_threshold);
  datagram.add_bool(_force_power_2);
  datagram.add_bool(_aggressively_clean_mapdir);
  datagram.add_bool(_round_uvs);
  datagram.add_float64(_round_unit);
  datagram.add_float64(_round_fuzz);
  datagram.add_int32((int)_remap_uv);
  writer->write_pointer(datagram, _color_type);
  writer->write_pointer(datagram, _alpha_type); 

  datagram.add_int32(_egg_files.size());
  EggFiles::const_iterator ei;
  for (ei = _egg_files.begin(); ei != _egg_files.end(); ++ei) {
    writer->write_pointer(datagram, (*ei).second);
  }

  // We don't write _command_line_eggs; that's specific to each
  // session.

  datagram.add_int32(_groups.size());
  Groups::const_iterator gi;
  for (gi = _groups.begin(); gi != _groups.end(); ++gi) {
    writer->write_pointer(datagram, (*gi).second);
  }

  datagram.add_int32(_textures.size());
  Textures::const_iterator ti;
  for (ti = _textures.begin(); ti != _textures.end(); ++ti) {
    writer->write_pointer(datagram, (*ti).second);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::complete_pointers
//       Access: Public, Virtual
//  Description: Called after the object is otherwise completely read
//               from a Bam file, this function's job is to store the
//               pointers that were retrieved from the Bam file for
//               each pointer object written.  The return value is the
//               number of pointers processed from the list.
////////////////////////////////////////////////////////////////////
int Palettizer::
complete_pointers(vector_typedWriteable &plist, BamReader *manager) {
  nassertr((int)plist.size() >= 2 + _num_egg_files + _num_groups + _num_textures, 0);
  int index = 0;

  if (plist[index] != (TypedWriteable *)NULL) {
    DCAST_INTO_R(_color_type, plist[index], index);
  }
  index++;

  if (plist[index] != (TypedWriteable *)NULL) {
    DCAST_INTO_R(_alpha_type, plist[index], index);
  }
  index++;

  int i;
  for (i = 0; i < _num_egg_files; i++) {
    EggFile *egg_file;
    DCAST_INTO_R(egg_file, plist[index], index);
    _egg_files.insert(EggFiles::value_type(egg_file->get_name(), egg_file));
    index++;
  }

  for (i = 0; i < _num_groups; i++) {
    PaletteGroup *group;
    DCAST_INTO_R(group, plist[index], index);
    _groups.insert(Groups::value_type(group->get_name(), group));
    index++;
  }

  for (i = 0; i < _num_textures; i++) {
    TextureImage *texture;
    DCAST_INTO_R(texture, plist[index], index);
    _textures.insert(Textures::value_type(texture->get_name(), texture));
    index++;
  }

  return index;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::make_Palettizer
//       Access: Protected
//  Description: This method is called by the BamReader when an object
//               of this type is encountered in a Bam file; it should
//               allocate and return a new object with all the data
//               read.
////////////////////////////////////////////////////////////////////
TypedWriteable* Palettizer::
make_Palettizer(const FactoryParams &params) {
  Palettizer *me = new Palettizer;
  BamReader *manager;
  Datagram packet;

  parse_params(params, manager, packet);
  DatagramIterator scan(packet);

  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: Palettizer::fillin
//       Access: Protected
//  Description: Reads the binary data from the given datagram
//               iterator, which was written by a previous call to
//               write_datagram().
////////////////////////////////////////////////////////////////////
void Palettizer::
fillin(DatagramIterator &scan, BamReader *manager) {
  _pi_version = scan.get_int32();
  _map_dirname = scan.get_string();
  _rel_dirname = FilenameUnifier::get_bam_filename(scan.get_string());
  FilenameUnifier::set_rel_dirname(_rel_dirname);
  _pal_x_size = scan.get_int32();
  _pal_y_size = scan.get_int32();
  _margin = scan.get_int32();
  _repeat_threshold = scan.get_float64();
  _force_power_2 = scan.get_bool();
  _aggressively_clean_mapdir = scan.get_bool();
  _round_uvs = scan.get_bool();
  _round_unit = scan.get_float64();
  _round_fuzz = scan.get_float64();
  _remap_uv = (RemapUV)scan.get_int32();
  manager->read_pointer(scan, this);  // _color_type
  manager->read_pointer(scan, this);  // _alpha_type

  _num_egg_files = scan.get_int32();
  manager->read_pointers(scan, this, _num_egg_files);

  _num_groups = scan.get_int32();
  manager->read_pointers(scan, this, _num_groups);

  _num_textures = scan.get_int32();
  manager->read_pointers(scan, this, _num_textures);
}
