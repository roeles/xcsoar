/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Topology/TopologyStore.hpp"
#include "Topology/TopologyFile.hpp"
#include "StringUtil.hpp"
#include "IO/LineReader.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"

void
TopologyStore::ScanVisibility(const WindowProjection &m_projection)
{
  // check if any needs to have cache updates because wasnt
  // visible previously when bounds moved

  // we will make sure we update at least one cache per call
  // to make sure eventually everything gets refreshed
  for (int z = 0; z < MAXTOPOLOGY; z++) {
    if (topology_store[z]) {
      topology_store[z]->updateCache(m_projection);
    }
  }
}

TopologyStore::~TopologyStore()
{
  Reset();
}

/**
 * Draws the topology to the given canvas
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void
TopologyStore::Draw(Canvas &canvas, BitmapCanvas &bitmap_canvas,
                    const WindowProjection &projection) const
{
  for (int z = 0; z < MAXTOPOLOGY; z++) {
    if (topology_store[z])
      topology_store[z]->Paint(canvas, bitmap_canvas, projection);
  }
}

void
TopologyStore::DrawLabels(Canvas &canvas, const WindowProjection &projection,
                          LabelBlock &label_block,
                          const SETTINGS_MAP &settings_map) const
{
  for (unsigned i = 0; i < MAXTOPOLOGY; i++)
    if (topology_store[i] != NULL)
      topology_store[i]->PaintLabels(canvas, projection,
                                     label_block, settings_map);
}

TopologyStore::TopologyStore()
{
  for (int z = 0; z < MAXTOPOLOGY; z++) {
    topology_store[z] = NULL;
  }
}

void
TopologyStore::Load(NLineReader &reader, const TCHAR* Directory)
{
  Reset();

  double ShapeRange;
  long ShapeIcon;
  long ShapeField;
  int numtopo = 0;
  char ShapeFilename[MAX_PATH];

  strcpy(ShapeFilename, NarrowPathName(Directory));
  strcat(ShapeFilename, DIR_SEPARATOR_S);

  char *ShapeFilenameEnd = ShapeFilename + strlen(ShapeFilename);

  char *line;
  while ((line = reader.read()) != NULL) {
    // Look For Comment
    if (string_is_empty(line) || line[0] == '*')
      continue;

    BYTE red, green, blue;
    // filename,range,icon,field

    // File name
    char *p = strchr(line, ',');
    if (p == NULL || p == line)
      continue;

    memcpy(ShapeFilenameEnd, line, p - line);
    strcpy(ShapeFilenameEnd + (p - line), ".shp");

    // Shape range
    ShapeRange = strtod(p + 1, &p);
    if (*p != _T(','))
      continue;

    // Shape icon
    ShapeIcon = strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Shape field for text display
    // sjt 02NOV05 - field parameter enabled
    ShapeField = strtol(p + 1, &p, 10) - 1;
    if (*p != _T(','))
      continue;

    // Red component of line / shading colour
    red = (BYTE)strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Green component of line / shading colour
    green = (BYTE)strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Blue component of line / shading colour
    blue = (BYTE)strtol(p + 1, NULL, 10);

    if ((red == 64) && (green == 96) && (blue == 240)) {
      // JMW update colours to ICAO standard
      red = 85; // water colours
      green = 160;
      blue = 255;
    }

    topology_store[numtopo] = new TopologyFile(ShapeFilename,
                                               fixed(ShapeRange) * 1000,
                                               Color(red, green, blue),
                                               ShapeField, ShapeIcon);

    numtopo++;
  }
}

void
TopologyStore::Reset()
{
  for (int z = 0; z < MAXTOPOLOGY; z++) {
    delete topology_store[z];
    topology_store[z] = NULL;
  }
}
