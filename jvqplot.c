/* jvqplot.c - very simple data file plotting
 *
 * Copyright (C) 2010  Jochen Voss
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
#  include <config.h>
#endif

#include <stdlib.h>
#include <math.h>

#include <gtk/gtk.h>


#define _(str) str


static GtkWidget *window, *drawing_area;

/**********************************************************************
 * store the data values
 */

struct status {
  double *data;
  int rows, cols;
  double min[2], max[2];
  gchar *message;
};
struct status status_rec = {
  .data = NULL,
  .message = NULL,
};
struct status *status = &status_rec;


static void
update_message(const gchar *message)
{
  g_free(status->message);
  if (message && *message) {
    status->message = g_strdup(message);
  } else {
    status->message = NULL;
  }
}

static void
update_data(double *data, int rows, int cols)
{
  int  i, j;

  g_free(status->data);
  status->data = data;
  if (! data) return;

  for (j=0; j<2; ++j) {
    status->min[j] = status->max[j] = data[j];
  }
  for (i=0; i<rows; ++i) {
    for (j=0; j<cols; ++j) {
      double x = data[i*cols+j];
      int jj = j>1 ? 1 : j;
      if (x < status->min[jj]) status->min[jj] = x;
      if (x > status->max[jj]) status->max[jj] = x;
    }
  }

  for (j=0; j<2; ++j) {
    if (status->min[j] == status->max[j]) {
      status->min[j] -= 1;
      status->max[j] += 1;
    }
    if (status->min[j] > 0 && 2*status->min[j] <= status->max[j]) {
      status->min[j] = 0;
    }
    if (status->max[j] < 0 && 2*status->max[j] >= status->min[j]) {
      status->max[j] = 0;
    }
  }

  status->rows = rows;
  status->cols = cols;

  update_message(NULL);
}

/**********************************************************************
 * graph layout handling
 */

static double xres = -1, yres;

static double
log_mod(double x, double *base)
/* This function returns `x', scaled by a signed power of ten
 * (`base'), such that the result fullfills `1 <= result < 10' and
 * `x=result*base'.  */
{
  double  s, tmp, y;

  g_assert(x != 0);
  s = (x>0) ? 1 : -1;

  tmp = floor(log10(s*x));
  *base = s*pow(10, tmp);
  y = x / *base;
  g_assert(1<=y && y<10);

  return  y;
}

static double
stepsize(double w, int *mult_ret)
{
  double  base, frac, size;

  g_assert(w>0);

  frac = log_mod(2*w, &base);
  if (frac < 2) {
    frac = 1;
    *mult_ret = 5;
  } else if (frac < 2.5) {
    frac = 2;
    *mult_ret = 5;
  } else if (frac < 5) {
    frac = 2.5;
    *mult_ret = 4;
  } else {
    frac = 5;
    *mult_ret = 5;
  }
  size = frac * base;

  g_assert(w < size && size <= 2*w);
  return  size;
}

static void
normalize(double a, double b, double d, double *aa, double *bb)
/* Input is an interval `[a;b]' and a tick distance `d'.  The function
 * computes an interval `[aa,bb]' which contains `[a,b]' such that
 * `aa' and `bb' are integer multiples of `dd'.  */
{
  double  tmpa, tmpb;

  g_assert(b>a && d>0);

  tmpa = ((int)(a/d)-1)*d;
  tmpb = ((int)(b/d)+1)*d;
  *aa = floor(a/d) * d;
  *bb = ceil(b/d) * d;

  g_assert(*aa<=a && *bb>=b);
}

struct layout {
  int width, height;
  double ax, ay, bx, by;
  double dx, dy;
  int xmult, ymult;
};

