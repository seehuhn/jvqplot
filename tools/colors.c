#include <stdio.h>
#include <gsl/gsl_qrng.h>

int
main (void)
{
  int i;
  gsl_qrng *q = gsl_qrng_alloc (gsl_qrng_sobol, 3);

  puts("static struct {");
  puts("  double r, g, b;");
  puts("} colors[100] = {");
  for (i=0;;) {
    double v[3];
    gsl_qrng_get (q, v);
    if (.3*v[0] + .59*v[1] + .11*v[2] > .5)
      continue;
    printf ("  { %f, %f, %f},\n", v[0], v[1], v[2]);
    ++i;
    if (i >= 100) break;
  }
  puts("};");

  gsl_qrng_free (q);
  return 0;
}
