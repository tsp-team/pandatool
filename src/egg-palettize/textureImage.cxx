// Filename: textureImage.cxx
// Created by:  drose (29Nov00)
// 
////////////////////////////////////////////////////////////////////

#include "textureImage.h"
#include "sourceTextureImage.h"
#include "destTextureImage.h"
#include "eggFile.h"
#include "paletteGroup.h"
#include "texturePlacement.h"
#include "filenameUnifier.h"

#include <indent.h>
#include <datagram.h>
#include <datagramIterator.h>
#include <bamReader.h>
#include <bamWriter.h>

TypeHandle TextureImage::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
TextureImage::
TextureImage() {
  _preferred_source = (SourceTextureImage *)NULL;
  _read_source_image = false;
  _got_dest_image = false;
  _is_surprise = true;
  _ever_read_image = false;
  _forced_grayscale = false;
  _forced_unalpha = false;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::note_egg_file
//       Access: Public
//  Description: Records that a particular egg file references this
//               texture.  This is essential to know when deciding how
//               to assign the TextureImage to the various
//               PaletteGroups.
////////////////////////////////////////////////////////////////////
void TextureImage::
note_egg_file(EggFile *egg_file) {
  nassertv(!egg_file->get_complete_groups().empty());
  _egg_files.insert(egg_file);
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::assign_groups
//       Access: Public
//  Description: Assigns the texture to all of the PaletteGroups the
//               various egg files that use it need.  Attempts to
//               choose the minimum set of PaletteGroups that
//               satisfies all of the egg files.
////////////////////////////////////////////////////////////////////
void TextureImage::
assign_groups() {
  if (_egg_files.empty()) {
    // If we're not referenced by any egg files any more, assign us to
    // no groups.
    PaletteGroups empty;
    assign_to_groups(empty);
    return;
  }

  PaletteGroups definitely_in;

  // First, we need to eliminate from consideration all the egg files
  // that are already taken care of by the user's explicit group
  // assignments for this texture.
  WorkingEggs needed_eggs;

  if (_explicitly_assigned_groups.empty()) {
    // If we have no explicit group assignments, we must consider all
    // the egg files.
    copy(_egg_files.begin(), _egg_files.end(), back_inserter(needed_eggs));
  
  } else {
    // Otherwise, we only need to consider the egg files that don't
    // have any groups in common with our explicit assignments.

    EggFiles::const_iterator ei;
    for (ei = _egg_files.begin(); ei != _egg_files.end(); ++ei) {
      PaletteGroups intersect;
      intersect.make_intersection(_explicitly_assigned_groups, (*ei)->get_complete_groups());
      if (!intersect.empty()) {
	// This egg file is satisfied by one of the texture's explicit
	// assignments.

	// We must use at least one of the explicitly-assigned groups
	// that satisfied the egg file.  We don't need to use all of
	// them, however, and we choose the first one arbitrarily.
	definitely_in.insert(*intersect.begin());

      } else {
	// This egg file was not satisfied by any of the texture's
	// explicit assignments.  Therefore, we'll need to choose some
	// additional group to assign the texture to, to make the egg
	// file happy.  Defer this a bit.
	needed_eggs.push_back(*ei);
      }
    }
  }

  while (!needed_eggs.empty()) {
    // We need to know the complete set of groups that we need to
    // consider adding the texture to.  This is the union of all the egg
    // files' requested groups.
    PaletteGroups total;
    WorkingEggs::const_iterator ei;
    for (ei = needed_eggs.begin(); ei != needed_eggs.end(); ++ei) {
      total.make_union(total, (*ei)->get_complete_groups());
    }
    
    // Now, find the group that will satisfy the most egg files.  If
    // two groups satisfy the same number of egg files, choose (a) the
    // most specific one, i.e. with the lowest dependency_level value,
    // and (b) the one that has the fewest egg files sharing it.
    nassertv(!total.empty());
    PaletteGroups::iterator gi = total.begin();
    PaletteGroup *best = (*gi);
    int best_egg_count = compute_egg_count(best, needed_eggs);
    ++gi;
    while (gi != total.end()) {
      PaletteGroup *group = (*gi);

      // Do we prefer this group to our current 'best'?
      bool prefer_group = false;
      int group_egg_count = compute_egg_count(group, needed_eggs);
      if (group_egg_count != best_egg_count) {
	prefer_group = (group_egg_count > best_egg_count);

      } else if (group->get_dependency_level() != best->get_dependency_level()){ 
	prefer_group = 
	  (group->get_dependency_level() < best->get_dependency_level());

      } else {
	prefer_group = (group->get_egg_count() < best->get_egg_count());
      }

      if (prefer_group) {
	best = group;
	best_egg_count = group_egg_count;
      }
      ++gi;
    }
    
    // Okay, now we've picked the best group.  Eliminate all the eggs
    // from consideration that are satisfied by this group, and repeat.
    definitely_in.insert(best);
    
    WorkingEggs next_needed_eggs;
    for (ei = needed_eggs.begin(); ei != needed_eggs.end(); ++ei) {
      if ((*ei)->get_complete_groups().count(best) == 0) {
	// This one wasn't eliminated.
	next_needed_eggs.push_back(*ei);
      }
    }
    needed_eggs.swap(next_needed_eggs);
  }

  // Finally, now that we've computed the set of groups we need to
  // assign the texture to, we need to reconcile this with the set of
  // groups we've assigned the texture to previously.
  assign_to_groups(definitely_in);
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::get_groups
//       Access: Public
//  Description: Once get_groups() has been called, this returns the
//               actual set of groups the TextureImage has been
//               assigned to.
////////////////////////////////////////////////////////////////////
const PaletteGroups &TextureImage::
get_groups() const {
  return _actual_assigned_groups;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::get_placement
//       Access: Public
//  Description: Gets the TexturePlacement object which represents the
//               assignment of this texture to the indicated group.
//               If the texture has not been assigned to the indicated
//               group, returns NULL.
////////////////////////////////////////////////////////////////////
TexturePlacement *TextureImage::
get_placement(PaletteGroup *group) const {
  Placement::const_iterator pi;
  pi = _placement.find(group);
  if (pi == _placement.end()) {
    return (TexturePlacement *)NULL;
  }

  return (*pi).second;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::force_replace
//       Access: Public
//  Description: Removes the texture from any PaletteImages it is
//               assigned to, but does not remove it from the groups.
//               It will be re-placed within each group when
//               PaletteGroup::place_all() is called.
////////////////////////////////////////////////////////////////////
void TextureImage::
force_replace() {
  Placement::iterator pi;
  for (pi = _placement.begin(); pi != _placement.end(); ++pi) {
    (*pi).second->force_replace();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::pre_txa_file
//       Access: Public
//  Description: Updates any internal state prior to reading the .txa
//               file.
////////////////////////////////////////////////////////////////////
void TextureImage::
pre_txa_file() {
  // Save our current properties, so we can note if they change.
  _pre_txa_properties = _properties;

  // Update our properties from the egg files that reference this
  // texture.  It's possible the .txa file will update them further.
  SourceTextureImage *source = get_preferred_source();
  if (source != (SourceTextureImage *)NULL) {
    _properties = source->get_properties();
  }

  _request.pre_txa_file();
  _is_surprise = true;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::post_txa_file
//       Access: Public
//  Description: Once the .txa file has been read and the TextureImage
//               matched against it, considers applying the requested
//               size change.  Updates the TextureImage's size with
//               the size the texture ought to be, if this can be
//               determined.
////////////////////////////////////////////////////////////////////
void TextureImage::
post_txa_file() {
  // First, get the actual size of the texture.
  SourceTextureImage *source = get_preferred_source();
  if (source != (SourceTextureImage *)NULL) {
    if (source->get_size()) {
      _size_known = true;
      _x_size = source->get_x_size();
      _y_size = source->get_y_size();
      _properties._got_num_channels = true;
      _properties._num_channels = source->get_num_channels();
    }
  }

  // Now update this with a particularly requested size.
  if (_request._got_size) {
    _size_known = true;
    _x_size = _request._x_size;
    _y_size = _request._y_size;
  }
    
  if (_request._got_num_channels) {
    _properties._got_num_channels = true;
    _properties._num_channels = _request._num_channels;

  } else {
    // If we didn't request a particular number of channels, examine
    // the image to determine if we can downgrade it, for instance
    // from color to grayscale.  
    if (_properties._got_num_channels &&
	(_properties._num_channels == 3 || _properties._num_channels == 4)) {
      consider_grayscale();
    }
    
    // Also consider downgrading from alpha to non-alpha.
    if (_properties._got_num_channels &&
	(_properties._num_channels == 2 || _properties._num_channels == 4)) {
      consider_unalpha();
    }
  }

  if (_request._format != EggTexture::F_unspecified) {
    _properties._format = _request._format;
  }
  if (_request._minfilter != EggTexture::FT_unspecified) {
    _properties._minfilter = _request._minfilter;
  }
  if (_request._magfilter != EggTexture::FT_unspecified) {
    _properties._magfilter = _request._magfilter;
  }

  // Finally, make sure our properties are fully defined.
  _properties.fully_define();

  // Now, if our properties have changed in all that from our previous
  // session, we need to re-place ourself in all palette groups.
  if (_properties != _pre_txa_properties) {
    force_replace();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::determine_placement_size
//       Access: Public
//  Description: Calls determine_size() on each TexturePlacement for
//               the texture, to ensure that each TexturePlacement is
//               still requesting the best possible size for the
//               texture.
////////////////////////////////////////////////////////////////////
void TextureImage::
determine_placement_size() {
  Placement::iterator pi;
  for (pi = _placement.begin(); pi != _placement.end(); ++pi) {
    TexturePlacement *placement = (*pi).second;
    placement->determine_size();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::get_omit
//       Access: Public
//  Description: Returns true if the user specifically requested to
//               omit this texture via the "omit" keyword in the .txa
//               file, or false otherwise.
////////////////////////////////////////////////////////////////////
bool TextureImage::
get_omit() const {
  return _request._omit;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::get_repeat_threshold
//       Access: Public
//  Description: Returns the suitable repeat threshold for this
//               texture.  This is either the
//               Palettizer::_repeat_threshold parameter, given
//               globally via -r, or a particular value for this
//               texture as supplied by the "repeat" keyword in the
//               .txa file.
////////////////////////////////////////////////////////////////////
double TextureImage::
get_repeat_threshold() const {
  return _request._repeat_threshold;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::get_margin
//       Access: Public
//  Description: Returns the suitable repeat threshold for this
//               texture.  This is either the Palettizer::_margin
//               parameter, or a particular value for this texture as
//               supplied by the "margin" keyword in the .txa file.
////////////////////////////////////////////////////////////////////
int TextureImage::
get_margin() const {
  return _request._margin;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::is_surprise
//       Access: Public
//  Description: Returns true if this particular texture is a
//               'surprise', i.e. it wasn't matched by a line in the
//               .txa file that didn't include the keyword 'cont'.
////////////////////////////////////////////////////////////////////
bool TextureImage::
is_surprise() const {
  return _is_surprise;
}


////////////////////////////////////////////////////////////////////
//     Function: TextureImage::get_source
//       Access: Public
//  Description: Returns the SourceTextureImage corresponding to the
//               given filename(s).  If the given filename has never
//               been used as a SourceTexture for this particular
//               texture, creates a new SourceTextureImage and returns
//               that.
////////////////////////////////////////////////////////////////////
SourceTextureImage *TextureImage::
get_source(const Filename &filename, const Filename &alpha_filename) {
  string key = get_source_key(filename, alpha_filename);

  Sources::iterator si;
  si = _sources.find(key);
  if (si != _sources.end()) {
    return (*si).second;
  }

  SourceTextureImage *source = 
    new SourceTextureImage(this, filename, alpha_filename);
  _sources.insert(Sources::value_type(key, source));

  // Clear out the preferred source image to force us to rederive this
  // next time someone asks.
  _preferred_source = (SourceTextureImage *)NULL;
  _read_source_image = false;
  _got_dest_image = false;

  return source;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::get_preferred_source
//       Access: Public
//  Description: Determines the preferred source image for examining
//               size and reading pixels, etc.  This is the largest
//               and most recent of all the available source images.
////////////////////////////////////////////////////////////////////
SourceTextureImage *TextureImage::
get_preferred_source() {
  if (_preferred_source != (SourceTextureImage *)NULL) {
    return _preferred_source;
  }

  // Now examine all of the various source images available to us and
  // pick the most suitable.  We base this on the following criteria:

  // (1) A suitable source image must be referenced by at least one
  // egg file, unless no source images are referenced by any egg file.

  // (2) A larger source image is preferable to a smaller one.

  // (3) Given two source images of the same size, the more recent one
  // is preferable.

  // Are any source images referenced by an egg file?

  bool any_referenced = false;
  Sources::iterator si;
  for (si = _sources.begin(); si != _sources.end() && !any_referenced; ++si) {
    SourceTextureImage *source = (*si).second;
    if (source->get_egg_count() > 0) {
      any_referenced = true;
    }
  }

  SourceTextureImage *best = (SourceTextureImage *)NULL;
  int best_size;

  for (si = _sources.begin(); si != _sources.end() && !any_referenced; ++si) {
    SourceTextureImage *source = (*si).second;

    if (source->get_egg_count() > 0 || !any_referenced) {
      // Rule (1) passes.

      if (source->exists() && source->get_size()) {
	int source_size = source->get_x_size() * source->get_y_size();
	if (best == (SourceTextureImage *)NULL) {
	  best = source;
	  best_size = source_size;

	} else if (source_size > best_size) {
	  // Rule (2) passes.
	  best = source;
	  best_size = source_size;

	} else if (source_size == best_size && 
		   source->get_filename().compare_timestamps(best->get_filename()) > 0) {
	  // Rule (3) passes.
	  best = source;
	  best_size = source_size;
	}
      }
    }
  }

  if (best == (SourceTextureImage *)NULL && !_sources.empty()) {
    // If we didn't pick any that pass, it must be that all of them
    // are unreadable.  In this case, it really doesn't matter which
    // one we pick.
    best = (*_sources.begin()).second;
  }

  _preferred_source = best;
  return _preferred_source;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::copy_unplaced
//       Access: Public
//  Description: Copies the texture to whichever destination
//               directories are appropriate for the groups in which
//               it has been unplaced.  Also removes the old filenames
//               for previous sessions where it was unplaced, but is
//               no longer.
//
//               If redo_all is true, this recopies the texture
//               whether it needed to or not.
////////////////////////////////////////////////////////////////////
void TextureImage::
copy_unplaced(bool redo_all) {
  // First, we need to build up the set of DestTextureImages that
  // represents the files we need to generate.
  Dests generate;

  // Go through all the TexturePlacements and note the ones for which
  // we're unplaced.  We check get_omit_reason() and not is_placed(),
  // because we want to consider solitary images to be unplaced in
  // this case.
  Placement::iterator pi;
  for (pi = _placement.begin(); pi != _placement.end(); ++pi) {
    TexturePlacement *placement = (*pi).second;
    if (placement->get_omit_reason() != OR_none) {
      DestTextureImage *dest = new DestTextureImage(placement);
      Filename filename = dest->get_filename();
      filename.make_canonical();

      pair<Dests::iterator, bool> insert_result = generate.insert
	(Dests::value_type(filename, dest));
      if (!insert_result.second) {
	// At least two DestTextureImages map to the same filename, no
	// sweat.
	delete dest;
	dest = (*insert_result.first).second;
      }

      placement->set_dest(dest);
    }
  }

  if (redo_all) {
    // If we're redoing everything, we remove everything first and
    // then recopy it again.
    Dests empty;
    remove_old_dests(empty, _dests);
    copy_new_dests(generate, empty);

  } else {
    // Otherwise, we only remove and recopy the things that changed
    // between this time and last time.
    remove_old_dests(generate, _dests);
    copy_new_dests(generate, _dests);
  }

  // Clean up the old set.
  Dests::iterator di;
  for (di = _dests.begin(); di != _dests.end(); ++di) {
    delete (*di).second;
  }

  _dests.swap(generate);
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::read_source_image
//       Access: Public
//  Description: Reads in the original image, if it has not already
//               been read, and returns it.
////////////////////////////////////////////////////////////////////
const PNMImage &TextureImage::
read_source_image() {
  if (!_read_source_image) {
    SourceTextureImage *source = get_preferred_source();
    if (source != (SourceTextureImage *)NULL) {
      source->read(_source_image);
    }
    _read_source_image = true;
    _ever_read_image = true;
  }

  return _source_image;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::get_dest_image
//       Access: Public
//  Description: Returns the image appropriate for writing to the
//               destination directory, having been resized and
//               everything.
////////////////////////////////////////////////////////////////////
const PNMImage &TextureImage::
get_dest_image() {
  if (!_got_dest_image) {
    const PNMImage &source_image = read_source_image();
    _dest_image.clear(get_x_size(), get_y_size(), get_num_channels(),
		      source_image.get_maxval());
    _dest_image.quick_filter_from(source_image);

    _got_dest_image = true;
  }

  return _dest_image;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::write_source_pathnames
//       Access: Public
//  Description: Writes the list of source pathnames that might
//               contribute to this texture to the indicated output
//               stream, one per line.
////////////////////////////////////////////////////////////////////
void TextureImage::
write_source_pathnames(ostream &out, int indent_level) const {
  Sources::const_iterator si;
  for (si = _sources.begin(); si != _sources.end(); ++si) {
    SourceTextureImage *source = (*si).second;

    indent(out, indent_level);
    source->output_filename(out);
    if (!source->is_size_known()) {
      out << " (unknown size)";

    } else {
      out << " " << source->get_x_size() << " " 
	  << source->get_y_size();

      if (source->get_properties().has_num_channels()) {
	out << " " << source->get_properties().get_num_channels();
      }
    }
    out << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::write_scale_info
//       Access: Public
//  Description: Writes the list of source pathnames that might
//               contribute to this texture to the indicated output
//               stream, one per line.
////////////////////////////////////////////////////////////////////
void TextureImage::
write_scale_info(ostream &out, int indent_level) {
  SourceTextureImage *source = get_preferred_source();
  indent(out, indent_level) << get_name();

  // Write the list of groups we're placed in.
  if (_placement.empty()) {
    out << " (not used)";
  } else {
    Placement::const_iterator pi;
    pi = _placement.begin();
    out << " (" << (*pi).second->get_group()->get_name();
    ++pi;
    while (pi != _placement.end()) {
      out << " " << (*pi).second->get_group()->get_name();
      ++pi;
    }
    out << ")";
  }

  out << " orig ";

  if (source == (SourceTextureImage *)NULL ||
      !source->is_size_known()) {
    out << "unknown";
  } else {
    out << source->get_x_size() << " " << source->get_y_size()
	<< " " << source->get_num_channels();
  }

  out << " new " << get_x_size() << " " << get_y_size()
      << " " << get_num_channels();

  if (source != (SourceTextureImage *)NULL &&
      source->is_size_known()) {
    double scale = 
      100.0 * (((double)get_x_size() / (double)source->get_x_size()) +
	       ((double)get_y_size() / (double)source->get_y_size())) / 2.0;
    out << " scale " << scale << "%";
  }
  out << "\n";
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::compute_egg_count
//       Access: Private
//  Description: Counts the number of egg files in the indicated set
//               that will be satisfied if a texture is assigned to
//               the indicated group.
////////////////////////////////////////////////////////////////////
int TextureImage::
compute_egg_count(PaletteGroup *group, 
		  const TextureImage::WorkingEggs &egg_files) {
  int count = 0;

  WorkingEggs::const_iterator ei;
  for (ei = egg_files.begin(); ei != egg_files.end(); ++ei) {
    if ((*ei)->get_complete_groups().count(group) != 0) {
      count++;
    }
  }

  return count;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::assign_to_groups
//       Access: Private
//  Description: Assigns the texture to the indicated set of groups.
//               If the texture was previously assigned to any of
//               these groups, keeps the same TexturePlacement object
//               for the assignment; at the same time, deletes any
//               TexturePlacement objects that represent groups we are
//               no longer assigned to.
////////////////////////////////////////////////////////////////////
void TextureImage::
assign_to_groups(const PaletteGroups &groups) {
  PaletteGroups::const_iterator gi;
  Placement::const_iterator pi;

  Placement new_placement;

  gi = groups.begin();
  pi = _placement.begin();

  while (gi != groups.end() && pi != _placement.end()) {
    PaletteGroup *a = (*gi);
    PaletteGroup *b = (*pi).first;

    if (a < b) {
      // Here's a group we're now assigned to that we weren't assigned
      // to previously.
      TexturePlacement *place = a->prepare(this);
      new_placement.insert
	(new_placement.end(), Placement::value_type(a, place));
      ++gi;

    } else if (b < a) {
      // Here's a group we're no longer assigned to.
      TexturePlacement *place = (*pi).second;
      delete place;
      ++pi;

    } else { // b == a
      // Here's a group we're still assigned to.
      TexturePlacement *place = (*pi).second;
      new_placement.insert
	(new_placement.end(), Placement::value_type(a, place));
      ++gi;
      ++pi;
    }
  }

  while (gi != groups.end()) {
    // Here's a group we're now assigned to that we weren't assigned
    // to previously.
    PaletteGroup *a = (*gi);
    TexturePlacement *place = a->prepare(this);
    new_placement.insert
      (new_placement.end(), Placement::value_type(a, place));
    ++gi;
  }

  while (pi != _placement.end()) {
    // Here's a group we're no longer assigned to.
    TexturePlacement *place = (*pi).second;
    delete place;
    ++pi;
  }

  _placement.swap(new_placement);
  _actual_assigned_groups = groups;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::consider_grayscale
//       Access: Private
//  Description: Examines the actual contents of the image to
//               determine if it should maybe be considered a
//               grayscale image (even though it has separate rgb
//               components).
////////////////////////////////////////////////////////////////////
void TextureImage::
consider_grayscale() {
  // Since this isn't likely to change for a particular texture after
  // its creation, we save a bit of time by not performing this check
  // unless this is the first time we've ever seen this texture.  This
  // will save us from having to load the texture images time we look
  // at them.  On the other hand, if we've already loaded up the
  // image, then go ahead.
  if (!_read_source_image && _ever_read_image) {
    if (_forced_grayscale) {
      _properties._num_channels -= 2;
    }
    return;
  }

  const PNMImage &source = read_source_image();
  if (!source.is_valid()) {
    return;
  }

  for (int y = 0; y < source.get_y_size(); y++) {
    for (int x = 0; x < source.get_x_size(); x++) {
      const xel &v = source.get_xel_val(x, y);
      if (PPM_GETR(v) != PPM_GETG(v) || PPM_GETR(v) != PPM_GETB(v)) {
	// Here's a colored pixel.  We can't go grayscale.
	_forced_grayscale = false;
	return;
      }
    }
  }

  // All pixels in the image were grayscale!
  _properties._num_channels -= 2;
  _forced_grayscale = true;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::consider_unalpha
//       Access: Private
//  Description: Examines the actual contents of the image to
//               determine if its alpha channel should be eliminated
//               (e.g. it's completely white, and therefore
//               pointless).
////////////////////////////////////////////////////////////////////
void TextureImage::
consider_unalpha() {
  // As above, we don't bother doing this if we've already done this
  // in a previous session.
  if (!_read_source_image && _ever_read_image) {
    if (_forced_unalpha) {
      _properties._num_channels--;
    }
    return;
  }

  const PNMImage &source = read_source_image();
  if (!source.is_valid()) {
    return;
  }

  if (!source.has_alpha()) {
    return;
  }

  for (int y = 0; y < source.get_y_size(); y++) {
    for (int x = 0; x < source.get_x_size(); x++) {
      if (source.get_alpha_val(x, y) != source.get_maxval()) {
	// Here's a non-white pixel; the alpha channel is meaningful.
	_forced_unalpha = false;
	return;
      }
    }
  }

  // All alpha pixels in the image were white!
  _properties._num_channels--;
  _forced_unalpha = true;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::remove_old_dests
//       Access: Private
//  Description: Removes all of the filenames named in b that are not
//               also named in a.
////////////////////////////////////////////////////////////////////
void TextureImage::
remove_old_dests(const TextureImage::Dests &a, const TextureImage::Dests &b) {
  Dests::const_iterator ai = a.begin();
  Dests::const_iterator bi = b.begin();

  while (ai != a.end() && bi != b.end()) {
    const string &astr = (*ai).first;
    const string &bstr = (*bi).first;

    if (astr < bstr) {
      // Here's a filename in a, not in b.
      ++ai;

    } else if (bstr < astr) {
      // Here's a filename in b, not in a.
      (*bi).second->unlink();
      ++bi;

    } else { // bstr == astr
      // Here's a filename in both a and b.
      ++ai;
      ++bi;
    }
  }

  while (bi != b.end()) {
    // Here's a filename in b, not in a.
    (*bi).second->unlink();
    ++bi;
  }

  while (ai != a.end()) {
    ++ai;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::copy_new_dests
//       Access: Private
//  Description: Copies a resized texture into each filename named in
//               a that is not also listed in b, or whose
//               corresponding listing in b is out of date.
////////////////////////////////////////////////////////////////////
void TextureImage::
copy_new_dests(const TextureImage::Dests &a, const TextureImage::Dests &b) {
  Dests::const_iterator ai = a.begin();
  Dests::const_iterator bi = b.begin();

  while (ai != a.end() && bi != b.end()) {
    const string &astr = (*ai).first;
    const string &bstr = (*bi).first;

    if (astr < bstr) {
      // Here's a filename in a, not in b.
      (*ai).second->copy(this);
      ++ai;

    } else if (bstr < astr) {
      // Here's a filename in b, not in a.
      ++bi;

    } else { // bstr == astr
      // Here's a filename in both a and b.
      (*ai).second->copy_if_stale((*bi).second, this);
      ++ai;
      ++bi;
    }
  }

  while (ai != a.end()) {
    // Here's a filename in a, not in b.
    (*ai).second->copy(this);
    ++ai;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::get_source_key
//       Access: Private
//  Description: Returns the key that a SourceTextureImage should be
//               stored in, given its one or two filenames.
////////////////////////////////////////////////////////////////////
string TextureImage::
get_source_key(const Filename &filename, const Filename &alpha_filename) {
  Filename f = FilenameUnifier::make_bam_filename(filename);
  Filename a = FilenameUnifier::make_bam_filename(alpha_filename);

  return f.get_fullpath() + ":" + a.get_fullpath();
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::register_with_read_factory
//       Access: Public, Static
//  Description: Registers the current object as something that can be
//               read from a Bam file.
////////////////////////////////////////////////////////////////////
void TextureImage::
register_with_read_factory() {
  BamReader::get_factory()->
    register_factory(get_class_type(), make_TextureImage);
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::write_datagram
//       Access: Public, Virtual
//  Description: Fills the indicated datagram up with a binary
//               representation of the current object, in preparation
//               for writing to a Bam file.
////////////////////////////////////////////////////////////////////
void TextureImage::
write_datagram(BamWriter *writer, Datagram &datagram) {
  ImageFile::write_datagram(writer, datagram);
  datagram.add_string(get_name());

  // We don't write out _request; this is re-read from the .txa file
  // each time.

  // We don't write out _pre_txa_properties; this is transitional.

  // We don't write out _preferred_source; this is redetermined each
  // session.

  datagram.add_bool(_is_surprise);
  datagram.add_bool(_ever_read_image);
  datagram.add_bool(_forced_grayscale);
  datagram.add_bool(_forced_unalpha);

  // We don't write out _explicitly_assigned_groups; this is re-read
  // from the .txa file each time.

  _actual_assigned_groups.write_datagram(writer, datagram);

  // We don't write out _egg_files; this is redetermined each session.

  datagram.add_uint32(_placement.size());
  Placement::const_iterator pi;
  for (pi = _placement.begin(); pi != _placement.end(); ++pi) {
    writer->write_pointer(datagram, (*pi).first);
    writer->write_pointer(datagram, (*pi).second);
  }

  datagram.add_uint32(_sources.size());
  Sources::const_iterator si;
  for (si = _sources.begin(); si != _sources.end(); ++si) {
    writer->write_pointer(datagram, (*si).second);
  }

  datagram.add_uint32(_dests.size());
  Dests::const_iterator di;
  for (di = _dests.begin(); di != _dests.end(); ++di) {
    writer->write_pointer(datagram, (*di).second);
  }

  // We don't write out _read_source_image, _source_image,
  // _got_dest_image, or _dest_image; these must be reread each
  // session.
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::complete_pointers
//       Access: Public, Virtual
//  Description: Called after the object is otherwise completely read
//               from a Bam file, this function's job is to store the
//               pointers that were retrieved from the Bam file for
//               each pointer object written.  The return value is the
//               number of pointers processed from the list.
////////////////////////////////////////////////////////////////////
int TextureImage::
complete_pointers(vector_typedWriteable &plist, BamReader *manager) {
  nassertr((int)plist.size() >= _num_placement * 2 + _num_sources + _num_dests, 0);
  int index = 0;

  int i;
  for (i = 0; i < _num_placement; i++) {
    PaletteGroup *group;
    TexturePlacement *placement;
    DCAST_INTO_R(group, plist[index], index);
    index++;
    DCAST_INTO_R(placement, plist[index], index);
    index++;
    _placement.insert(Placement::value_type(group, placement));
  }

  for (i = 0; i < _num_sources; i++) {
    SourceTextureImage *source;
    DCAST_INTO_R(source, plist[index], index);
    string key = get_source_key(source->get_filename(), 
				source->get_alpha_filename());

    bool inserted = _sources.insert(Sources::value_type(key, source)).second;
    index++;
    nassertr(inserted, index);
  }

  for (i = 0; i < _num_dests; i++) {
    DestTextureImage *dest;
    DCAST_INTO_R(dest, plist[index], index);
    bool inserted = _dests.insert(Dests::value_type(dest->get_filename(), dest)).second;
    index++;
    nassertr(inserted, index);
  }

  return index;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::make_TextureImage
//       Access: Protected
//  Description: This method is called by the BamReader when an object
//               of this type is encountered in a Bam file; it should
//               allocate and return a new object with all the data
//               read.
////////////////////////////////////////////////////////////////////
TypedWriteable *TextureImage::
make_TextureImage(const FactoryParams &params) {
  TextureImage *me = new TextureImage;
  BamReader *manager;
  Datagram packet;

  parse_params(params, manager, packet);
  DatagramIterator scan(packet);

  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: TextureImage::fillin
//       Access: Protected
//  Description: Reads the binary data from the given datagram
//               iterator, which was written by a previous call to
//               write_datagram().
////////////////////////////////////////////////////////////////////
void TextureImage::
fillin(DatagramIterator &scan, BamReader *manager) {
  ImageFile::fillin(scan, manager);
  set_name(scan.get_string());

  _is_surprise = scan.get_bool();
  _ever_read_image = scan.get_bool();
  _forced_grayscale = scan.get_bool();
  _forced_unalpha = scan.get_bool();

  _actual_assigned_groups.fillin(scan, manager);

  _num_placement = scan.get_uint32();
  manager->read_pointers(scan, this, _num_placement * 2);

  _num_sources = scan.get_uint32();
  manager->read_pointers(scan, this, _num_sources);
  _num_dests = scan.get_uint32();
  manager->read_pointers(scan, this, _num_dests);
}