struct layout *
new_layout(int w_pix, int h_pix, double xres, double yres,
	   double xmin, double xmax, double ymin, double ymax)
{
  /* physical layout dimensions */
  double w_phys = w_pix/xres;
  double h_phys = h_pix/yres;
  double tgap_phys = 7/72.27;
  double rgap_phys = 7/72.27;
  double bgap_phys = (h_phys>1) ? 20/72.27 : 7/72.27;
  double lgap_phys = (w_phys>1.5) ? .5 : 7/72.27;

  /* grid spacing */
  double n_xlab = (w_phys-lgap_phys-rgap_phys)*2.54; /* <= 1 grid line / cm */
  double n_ylab = (h_phys-tgap_phys-bgap_phys)*2.54;
  double x0, x1, dx, y0, y1, dy, xscale, yscale, dz, scale;
  int xmult, ymult;
  dx = stepsize((xmax-xmin)/n_xlab, &xmult);
  dy = stepsize((ymax-ymin)/n_ylab, &ymult);

  /* check whether 1:1 aspect ratio is acceptable */
  dz = MAX(dx, dy);
  normalize(xmin, xmax, dz, &x0, &x1);
  normalize(ymin, ymax, dz, &y0, &y1);
  xscale = (w_phys-lgap_phys-rgap_phys) / (x1-x0);
  yscale = (h_phys-tgap_phys-bgap_phys) / (y1-y0);
  scale = MIN(xscale, yscale);
  if (scale*(xmax-xmin) >= .2*w_phys && scale*(ymax-ymin) >= .1*h_phys) {
    /* yes, we can use 1:1 */
    if (dx >= dy) {
      dy = dx;
      ymult = xmult;
    } else {
      dx = dy;
      xmult = ymult;
    }
    xscale = yscale = scale;
  } else {
    /* no, use independent scales */
    normalize(xmin, xmax, dx, &x0, &x1);
    normalize(ymin, ymax, dy, &y0, &y1);
    xscale = (w_phys-lgap_phys-rgap_phys) / (x1-x0);
    yscale = (h_phys-tgap_phys-bgap_phys) / (y1-y0);
  }

  double xpos = (w_phys-lgap_phys-rgap_phys-xscale*(x1-x0))/2 + lgap_phys;
  double ypos = (h_phys-tgap_phys-bgap_phys-yscale*(y1-y0))/2 + bgap_phys;

  struct layout *L = g_new(struct layout, 1);
  L->width = w_pix;
  L->height = h_pix;
  L->ax = xscale*xres;
  L->bx = (xpos - x0*xscale)*xres;
  L->dx = dx;
  L->xmult = xmult;
  L->ay = -yscale*yres;
  L->by = h_pix - (ypos - y0*yscale)*yres;
  L->dy = dy;
  L->ymult = ymult;

  return L;
}

static void
delete_layout(struct layout *L)
{
  g_free(L);
}

/**********************************************************************
 * draw the graph
 */

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

