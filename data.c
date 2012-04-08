/* data.c - load and store the data values
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

#include <glib.h>
#include <gio/gio.h>

#include "jvqplot.h"


#define JVQPLOT_ERROR jvqplot_error_quark ()
static GQuark
jvqplot_error_quark (void)
{
  return g_quark_from_static_string ("jvqplot-error-quark");
}
#define JVQPLOT_ERROR_CORRUPTED 1
#define JVQPLOT_ERROR_INCOMPLETE 2


static struct state state_rec = {
  .dataset_used = 0,
  .dataset = NULL,
  .message = NULL,
};
struct state *state = &state_rec;


static void
update_message(const gchar *message)
{
  g_free(state->message);
  if (message && *message) {
    state->message = g_strdup(message);
  } else {
    state->message = NULL;
  }
}

static void
update_data(int dataset_used, struct dataset *dataset)
{
  int  i, j, k;

  if (dataset_used == 0) return;

  for (k=0; k<state->dataset_used; ++k) g_free(state->dataset[k].data);
  g_free(state->dataset);
  state->dataset_used = dataset_used;
  state->dataset = dataset;

  for (j=0; j<2; ++j) {
    state->min[j] = state->max[j] = dataset[0].data[j];
  }
  for (k=0; k<dataset_used; ++k) {
    int rows = dataset[k].rows;
    int cols = dataset[k].cols;
    double *data = dataset[k].data;
    for (i=0; i<rows; ++i) {
      for (j=0; j<cols; ++j) {
        double x = data[i*cols+j];
        int jj = j>1 ? 1 : j;
        if (x < state->min[jj]) state->min[jj] = x;
        if (x > state->max[jj]) state->max[jj] = x;
      }
    }
  }

  for (j=0; j<2; ++j) {
    if (state->min[j] == state->max[j]) {
      state->min[j] -= 1;
      state->max[j] += 1;
    }
    if (state->min[j] > 0 && 4*state->min[j] <= state->max[j]) {
      /* at most extend the horizontal range by anoth 33.3% */
      state->min[j] = 0;
    }
    if (state->max[j] < 0 && 2*state->max[j] >= state->min[j]) {
      /* at most double the vertical range */
      state->max[j] = 0;
    }
  }

  update_message(NULL);
}

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
    gboolean is_empty = ! words[0];
    if (is_empty || words[0][0] == '#') goto next_line;

    int n = 0;
    while (words[n]) ++n;
    if (cols == 0) {
      cols = n;
    } else if (n < cols
               && g_buffered_input_stream_get_available(G_BUFFERED_INPUT_STREAM(in)) == 0) {
      g_set_error(&err, JVQPLOT_ERROR, JVQPLOT_ERROR_INCOMPLETE,
                  "incomplete input");
      goto next_line;
    } else if (cols != n) {
      g_set_error(&err, JVQPLOT_ERROR, JVQPLOT_ERROR_CORRUPTED,
                  "invalid data (malformed matrix)");
      goto next_line;
    }

    /* if there is only one column, prepend the index */
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
        g_set_error(&err, JVQPLOT_ERROR, JVQPLOT_ERROR_CORRUPTED,
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
    if (is_empty && rows > 0) break;
  }

  if (err) {
    g_propagate_error(err_ret, err);
    if (g_error_matches(err, JVQPLOT_ERROR, JVQPLOT_ERROR_CORRUPTED)) {
      g_free(result);
      return NULL;
    }
  }

  result = g_renew(double, result, result_used);
  *rows_ret = rows;
  *cols_ret = (cols==1) ? 2 : cols;
  return result;
}

void
read_data(GFile *file)
{
  GError *err = NULL;
  int  rows, cols;

  if (! file) {
    update_message("data file removed");
    return;
  }

  GFileInputStream *in = g_file_read(file, NULL, &err);
  if (err) {
    update_message(err->message);
    g_clear_error(&err);
    return;
  }
  GDataInputStream *inn = g_data_input_stream_new(G_INPUT_STREAM(in));
  g_object_unref(in);

  int dataset_used = 0;
  int dataset_allocated = 4;
  struct dataset *dataset = g_new(struct dataset, dataset_allocated);
  double *data;
  while ((data = parse_data_file(inn, &rows, &cols, &err))) {
    if (dataset_used >= dataset_allocated) {
      dataset_allocated *= 2;
      dataset = g_renew(struct dataset, dataset, dataset_allocated);
    }
    dataset[dataset_used].data = data;
    dataset[dataset_used].rows = rows;
    dataset[dataset_used].cols = cols;
    dataset_used += 1;
    if (err) break;
  }
  if (dataset_used == 0) {
    g_set_error(&err, JVQPLOT_ERROR, JVQPLOT_ERROR_CORRUPTED,
                "no data found");
  }

  g_input_stream_close(G_INPUT_STREAM(inn), NULL, NULL);
  g_object_unref(inn);

  update_data(dataset_used, dataset);
  if (err) {
    update_message(err->message);
    g_clear_error(&err);
  }
}
