/* Patching test.

Takes: > wall.rsf
*/

#include <rsf.h>

#include "helix.h"
#include "helicon.h"
#include "tent.h"
#include "patching.h"

int main(int argc, char* argv[])
{
    int n[2], w[2], k[2], a[2], l[2], n12, w12, i;
    float *wall, *data, *windwt;
    filter aa;
    sf_file wind, out;

    sf_init (argc, argv);
    out = sf_output("out");

    sf_setformat(out,"native_float");

    if (!sf_getint("n1",&n[0])) n[0] = 100;
    if (!sf_getint("n2",&n[1])) n[1] = 30;

    sf_putint(out,"n1",n[0]);
    sf_putint(out,"n2",n[1]);

    if (!sf_getint("w1",&w[0])) w[0] = 17;
    if (!sf_getint("w2",&w[1])) w[1] = 6;

    if (!sf_getint("k1",&k[0])) k[0] = 5;
    if (!sf_getint("k2",&k[1])) k[1] = 11;

    if (!sf_getint("a1",&a[0])) a[0] = 1;
    if (!sf_getint("a2",&a[1])) a[1] = 1;

    if (!sf_getint("lag1",&l[0])) l[0] = 1;
    if (!sf_getint("lag2",&l[1])) l[1] = 1;

    n12 = n[0]*n[1];
    w12 = w[0]*w[1];

    wall = sf_floatalloc(n12);
    data = sf_floatalloc(n12);
    windwt = sf_floatalloc(w12);

    for (i=0; i < n12; i++) {
	wall[i] = 1.;
    }

    aa = allocatehelix (1);
    aa->lag[0] = 1;

    tent (2, w, l, a, windwt);

    if (NULL != sf_getstring("wind")) {
	/* optional output file for window weight */
	wind = sf_output("wind");
	sf_setformat(wind,"native_float");	
	sf_putint(wind,"n1",w[0]);
	sf_putint(wind,"n2",w[1]);

	sf_floatwrite (windwt,w12,wind);
	sf_fileclose(wind);
    }

    helicon_init (aa);
    patching (helicon_lop, wall, data, 2, k, n, w, windwt);

    for (i=0; i < n12; i+= n[0]) {
	data[i] = 0.;
    }
    for (i=1; i < n12; i+= n[0]) {
	data[i] = 0.;
    }
    for (i=n[0]-2; i < n12; i+= n[0]) {
	data[i] = 0.;
    }
    for (i=n[0]-1; i < n12; i+= n[0]) {
	data[i] = 0.;
    }

    sf_floatwrite(data,n12,out);

    exit (0);
}