static void
draw_graph(cairo_t *cr, struct layout *L, gboolean is_screen)
{
  int i, j;

  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_paint(cr);

  if (! status->data)
    goto draw_message;

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

  for (j=1; j<status->cols; ++j) {
    cairo_set_line_width(cr, 6);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgba(cr, 1, 1, 1, .5);
    for (i=0; i<status->rows; ++i) {
      double x = status->data[i*status->cols];
      double y = status->data[i*status->cols+j];
      double wx = L->ax*x + L->bx;
      double wy = L->ay*y + L->by;
      if (i == 0) {
	cairo_move_to(cr, wx, wy);
      } else {
	cairo_line_to(cr, wx, wy);
      };
    }
    cairo_stroke(cr);

    int ci = (j-1)%100;
    cairo_set_line_width(cr, 2);
    cairo_set_source_rgb(cr, colors[ci].r, colors[ci].g, colors[ci].b);
    for (i=0; i<status->rows; ++i) {
      double x = status->data[i*status->cols];
      double y = status->data[i*status->cols+j];
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

 draw_message:
  if (status->message && is_screen) {
    cairo_select_font_face(cr, "sans-serif",
			   CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 24.0);

    cairo_text_extents_t te;
    cairo_text_extents(cr, status->message, &te);
    double xpos = xres/2.54 - te.x_bearing;
    double ypos = yres/2.54 - te.y_bearing;
    cairo_rectangle(cr, xpos+te.x_bearing-2, ypos+te.y_bearing-2,
		    te.width+4, te.height+4);
    cairo_set_source_rgba(cr, 1, 1, 1, .8);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, .8, 0, 0);
    cairo_move_to(cr, xpos, ypos);
    cairo_show_text(cr, status->message);
  }
}

/**********************************************************************
 * printing
 */

static GtkPrintSettings *settings = NULL;

static void
print_page(GtkPrintOperation *operation, GtkPrintContext *ctx,
	   gint page_nr, gpointer data)
{
  gdouble width = gtk_print_context_get_width(ctx);
  gdouble height = gtk_print_context_get_height(ctx);
  gdouble xres = gtk_print_context_get_dpi_x(ctx);
  gdouble yres = gtk_print_context_get_dpi_y(ctx);
  double x0 = status->min[0];
  double x1 = status->max[0];
  double y0 = status->min[1];
  double y1 = status->max[1];

  cairo_t *cr = gtk_print_context_get_cairo_context(ctx);

  if ((width < height && x1-x0 > y1-y0)
      || (width > height && x1-x0 < y1-y0)) {
    /* rotate page */
    cairo_translate(cr, width, 0);
    cairo_rotate(cr, M_PI/2);

    gdouble tmp;
    tmp = width, width = height, height = tmp;
    tmp = xres, xres = yres, yres = tmp;
  }

  struct layout *L = new_layout(width, height, xres, yres, x0, x1, y0, y1);
  draw_graph(cr, L, FALSE);
  delete_layout(L);
}

static void
print_action(GtkAction *action, gpointer data)
{
  if (! status->data)  return;

  GtkPrintOperation *print = gtk_print_operation_new();
  if (settings) gtk_print_operation_set_print_settings(print, settings);
  gtk_print_operation_set_n_pages(print, 1);
  g_signal_connect(print, "draw_page", G_CALLBACK(print_page), NULL);

  GtkPrintOperationResult res;
  res = gtk_print_operation_run(print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
				GTK_WINDOW(window), NULL);
  if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
    if (settings) g_object_unref(settings);
    settings = g_object_ref(gtk_print_operation_get_print_settings(print));
  }

  g_object_unref(print);
}

/**********************************************************************
 * GTK+ related functions
 */

#define JVQPLOT_ERROR jvqplot_error_quark ()
static GQuark
jvqplot_error_quark (void)
{
  return g_quark_from_static_string ("jvqplot-error-quark");
}
#define JVQPLOT_ERROR_INPUT_DATA 1


static double *
parse_data_file(GDataInputStream *in,
		int *rows_ret, int *cols_ret, GError **err_ret)
{
  GError *err = NULL;
  int result_used = 0;
  int result_allocated = 256;
  double *result = g_new(double, result_allocated);

  gchar *line;
  int rows = 0;
  int cols = 0;

  while ((line = g_data_input_stream_read_line(in, NULL, NULL, &err))) {
    gchar **words = g_strsplit_set(line, " \t", 0);
    if (! words[0] || words[0][0] == '#') goto next_line;

    int n = 0;
    while (words[n]) ++n;
    if (cols == 0) {
      cols = n;
    } else if (cols != n) {
      g_set_error(&err, JVQPLOT_ERROR, JVQPLOT_ERROR_INPUT_DATA,
		  "invalid data (malformed matrix)");
      goto next_line;
    }

    if (cols == 1) {
      if (result_used >= result_allocated) {
	result_allocated *= 2;
	result = g_renew(double, result, result_allocated);
      }
      result[result_used++] = rows+1;
    }

    int i;
    for (i=0; i<cols; ++i) {
      char *endptr;
      double x = strtod(words[i], &endptr);
      if (*endptr) {
	g_set_error(&err, JVQPLOT_ERROR, JVQPLOT_ERROR_INPUT_DATA,
		    "invalid data (malformed number)");
	goto next_line;
      }
      if (result_used >= result_allocated) {
	result_allocated *= 2;
	result = g_renew(double, result, result_allocated);
      }
      result[result_used++] = x;
    }
    rows += 1;

  next_line:
    g_strfreev(words);
    g_free(line);
    if (err) break;
  }

  if (!err && result_used == 0) {
    g_set_error(&err, JVQPLOT_ERROR, JVQPLOT_ERROR_INPUT_DATA,
		"no data found");
  }

  if (err) {
    g_propagate_error(err_ret, err);
    g_free(result);
    return NULL;
  }

  result = g_renew(double, result, result_used);
  *rows_ret = rows;
  *cols_ret = (cols==1) ? 2 : cols;
  return result;
}

