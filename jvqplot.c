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


static GtkWidget *window, *drawing_area;
static double xdpi = -1, ydpi;

/**********************************************************************
 * auxiliary functions
 */

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
  frac = log_mod(w, &base);
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

  g_assert(size <= w && 2*size > w);
  return  size;
}

static void
normalize(double a, double b, double d,
	  double *aa, double *bb, double *dd, int *mult_ret)
/* Input is an interval `[a;b]' and a minimal tick distance `d'.  The
 * function computes an interval `[aa,bb]' which contains `[a,b]' and
 * a tick distance `dd', such that `dd>=d' and `aa' and `bb' are integer
 * multiples of `dd'.  */
{
  double  unit, tmpa, tmpb;

  g_assert(b>a && d>0);

  unit = stepsize(2*d, mult_ret);
  tmpa = ((int)(a/unit)-1)*unit;
  tmpb = ((int)(b/unit)+1)*unit;
  *aa = floor(a/unit) * unit;
  *bb = ceil(b/unit) * unit;
  *dd = unit;

  g_assert(*aa<=a && *bb>=b && *dd>=d);
}

/**********************************************************************
 * store the data values
 */

struct status {
  double *data;
  double min[2], max[2];
  int rows, cols;
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
  cairo_t *cr = gdk_cairo_create(event->window);
  int width = widget->allocation.width;
  int height = widget->allocation.height;
  int i;

  if (xdpi < 0) {
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
    xdpi = gdk_screen_get_width(screen)/gdk_screen_get_width_mm(screen)*25.4;
    ydpi = gdk_screen_get_height(screen)/gdk_screen_get_height_mm(screen)*25.4;
    if (xdpi < 50 || xdpi > 300 || ydpi < 50 || ydpi > 300) {
      xdpi = ydpi = 100;
    }
  }

  cairo_rectangle(cr,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);
  cairo_clip(cr);
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_paint(cr);

  if (! status->data)
    goto draw_message;

  double lgap = .5;
  double bgap = 20/72.27;

  double  n_xlab = (width/xdpi-lgap-14/72.27)*2.54; /* <= 1 grid line / cm */
  double  n_ylab = (height/ydpi-bgap-14/72.27)*2.54;
  double  xmin, xmax, dx, ymin, ymax, dy;
  int  xmult, ymult;
  normalize(status->min[0], status->max[0],
	    (status->max[0]-status->min[0])/n_xlab,
	    &xmin, &xmax, &dx, &xmult);
  normalize(status->min[1], status->max[1],
	    (status->max[1]-status->min[1])/n_ylab,
	    &ymin, &ymax, &dy, &ymult);

  double xscale = (width/xdpi-lgap-14/72.27)/(xmax-xmin); /* inch / x-unit */
  double yscale = (height/ydpi-bgap-14/72.27)/(ymax-ymin); /* inch / y-unit */
  if (yscale > xscale && yscale < 10*xscale) yscale = xscale;
  if (xscale > yscale && xscale < 10*yscale) xscale = yscale;

  double xoffs = (width/xdpi-lgap - xscale*(xmax-xmin))/2+lgap; /* inch */
  double yoffs = (height/ydpi-bgap - yscale*(ymax-ymin))/2+bgap; /* inch */

  cairo_set_line_width(cr, 1);
  cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
  for (i=ceil((xmin-xoffs/xscale)/dx); ; ++i) {
    double wx = ((i*dx-xmin)*xscale+xoffs)*xdpi;
    if (wx > width) break;

    wx = floor(wx) + .5;
    cairo_move_to(cr, wx, 0);
    cairo_line_to(cr, wx, height);
  }
  for (i=ceil((ymin-yoffs/yscale)/dy); ; ++i) {
    double wy = height - ((i*dy-ymin)*yscale+yoffs)*ydpi;
    if (wy < 0) break;

    wy = floor(wy) + .5;
    cairo_move_to(cr, 0, wy);
    cairo_line_to(cr, width, wy);
  }
  cairo_stroke(cr);
  cairo_set_line_width(cr, 2);
  cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
  for (i=ceil((xmin-xoffs/xscale)/dx); ; ++i) {
    double wx = ((i*dx-xmin)*xscale+xoffs)*xdpi;
    if (wx > width) break;
    if (i % xmult) continue;

    wx = round(wx);
    cairo_move_to(cr, wx, 0);
    cairo_line_to(cr, wx, height);
  }
  for (i=ceil((ymin-yoffs/yscale)/dy); ; ++i) {
    double wy = height - ((i*dy-ymin)*yscale+yoffs)*ydpi;
    if (wy < 0) break;
    if (i % ymult) continue;

    wy = round(wy);
    cairo_move_to(cr, 0, wy);
    cairo_line_to(cr, width, wy);
  }
  cairo_stroke(cr);

