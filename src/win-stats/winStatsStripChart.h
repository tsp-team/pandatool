// Filename: winStatsStripChart.h
// Created by:  drose (03Dec03)
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

#ifndef WINSTATSSTRIPCHART_H
#define WINSTATSSTRIPCHART_H

#include "pandatoolbase.h"

#include "winStatsGraph.h"
#include "pStatStripChart.h"
#include "pointerTo.h"

#include <windows.h>

class WinStatsMonitor;

////////////////////////////////////////////////////////////////////
//       Class : WinStatsStripChart
// Description : A window that draws a strip chart, given a view.
////////////////////////////////////////////////////////////////////
class WinStatsStripChart : public PStatStripChart, public WinStatsGraph {
public:
  WinStatsStripChart(WinStatsMonitor *monitor,
                     int thread_index, int collector_index, bool show_level);
  virtual ~WinStatsStripChart();

  virtual void new_collector(int collector_index);
  virtual void new_data(int thread_index, int frame_number);
  virtual void force_redraw();
  virtual void changed_graph_size(int graph_xsize, int graph_ysize);

  virtual void set_time_units(int unit_mask);
  virtual void set_scroll_speed(float scroll_speed);
  virtual void clicked_label(int collector_index);
  void set_vertical_scale(float value_height);

protected:
  virtual void update_labels();

  virtual void clear_region();
  virtual void copy_region(int start_x, int end_x, int dest_x);
  virtual void draw_slice(int x, int w, int frame_number);
  virtual void draw_empty(int x, int w);
  virtual void draw_cursor(int x);
  virtual void end_draw(int from_x, int to_x);

  LONG window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  virtual LONG graph_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  virtual void additional_window_paint(HDC hdc);
  virtual void additional_graph_window_paint(HDC hdc);
  virtual DragMode consider_drag_start(int mouse_x, int mouse_y, 
                                       int width, int height);

private:
  void draw_guide_bar(HDC hdc, int from_x, int to_x, const GuideBar &bar);
  int draw_guide_label(HDC hdc, int x, const GuideBar &bar, int last_y);
  void create_window();
  static void register_window_class(HINSTANCE application);

  static LONG WINAPI static_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  int _brush_origin;
  string _net_value_text;

  static bool _window_class_registered;
  static const char * const _window_class_name;
};

#endif
