// Filename: fltInstanceRef.cxx
// Created by:  drose (30Aug00)
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

#include "fltInstanceRef.h"
#include "fltRecordReader.h"
#include "fltRecordWriter.h"
#include "fltInstanceDefinition.h"
#include "fltHeader.h"

TypeHandle FltInstanceRef::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: FltInstanceRef::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
FltInstanceRef::
FltInstanceRef(FltHeader *header) : FltBead(header) {
  _instance_index = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: FltInstanceRef::get_instance
//       Access: Public
//  Description: Returns the instance subtree referenced by this node,
//               or NULL if the reference is invalid.
////////////////////////////////////////////////////////////////////
FltInstanceDefinition *FltInstanceRef::
get_instance() const {
  return _header->get_instance(_instance_index);
}

////////////////////////////////////////////////////////////////////
//     Function: FltInstanceRef::write
//       Access: Public
//  Description: Writes a multiple-line description of the record and
//               all of its children.  This is a human-readable
//               description, primarily for debugging; to write a flt
//               file, use FltHeader::write_flt().
////////////////////////////////////////////////////////////////////
void FltInstanceRef::
write(ostream &out, int indent_level) const {
  indent(out, indent_level) << "instance";
  FltInstanceDefinition *def = _header->get_instance(_instance_index);
  if (def != (FltInstanceDefinition *)NULL) {
    def->write_children(out, indent_level + 2);
    indent(out, indent_level) << "}\n";
  } else {
    out << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: FltInstanceRef::extract_record
//       Access: Protected, Virtual
//  Description: Fills in the information in this bead based on the
//               information given in the indicated datagram, whose
//               opcode has already been read.  Returns true on
//               success, false if the datagram is invalid.
////////////////////////////////////////////////////////////////////
bool FltInstanceRef::
extract_record(FltRecordReader &reader) {
  if (!FltBead::extract_record(reader)) {
    return false;
  }

  nassertr(reader.get_opcode() == FO_instance_ref, false);
  DatagramIterator &iterator = reader.get_iterator();

  iterator.skip_bytes(2);
  _instance_index = iterator.get_be_int16();

  check_remaining_size(iterator);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: FltInstanceRef::write_record_and_children
//       Access: Protected, Virtual
//  Description: Writes this record out to the flt file, along with all
//               of its ancillary records and children records.  Returns
//               FE_ok on success, or something else on error.
////////////////////////////////////////////////////////////////////
FltError FltInstanceRef::
write_record_and_children(FltRecordWriter &writer) const {
  // First, make sure our instance definition has already been written.
  FltError result = writer.write_instance_def(_header, _instance_index);
  if (result != FE_ok) {
    return result;
  }

  // Then write out our own record.
  return FltBead::write_record_and_children(writer);
}

////////////////////////////////////////////////////////////////////
//     Function: FltInstanceRef::build_record
//       Access: Protected, Virtual
//  Description: Fills up the current record on the FltRecordWriter with
//               data for this record, but does not advance the
//               writer.  Returns true on success, false if there is
//               some error.
////////////////////////////////////////////////////////////////////
bool FltInstanceRef::
build_record(FltRecordWriter &writer) const {
  if (!FltBead::build_record(writer)) {
    return false;
  }

  writer.set_opcode(FO_instance_ref);
  Datagram &datagram = writer.update_datagram();

  datagram.pad_bytes(2);
  datagram.add_be_int16(_instance_index);

  return true;
}