  cairo_select_font_face(cr, "sans-serif",
			 CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 12.0);
  for (i=ceil((xmin-xoffs/xscale)/dx); ; ++i) {
    char buffer[32];
    double wx = ((i*dx-xmin)*xscale+xoffs)*xdpi;
    if (wx > width) break;
    if (i % xmult) continue;
    snprintf(buffer, 32, "%g", i*dx);

    cairo_text_extents_t te;
    cairo_text_extents(cr, buffer, &te);

    double xpos = wx - te.x_bearing - .5*te.width;
    cairo_rectangle(cr, xpos+te.x_bearing-2, height-8+te.y_bearing-2,
		    te.width+4, te.height+4);
    cairo_set_source_rgba(cr, 1, 1, 1, .8);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, xpos, height-8);
    cairo_show_text(cr, buffer);
  }
  for (i=ceil((ymin-yoffs/yscale)/dy); ; ++i) {
    char buffer[32];
    double wy = height - ((i*dy-ymin)*yscale+yoffs)*ydpi;
    if (wy < 0) break;
    if (i % ymult) continue;
    snprintf(buffer, 32, "%g", i*dy);

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

  cairo_set_line_width(cr, 6);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_source_rgba(cr, 1, 1, 1, .5);
  for (i=0; i<status->rows; ++i) {
    double x = status->data[i*status->cols];
    double y = status->data[i*status->cols+1];
    double wx = ((x-xmin)*xscale+xoffs)*xdpi;
    double wy = height - ((y-ymin)*yscale+yoffs)*ydpi;
    if (i == 0) {
      cairo_move_to(cr, wx, wy);
    } else {
      cairo_line_to(cr, wx, wy);
    };
  }
  cairo_stroke(cr);

  cairo_set_line_width(cr, 2);
  cairo_set_source_rgb(cr, 0, 0, .8);
  for (i=0; i<status->rows; ++i) {
    double x = status->data[i*status->cols];
    double y = status->data[i*status->cols+1];
    double wx = ((x-xmin)*xscale+xoffs)*xdpi;
    double wy = height - ((y-ymin)*yscale+yoffs)*ydpi;
    if (i == 0) {
      cairo_move_to(cr, wx, wy);
    } else {
      cairo_line_to(cr, wx, wy);
    };
  }
  cairo_stroke(cr);

 draw_message:
  if (status->message) {
    cairo_text_extents_t te;
    cairo_set_font_size(cr, 24.0);
    cairo_text_extents(cr, status->message, &te);

    double xpos = xdpi/2.54 - te.x_bearing;
    double ypos = ydpi/2.54 - te.y_bearing;
    cairo_rectangle(cr, xpos+te.x_bearing-2, ypos+te.y_bearing-2,
		    te.width+4, te.height+4);
    cairo_set_source_rgba(cr, 1, 1, 1, .8);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, .8, 0, 0);
    cairo_move_to(cr, xpos, ypos);
    cairo_show_text(cr, status->message);
  }

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
data_file_changed(GFileMonitor *monitor, GFile *file, GFile *other_file,
		  GFileMonitorEvent event_type, gpointer user_data)
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
destroy(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

int
main(int argc, char **argv)
{
  GError *err = NULL;
  gboolean gui;

  GOptionEntry entries[] = {
    { NULL, '\0', 0, 0, NULL, NULL, NULL }
  };
  gui = gtk_init_with_args(&argc, &argv,
			   "datafile",
			   entries,
			   NULL,
			   &err);
  if (err) {
    fprintf(stderr, "%s\n", err->message);
    g_clear_error(&err);
    return 1;
  }
  if (! gui) {
    fprintf(stderr, "cannot initialise GTK+\n");
    return 1;
  }

  if (argc<2) {
    fprintf(stderr, "error: no data file given\n");
    return 1;
  }
  GFile *data_file = g_file_new_for_commandline_arg(argv[1]);
  read_data(data_file);

  GFileMonitor *monitor = g_file_monitor(data_file, 0, NULL, &err);
  g_object_unref(data_file);
  if (err) {
    fprintf(stderr, "error: cannot monitor file: %s\n", err->message);
    g_clear_error(&err);
    return 1;
  }
  g_signal_connect(monitor, "changed", G_CALLBACK(data_file_changed), NULL);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
  gchar *window_title = g_strconcat("jvqplot: ", argv[1], NULL);
  gtk_window_set_title(GTK_WINDOW(window), window_title);
  g_free(window_title);
  g_signal_connect(window, "destroy", G_CALLBACK(destroy), NULL);

  drawing_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area, 100, 100);
  g_signal_connect(G_OBJECT(drawing_area), "expose_event",
		   G_CALLBACK(expose_event_callback), NULL);

  gtk_container_add(GTK_CONTAINER(window), drawing_area);

  gtk_widget_show_all(window);
  gtk_main();

  return 0;
}
