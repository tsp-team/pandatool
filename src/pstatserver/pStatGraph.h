// Filename: pStatGraph.h
// Created by:  drose (19Jul00)
// 
////////////////////////////////////////////////////////////////////

#ifndef PSTATGRAPH_H
#define PSTATGRAPH_H

#include <pandatoolbase.h>

#include "pStatMonitor.h"
#include "pStatClientData.h"

#include <luse.h>
#include <vector_int.h>

#include <map>

class PStatView;

////////////////////////////////////////////////////////////////////
//       Class : PStatGraph
// Description : This is an abstract base class for several different
//               kinds of graphs that have a few things in common,
//               like labels and guide bars.
////////////////////////////////////////////////////////////////////
class PStatGraph {
public:
  PStatGraph(PStatMonitor *monitor, int xsize, int ysize);
  virtual ~PStatGraph();

  INLINE PStatMonitor *get_monitor() const;

  INLINE int get_num_labels() const;
  INLINE int get_label_collector(int n) const;
  INLINE string get_label_name(int n) const;
  INLINE RGBColorf get_label_color(int n) const;

  INLINE void set_target_frame_rate(float frame_rate);
  INLINE float get_target_frame_rate() const;

  INLINE int get_xsize() const;
  INLINE int get_ysize() const;

  class GuideBar {
  public:
    GuideBar(float height, const string &label, bool is_target);
    GuideBar(const GuideBar &copy);

    float _height;
    string _label;
    bool _is_target;
  };

  enum GuideBarUnits {
    GBU_hz         = 0x0001,
    GBU_ms         = 0x0002,
    GBU_named      = 0x0004,
    GBU_show_units = 0x0008,
  };

  int get_num_guide_bars() const;
  const GuideBar &get_guide_bar(int n) const;

  INLINE void set_guide_bar_units(int unit_mask);
  INLINE int get_guide_bar_units() const;
  INLINE void set_guide_bar_unit_name(const string &unit_name);
  INLINE const string &get_guide_bar_unit_name() const;

  static string format_number(float value);
  static string format_number(float value, int guide_bar_units, 
                              const string &unit_name = string());

protected:
  virtual void normal_guide_bars()=0;
  void update_guide_bars(int num_bars, float scale);
  GuideBar make_guide_bar(float value) const;

  bool _labels_changed;
  bool _guide_bars_changed;

  PT(PStatMonitor) _monitor;

  float _target_frame_rate;

  int _xsize;
  int _ysize;

  // Table of the collectors that should be drawn as labels, in order
  // from bottom to top.
  typedef vector_int Labels;
  Labels _labels;

  typedef vector<GuideBar> GuideBars;
  GuideBars _guide_bars;
  int _guide_bar_units;
  string _unit_name;
};

#include "pStatGraph.I"

#endif
