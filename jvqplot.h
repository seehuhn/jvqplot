/* jvqplot.h - common header file for jvqplot
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

#ifndef FILE_JVQPLOT_H_SEEN
#define FILE_JVQPLOT_H_SEEN

#include <cairo.h>              /* for 'cairo_t' */
#include <gio/gio.h>            /* for 'GFile' */


/* from "data.c" */
struct dataset {
  double *data;
  int rows, cols;
};
struct state {
  int dataset_used;
  struct dataset *dataset;
  double min[2], max[2];
  gchar *message;
};
extern struct state *state;
extern void read_data(GFile *file);


/* from "layout.c" */
struct layout {
  int width, height;
  double ax, ay, bx, by;
  double dx, dy;
  int xmult, ymult;
};
extern struct layout *new_layout(int w_pix, int h_pix,
                                 double xres, double yres,
                                 double xmin, double xmax,
                                 double ymin, double ymax);
extern void delete_layout(struct layout *L);


/* from "draw.c" */
extern double xres, yres;
extern void draw_graph(cairo_t *cr, struct layout *L, gboolean is_screen);


#endif /* FILE_JVQPLOT_H_SEEN */
