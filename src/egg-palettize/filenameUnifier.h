// Filename: filenameUnifier.h
// Created by:  drose (05Dec00)
// 
////////////////////////////////////////////////////////////////////

#ifndef FILENAMEUNIFIER_H
#define FILENAMEUNIFIER_H

#include <pandatoolbase.h>

#include <filename.h>

////////////////////////////////////////////////////////////////////
// 	 Class : FilenameUnifier
// Description : This static class does the job of converting
//               filenames from relative to absolute to canonical or
//               whatever is appropriate.  Its main purpose is to
//               allow us to write relative pathnames to the bam file
//               and turn them back into absolute pathnames on read,
//               so that a given bam file does not get tied to
//               absolute pathnames.
////////////////////////////////////////////////////////////////////
class FilenameUnifier {
public:
  static void set_txa_filename(const Filename &txa_filename);
  static void set_rel_dirname(const Filename &rel_dirname);

  static Filename make_bam_filename(Filename filename);
  static Filename get_bam_filename(Filename filename);
  static Filename make_egg_filename(Filename filename);
  static Filename make_user_filename(Filename filename);

private:
  static Filename _txa_filename;
  static Filename _txa_dir;
  static Filename _rel_dirname;
};

#endif
