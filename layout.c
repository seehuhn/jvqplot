/* layout.c - plot layout handling
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

#include <math.h>

#include <glib.h>

#include "jvqplot.h"

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
  g_assert(b>a && d>0);

  *aa = floor(a/d) * d;
  *bb = ceil(b/d) * d;

  g_assert(*aa<=a && *bb>=b);
}

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
  if (scale*(xmax-xmin) >= .6*w_phys && scale*(ymax-ymin) >= .2*h_phys) {
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

void
delete_layout(struct layout *L)
{
  g_free(L);
}
