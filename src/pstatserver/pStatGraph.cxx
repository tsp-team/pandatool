// Filename: pStatGraph.cxx
// Created by:  drose (19Jul00)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#include "pStatGraph.h"

#include <pStatFrameData.h>
#include <pStatCollectorDef.h>
#include <string_utils.h>
#include <config_pstats.h>

#include <stdio.h>  // for sprintf

////////////////////////////////////////////////////////////////////
//     Function: PStatGraph::GuideBar::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
PStatGraph::GuideBar::
GuideBar(float height, const string &label, bool is_target) :
  _height(height),
  _label(label),
  _is_target(is_target)
{
}

////////////////////////////////////////////////////////////////////
//     Function: PStatGraph::GuideBar::Copy Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
PStatGraph::GuideBar::
GuideBar(const PStatGraph::GuideBar &copy) :
  _height(copy._height),
  _label(copy._label),
  _is_target(copy._is_target)
{
}

////////////////////////////////////////////////////////////////////
//     Function: PStatGraph::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
PStatGraph::
PStatGraph(PStatMonitor *monitor, int xsize, int ysize) :
  _monitor(monitor),
  _xsize(xsize),
  _ysize(ysize)
{
  _target_frame_rate = pstats_target_frame_rate;
  _labels_changed = false;
  _guide_bars_changed = false;
  _guide_bar_units = GBU_ms;
}

////////////////////////////////////////////////////////////////////
//     Function: PStatGraph::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
PStatGraph::
~PStatGraph() {
}

////////////////////////////////////////////////////////////////////
//     Function: PStatGraph::get_num_guide_bars
//       Access: Public
//  Description: Returns the number of horizontal guide bars that
//               should be drawn, based on the indicated target frame
//               rate.  Not all of these may be visible; some may be
//               off the top of the chart because of the vertical
//               scale.
////////////////////////////////////////////////////////////////////
int PStatGraph::
get_num_guide_bars() const {
  return _guide_bars.size();
}

////////////////////////////////////////////////////////////////////
//     Function: PStatGraph::get_guide_bar
//       Access: Public
//  Description: Returns the nth horizontal guide bar.  This should be
//               drawn as a horizontal line across the chart at the y
//               pixel location determined by height_to_pixel(bar._height).
//
//               It is possible that this bar will be off the top of
//               the chart.
////////////////////////////////////////////////////////////////////
const PStatGraph::GuideBar &PStatGraph::
get_guide_bar(int n) const {
#ifndef NDEBUG
  static GuideBar bogus_bar(0.0, "bogus", false);
  nassertr(n >= 0 && n < (int)_guide_bars.size(), bogus_bar);
#endif
  return _guide_bars[n];
}


////////////////////////////////////////////////////////////////////
//     Function: PStatGraph::format_number
//       Access: Public, Static
//  Description: Returns a string representing the value nicely
//               formatted for its range.
////////////////////////////////////////////////////////////////////
string PStatGraph::
format_number(float value) {
  char buffer[128];

  if (value < 0.01) {
    sprintf(buffer, "%0.4f", value);
  } else if (value < 0.1) {
    sprintf(buffer, "%0.3f", value);
  } else if (value < 1.0) {
    sprintf(buffer, "%0.2f", value);
  } else if (value < 10.0) {
    sprintf(buffer, "%0.1f", value);
  } else {
    sprintf(buffer, "%0.0f", value);
  }

  return buffer;
}

////////////////////////////////////////////////////////////////////
//     Function: PStatGraph::format_number
//       Access: Public, Static
//  Description: Returns a string representing the value nicely
//               formatted for its range, including the units
//               as indicated.
////////////////////////////////////////////////////////////////////
string PStatGraph::
format_number(float value, int guide_bar_units, const string &unit_name) {
  string label;

  if ((guide_bar_units & GBU_named) != 0) {
    // Units are whatever is specified by unit_name, not a time unit
    // at all.
    label = format_number(value);
    if ((guide_bar_units & GBU_show_units) != 0 && !unit_name.empty()) {
      label += " ";
      label += unit_name;
    }

  } else {
    // Units are either milliseconds or hz, or both.
    if ((guide_bar_units & GBU_ms) != 0) {
      float ms = value * 1000.0;
      label += format_number(ms);
      if ((guide_bar_units & GBU_show_units) != 0) {
        label += " ms";
      }
    }

    if ((guide_bar_units & GBU_hz) != 0) {
      float hz = 1.0 / value;

      if ((guide_bar_units & GBU_ms) != 0) {
        label += " (";
      }
      label += format_number(hz);
      if ((guide_bar_units & GBU_show_units) != 0) {
        label += " Hz";
      }
      if ((guide_bar_units & GBU_ms) != 0) {
        label += ")";
      }
    }
  }

  return label;
}

////////////////////////////////////////////////////////////////////
//     Function: PStatGraph::update_guide_bars
//       Access: Protected
//  Description: Resets the list of guide bars.
////////////////////////////////////////////////////////////////////
void PStatGraph::
update_guide_bars(int num_bars, float scale) {
  _guide_bars.clear();

  // We'd like to draw about num_bars bars on the chart.  But we also
  // want the bars to be harmonics of the target frame rate, so that
  // the bottom bar is at tfr/n or n * tfr, where n is an integer, and
  // the upper bars are even multiples of that.

  // Choose a suitable harmonic of the target frame rate near the
  // bottom part of the chart.

  float bottom = (float)num_bars / scale;

  float harmonic;
  if (_target_frame_rate < bottom) {
    // n * tfr
    harmonic = floor(bottom / _target_frame_rate + 0.5) * _target_frame_rate;

  } else {
    // tfr / n
    harmonic = _target_frame_rate / floor(_target_frame_rate / bottom + 0.5);
  }

  // Now, make a few bars at k / harmonic.
  for (int k = 1; k / harmonic <= scale; k++) {
    _guide_bars.push_back(make_guide_bar(k / harmonic));
  }

  _guide_bars_changed = true;
}

////////////////////////////////////////////////////////////////////
//     Function: PStatGraph::make_guide_bar
//       Access: Protected
//  Description: Makes a guide bar for the indicated elapsed time or
//               level units.
////////////////////////////////////////////////////////////////////
PStatGraph::GuideBar PStatGraph::
make_guide_bar(float value) const {
  string label = format_number(value, _guide_bar_units, _unit_name);

  bool is_target = false;

  if ((_guide_bar_units & GBU_named) == 0) {
    // If it's a time unit, check to see if it matches our target
    // frame rate.
    float hz = 1.0 / value;
    is_target = IS_NEARLY_EQUAL(hz, _target_frame_rate);
  }

  return GuideBar(value, label, is_target);
}
