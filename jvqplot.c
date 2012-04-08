/* jvqplot.c - very simple data file plotting
 *
 * Copyright (C) 2010, 2012  Jochen Voss
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
#include <stdio.h>
#include <math.h>

#include <gtk/gtk.h>

#include "jvqplot.h"


#define _(str) str


static GtkWidget *window, *drawing_area;
static GtkPrintSettings *settings = NULL;


static void
print_page(GtkPrintOperation *operation, GtkPrintContext *ctx,
           gint page_nr, gpointer data)
{
  gdouble width = gtk_print_context_get_width(ctx);
  gdouble height = gtk_print_context_get_height(ctx);
  gdouble xres = gtk_print_context_get_dpi_x(ctx);
  gdouble yres = gtk_print_context_get_dpi_y(ctx);
  double x0 = state->min[0];
  double x1 = state->max[0];
  double y0 = state->min[1];
  double y1 = state->max[1];

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
  if (! state->dataset_used)  return;

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

  if (state->dataset_used && L) {
    double x0 = L->ax*state->min[0]+L->bx;
    double y0 = L->ay*state->min[1]+L->by;
    double x1 = L->ax*state->max[0]+L->bx;
    double y1 = L->ay*state->max[1]+L->by;
    if (L->width != width || L->height != height
        || x0 < 0 || x0 > width || y0 < 0 || y0 > height
        || x1 < 0 || x1 > width || y1 < 0 || y1 > height
        || x1-x0 < .6*width || y0-y1 < .2*height) {
      delete_layout(L);
      L = NULL;
    }
  }
  if (state->dataset_used && !L) {
    L = new_layout(width, height, xres, yres,
                   state->min[0], state->max[0],
                   state->min[1], state->max[1]);
  }
  draw_graph(cr, L, TRUE);

  cairo_destroy(cr);
  return TRUE;
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
    read_data(NULL);
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
  gtk_show_uri(NULL, "http://www.seehuhn.de/pages/jvqplot",
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

  gboolean version_flag = FALSE;
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
    puts("Copyright(C) 2010, 2012 Jochen Voss <voss@seehuhn.de>");
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
