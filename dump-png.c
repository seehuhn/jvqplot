/* dump-png.c - dump the jvqplot output into a png file (for the web page)
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

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <cairo.h>

#include "jvqplot.h"


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
    gui = gtk_init_with_args(&argc, &argv, "width height datafile outfile.png", entries, NULL, &err);
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
        puts("dump-png " VERSION);
        puts("Copyright(C) 2012 Jochen Voss <voss@seehuhn.de>");
        puts("License GPLv3+: GNU GPL version 3 or later "
             "<http://gnu.org/licenses/gpl.html>");

        puts("This is free software: you are free to change "
             "and redistribute it.");
        puts("There is NO WARRANTY, to the extent permitted by law.");
        exit(0);
    }
    if (argc<5) {
        fprintf(stderr, "error: no data file given\n");
        exit(1);
    } else if (argc>5) {
        fprintf(stderr, "error: too many arguments\n");
        exit(1);
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    const char *infile = argv[3];
    const char *outfile = argv[4];

    /* read the data */
    GFile *in;
    in = g_file_new_for_commandline_arg(infile);
    read_data(in);
    g_object_unref(in);

    struct layout *L;
    L = new_layout(width, height, 96, 96,
                   state->min[0], state->max[0],
                   state->min[1], state->max[1]);

    /* generate the PNG plot */
    cairo_surface_t *surface;
    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);

    cairo_t *cr;
    cr = cairo_create(surface);
    draw_graph(cr, L, FALSE);
    cairo_destroy(cr);

    cairo_status_t rc;
    rc = cairo_surface_write_to_png(surface, outfile);
    if (rc != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "error: cannot write \"%s\" (%s)\n",
                outfile, cairo_status_to_string(rc));
        exit(1);
    }

    delete_layout(L);
    cairo_surface_destroy(surface);
    return 0;
}