static gboolean
expose_event_callback(GtkWidget *widget, GdkEventExpose *event,
		      gpointer data)
{
  int width = widget->allocation.width;
  int height = widget->allocation.height;

  if (xres < 0) {
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
    xres = gdk_screen_get_width(screen)/gdk_screen_get_width_mm(screen)*25.4;
    yres = gdk_screen_get_height(screen)/gdk_screen_get_height_mm(screen)*25.4;
    if (xres < 50 || xres > 350 || yres < 50 || yres > 350) {
      xres = yres = 100;
    }
  }

  cairo_t *cr = gdk_cairo_create(event->window);
  cairo_rectangle(cr, event->area.x, event->area.y,
		  event->area.width, event->area.height);
  cairo_clip(cr);

  static struct layout *L = NULL;

  if (status->data && L) {
    double x0 = L->ax*status->min[0]+L->bx;
    double y0 = L->ay*status->min[1]+L->by;
    double x1 = L->ax*status->max[0]+L->bx;
    double y1 = L->ay*status->max[1]+L->by;
    if (L->width != width || L->height != height
	|| x0 < 0 || x0 > width || y0 < 0 || y0 > height
	|| x1 < 0 || x1 > width || y1 < 0 || y1 > height
	|| x1-x0 < .2*width || y0-y1 < .1*height) {
      delete_layout(L);
      L = NULL;
    }
  }
  if (status->data && !L) {
    L = new_layout(width, height, xres, yres,
		   status->min[0], status->max[0],
		   status->min[1], status->max[1]);
  }
  draw_graph(cr, L, TRUE);

  cairo_destroy(cr);
  return TRUE;
}

static void
read_data(GFile *file)
{
  GError *err = NULL;
  double *data;
  int  rows, cols;

  GFileInputStream *in = g_file_read(file, NULL, &err);
  if (err) {
    update_message(err->message);
    g_clear_error(&err);
    return;
  }
  GDataInputStream *inn = g_data_input_stream_new(G_INPUT_STREAM(in));
  g_object_unref(in);

  data = parse_data_file(inn, &rows, &cols, &err);
  g_input_stream_close(G_INPUT_STREAM(inn), NULL, NULL);
  g_object_unref(inn);

  if (err) {
    g_free(data);
    update_message(err->message);
    g_clear_error(&err);
  } else {
    update_data(data, rows, cols);
  }
}

static void
data_changed_cb(GFileMonitor *monitor, GFile *file, GFile *other_file,
		GFileMonitorEvent event_type, gpointer data)
{
  switch (event_type) {
  case G_FILE_MONITOR_EVENT_CHANGED:
  case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
  case G_FILE_MONITOR_EVENT_CREATED:
    read_data(file);
    break;
  case G_FILE_MONITOR_EVENT_DELETED:
    update_message("data file removed");
    break;
  default:
    break;
  }
  gtk_widget_queue_draw_area(drawing_area,
			     0, 0,
			     drawing_area->allocation.width,
			     drawing_area->allocation.height);
}

static void
quit_cb(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

static gboolean
popup_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  GtkMenu *popup_menu = GTK_MENU(data);
  if (event->button != 3)
    return FALSE;
  gtk_menu_popup(popup_menu, NULL, NULL, NULL, NULL,
		 event->button, event->time);
  return TRUE;
}

static void
home_action(GtkAction *action, gpointer data)
{
  gtk_show_uri(NULL, "http://seehuhn.de/pages/jvqplot",
	       GDK_CURRENT_TIME, NULL);
}

static const gchar *menu_def =
  "<ui>"
  "  <popup name=\"MainMenu\">"
  "    <menuitem name=\"Home\" action=\"HomeAction\" />"
  "    <menuitem name=\"Print\" action=\"PrintAction\" />"
  "    <menuitem name=\"Quit\" action=\"QuitAction\" />"
  "  </popup>"
  "</ui>";

