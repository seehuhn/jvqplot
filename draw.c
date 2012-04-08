/* draw.c - draw the graphs
 *
 * Copyright (C) 2012  Jochen Voss.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <math.h>

#include <cairo.h>

#include "jvqplot.h"


double xres = -1, yres;

static struct {
  double r, g, b;
} colors[100] = {
  { 0.929688, 0.054688, 0.226562},
  { 0.027344, 0.597656, 0.238281},
  { 0.039062, 0.132812, 0.929688},
  { 0.750000, 0.250000, 0.750000},
  { 0.375000, 0.375000, 0.625000},
  { 0.625000, 0.125000, 0.375000},
  { 0.187500, 0.312500, 0.312500},
  { 0.937500, 0.062500, 0.562500},
  { 0.437500, 0.562500, 0.062500},
  { 0.312500, 0.187500, 0.937500},
  { 0.562500, 0.437500, 0.187500},
  { 0.093750, 0.468750, 0.843750},
  { 0.843750, 0.218750, 0.093750},
  { 0.468750, 0.093750, 0.468750},
  { 0.718750, 0.343750, 0.718750},
  { 0.156250, 0.156250, 0.531250},
  { 0.281250, 0.281250, 0.156250},
  { 0.531250, 0.031250, 0.906250},
  { 0.031250, 0.531250, 0.406250},
  { 0.046875, 0.265625, 0.609375},
  { 0.796875, 0.015625, 0.359375},
  { 0.296875, 0.515625, 0.859375},
  { 0.421875, 0.140625, 0.234375},
  { 0.234375, 0.078125, 0.796875},
  { 0.984375, 0.328125, 0.046875},
  { 0.359375, 0.453125, 0.421875},
  { 0.609375, 0.203125, 0.671875},
  { 0.109375, 0.703125, 0.171875},
  { 0.078125, 0.234375, 0.265625},
  { 0.453125, 0.359375, 0.890625},
  { 0.703125, 0.109375, 0.140625},
  { 0.203125, 0.609375, 0.640625},
  { 0.140625, 0.421875, 0.078125},
  { 0.890625, 0.171875, 0.828125},
  { 0.265625, 0.046875, 0.703125},
  { 0.515625, 0.296875, 0.453125},
  { 0.023438, 0.398438, 0.445312},
  { 0.773438, 0.148438, 0.695312},
  { 0.273438, 0.648438, 0.195312},
  { 0.398438, 0.023438, 0.820312},
  { 0.648438, 0.273438, 0.070312},
  { 0.210938, 0.210938, 0.132812},
  { 0.335938, 0.335938, 0.507812},
  { 0.585938, 0.085938, 0.257812},
  { 0.085938, 0.585938, 0.757812},
  { 0.117188, 0.117188, 0.664062},
  { 0.492188, 0.492188, 0.039062},
  { 0.742188, 0.242188, 0.789062},
  { 0.179688, 0.304688, 0.976562},
  { 0.304688, 0.179688, 0.351562},
  { 0.554688, 0.429688, 0.601562},
  { 0.789062, 0.382812, 0.179688},
  { 0.414062, 0.257812, 0.304688},
  { 0.664062, 0.007812, 0.554688},
  { 0.164062, 0.507812, 0.054688},
  { 0.226562, 0.445312, 0.742188},
  { 0.976562, 0.195312, 0.492188},
  { 0.351562, 0.070312, 0.117188},
  { 0.601562, 0.320312, 0.867188},
  { 0.070312, 0.351562, 0.210938},
  { 0.820312, 0.101562, 0.960938},
  { 0.445312, 0.226562, 0.585938},
  { 0.132812, 0.039062, 0.398438},
  { 0.257812, 0.414062, 0.773438},
  { 0.507812, 0.164062, 0.023438},
  { 0.007812, 0.664062, 0.523438},
  { 0.011719, 0.332031, 0.785156},
  { 0.761719, 0.082031, 0.035156},
  { 0.261719, 0.582031, 0.535156},
  { 0.386719, 0.207031, 0.410156},
  { 0.199219, 0.019531, 0.597656},
  { 0.949219, 0.269531, 0.347656},
  { 0.324219, 0.394531, 0.222656},
  { 0.574219, 0.144531, 0.972656},
  { 0.074219, 0.644531, 0.472656},
  { 0.105469, 0.175781, 0.066406},
  { 0.480469, 0.300781, 0.691406},
  { 0.730469, 0.050781, 0.441406},
  { 0.230469, 0.550781, 0.941406},
  { 0.167969, 0.488281, 0.253906},
  { 0.917969, 0.238281, 0.503906},
  { 0.292969, 0.113281, 0.878906},
  { 0.542969, 0.363281, 0.128906},
  { 0.058594, 0.066406, 0.332031},
  { 0.808594, 0.316406, 0.582031},
  { 0.433594, 0.441406, 0.957031},
  { 0.683594, 0.191406, 0.207031},
  { 0.246094, 0.253906, 0.019531},
  { 0.996094, 0.003906, 0.769531},
  { 0.496094, 0.503906, 0.269531},
  { 0.371094, 0.128906, 0.644531},
  { 0.621094, 0.378906, 0.394531},
  { 0.089844, 0.410156, 0.550781},
  { 0.839844, 0.160156, 0.300781},
  { 0.464844, 0.035156, 0.175781},
  { 0.714844, 0.285156, 0.925781},
  { 0.152344, 0.222656, 0.863281},
  { 0.277344, 0.347656, 0.488281},
  { 0.527344, 0.097656, 0.738281},
  { 0.500000, 0.500000, 0.500000},
};

void
draw_graph(cairo_t *cr, struct layout *L, gboolean is_screen)
{
  int i, j, k;

  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_paint(cr);

  if (! state->dataset_used)
    goto draw_message;

  /* minor grid lines */
  cairo_set_line_width(cr, 1);
  cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
  for (i=ceil(-L->bx/L->ax/L->dx); ; ++i) {
    double wx = L->ax*i*L->dx + L->bx;
    if (wx > L->width) break;
    if (i%L->xmult == 0) continue;
    if (is_screen) wx = floor(wx) + .5;
    cairo_move_to(cr, wx, 0);
    cairo_line_to(cr, wx, L->height);
  }
  for (i=ceil((L->height-L->by)/L->ay/L->dy); ; ++i) {
    double wy = L->ay*i*L->dy + L->by;
    if (wy < 0) break;
    if (i%L->ymult == 0) continue;
    if (is_screen) wy = floor(wy) + .5;
    cairo_move_to(cr, 0, wy);
    cairo_line_to(cr, L->width, wy);
  }
  cairo_stroke(cr);

  /* major grid lines */
  cairo_set_line_width(cr, 2);
  cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
  for (i=ceil(-L->bx/L->ax/L->dx); ; ++i) {
    double wx = L->ax*i*L->dx + L->bx;
    if (wx > L->width) break;
    if (i%L->xmult) continue;
    if (is_screen) wx = floor(wx) + .5;
    cairo_move_to(cr, wx, 0);
    cairo_line_to(cr, wx, L->height);
  }
  for (i=ceil((L->height-L->by)/L->ay/L->dy); ; ++i) {
    double wy = L->ay*i*L->dy + L->by;
    if (wy < 0) break;
    if (i%L->ymult) continue;
    if (is_screen) wy = floor(wy) + .5;
    cairo_move_to(cr, 0, wy);
    cairo_line_to(cr, L->width, wy);
  }
  cairo_stroke(cr);

  /* grid labels */
  cairo_select_font_face(cr, "sans-serif",
                         CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 12.0);
  for (i=ceil(-L->bx/L->ax/L->dx); ; ++i) {
    double wx = L->ax*i*L->dx + L->bx;
    if (wx > L->width) break;
    if (i%L->xmult) continue;
    if (is_screen) wx = floor(wx) + .5;

    char buffer[32];
    snprintf(buffer, 32, "%g", i*L->dx);

    cairo_text_extents_t te;
    cairo_text_extents(cr, buffer, &te);

    double xpos = wx - te.x_bearing - .5*te.width;
    if (xpos+te.x_bearing+te.width+2 > L->width) {
      /* make sure the right-most label is visible */
      xpos = L->width-te.x_bearing-te.width-2;
    }
    cairo_rectangle(cr, xpos+te.x_bearing-2, L->height-8+te.y_bearing-2,
                    te.width+4, te.height+4);
    cairo_set_source_rgba(cr, 1, 1, 1, .8);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, xpos, L->height-8);
    cairo_show_text(cr, buffer);
  }
  for (i=ceil((L->height-L->by)/L->ay/L->dy); ; ++i) {
    double wy = L->ay*i*L->dy + L->by;
    if (wy < 0) break;
    if (i%L->ymult) continue;
    if (is_screen) wy = floor(wy) + .5;

    char buffer[32];
    snprintf(buffer, 32, "%g", i*L->dy);

    cairo_text_extents_t te;
    cairo_text_extents(cr, buffer, &te);

    cairo_rectangle(cr, 8+te.x_bearing-2, wy+4+te.y_bearing-2,
                    te.width+4, te.height+4);
    cairo_set_source_rgba(cr, 1, 1, 1, .8);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 8, wy+4);
    cairo_show_text(cr, buffer);
  }

  /* graphs for the data */
  for (k=0; k<state->dataset_used; ++k) {
    int cols = state->dataset[k].cols;
    int rows = state->dataset[k].rows;
    double *data = state->dataset[k].data;
    for (j=1; j<cols; ++j) {
      cairo_set_source_rgba(cr, 1, 1, 1, .5);
      if (rows == 1) {
        double x = data[0];
        double y = data[j];
        cairo_arc(cr, L->ax*x + L->bx, L->ay*y + L->by, 6, 0, 2*M_PI);
        cairo_close_path(cr);
        cairo_fill(cr);
      } else {
        cairo_set_line_width(cr, 6);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        for (i=0; i<rows; ++i) {
          double x = data[i*cols];
          double y = data[i*cols+j];
          double wx = L->ax*x + L->bx;
          double wy = L->ay*y + L->by;
          if (i == 0) {
            cairo_move_to(cr, wx, wy);
          } else {
            cairo_line_to(cr, wx, wy);
          };
        }
        cairo_stroke(cr);
      }

      int ci = (j-1)%100;
      cairo_set_source_rgb(cr, colors[ci].r, colors[ci].g, colors[ci].b);
      if (rows == 1) {
        double x = data[0];
        double y = data[j];
        cairo_arc(cr, L->ax*x + L->bx, L->ay*y + L->by, 4, 0, 2*M_PI);
        cairo_close_path(cr);
        cairo_fill(cr);
      } else {
        cairo_set_line_width(cr, 2);
        for (i=0; i<rows; ++i) {
          double x = data[i*cols];
          double y = data[i*cols+j];
          double wx = L->ax*x + L->bx;
          double wy = L->ay*y + L->by;
          if (i == 0) {
            cairo_move_to(cr, wx, wy);
          } else {
            cairo_line_to(cr, wx, wy);
          };
        }
        cairo_stroke(cr);
      }
    }
  }

 draw_message:
  /* state messages */
  if (state->message && is_screen) {
    cairo_select_font_face(cr, "sans-serif",
                           CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 24.0);

    cairo_text_extents_t te;
    cairo_text_extents(cr, state->message, &te);
    double xpos = xres/2.54 - te.x_bearing;
    double ypos = yres/2.54 - te.y_bearing;
    cairo_rectangle(cr, xpos+te.x_bearing-2, ypos+te.y_bearing-2,
                    te.width+4, te.height+4);
    cairo_set_source_rgba(cr, 1, 1, 1, .8);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, .8, 0, 0);
    cairo_move_to(cr, xpos, ypos);
    cairo_show_text(cr, state->message);
  }
}