static void
define_menu(void)
{
  GError *err = NULL;

  static GtkActionEntry entries[] = {
    /* name, stock id, label, accelerator, tooltip, callback */
    { "HomeAction", NULL, _("Visit _Home Page"), NULL,
      _("open the jvqplot homepage in a web browser"),
      G_CALLBACK(home_action) },
    { "PrintAction", GTK_STOCK_PRINT, _("_Print"), "<control>P",
      _("print the current graph"), G_CALLBACK(print_action) },
    { "QuitAction", GTK_STOCK_QUIT, _("_Quit"), "<control>Q",
      _("quit the program"), G_CALLBACK(quit_cb) }
  };
  static guint n_entries = G_N_ELEMENTS (entries);

  GtkActionGroup *action_group = gtk_action_group_new("jvqplot");
  gtk_action_group_add_actions(action_group, entries, n_entries, NULL);

  GtkUIManager *menu_manager = gtk_ui_manager_new();
  gtk_ui_manager_insert_action_group (menu_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string(menu_manager, menu_def, -1, &err);
  if (err) {
    fprintf(stderr, "%s\n", err->message);
    g_clear_error(&err);
    exit(1);
  }
  gtk_window_add_accel_group(GTK_WINDOW(window),
			     gtk_ui_manager_get_accel_group(menu_manager));

  GtkWidget *popup_menu = gtk_ui_manager_get_widget(menu_manager, "/MainMenu");
  gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(G_OBJECT(window), "button_press_event",
		   G_CALLBACK(popup_cb), popup_menu);
}

int
main(int argc, char **argv)
{
  GError *err = NULL;
  gboolean gui;

  gboolean version_flag;
  GOptionEntry entries[] = {
    { "version", 'v', 0, G_OPTION_ARG_NONE, &version_flag,
      "Show version information", NULL },
    { NULL, '\0', 0, 0, NULL, NULL, NULL }
  };
  gui = gtk_init_with_args(&argc, &argv, "datafile", entries, NULL, &err);
  if (err) {
    fprintf(stderr, "%s\n", err->message);
    g_clear_error(&err);
    exit(1);
  }
  if (! gui) {
    fprintf(stderr, "cannot initialise GTK+\n");
    exit(1);
  }
  if (version_flag) {
    puts("jvqplot " VERSION);
    puts("Copyright(C) 2010 Jochen Voss <voss@seehuhn.de>");
    puts("License GPLv3+: GNU GPL version 3 or later "
	 "<http://gnu.org/licenses/gpl.html>");

    puts("This is free software: you are free to change "
	 "and redistribute it.");
    puts("There is NO WARRANTY, to the extent permitted by law.");
    exit(0);
  }
  if (argc<2) {
    fprintf(stderr, "error: no data file given\n");
    exit(1);
  } else if (argc>2) {
    fprintf(stderr, "error: too many arguments\n");
    exit(1);
  }

  GFile *data_file = g_file_new_for_commandline_arg(argv[1]);
  read_data(data_file);
  GFileMonitor *monitor = g_file_monitor(data_file, 0, NULL, &err);
  g_object_unref(data_file);
  if (err) {
    fprintf(stderr, "error: cannot monitor file: %s\n", err->message);
    g_clear_error(&err);
    exit(1);
  }
  g_signal_connect(monitor, "changed", G_CALLBACK(data_changed_cb), NULL);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
  gchar *window_title = g_strconcat("jvqplot: ", argv[1], NULL);
  gtk_window_set_title(GTK_WINDOW(window), window_title);
  g_free(window_title);
  g_signal_connect(window, "destroy", G_CALLBACK(quit_cb), NULL);

  drawing_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area, 100, 100);
  g_signal_connect(G_OBJECT(drawing_area), "expose_event",
		   G_CALLBACK(expose_event_callback), NULL);
  gtk_container_add(GTK_CONTAINER(window), drawing_area);

  define_menu();

  gtk_widget_show_all(window);
  gtk_main();

  return 0;
}
