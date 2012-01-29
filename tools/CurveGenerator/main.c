/*
Copyright (c) 2011-2012 Hiroshi Tsubokawa
See LICENSE and README
*/

#include "CurveIO.h"
#include "MeshIO.h"
#include "Mesh.h"
#include "Vector.h"
#include "Array.h"
#include "Triangle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static const char USAGE[] =
"Usage: gencurve [options] inputfile(*.mesh) outputfile(*.crv)\n"
"Options:\n"
"  --help         Display this information\n"
"\n";

int main(int argc, const char **argv)
{
	struct CurveOutput *out;
	struct Mesh *mesh;
	const char *meshfile;
	const char *curvefile;

	double *CPs;
	double *width;
	int *indices;
	int *ncurves_on_face;

	double *sourceP;
	double *sourceN;

	int total_ncurves;
	int total_ncps;

	int curve_id;
	int cp_id;
	int i;

	if (argc == 2 && strcmp(argv[1], "--help") == 0) {
		printf(USAGE);
		return 0;
	}

	if (argc != 3) {
		fprintf(stderr, "error: invalid number of arguments.\n");
		fprintf(stderr, USAGE);
		return -1;
	}
	meshfile = argv[1];
	curvefile = argv[2];

	mesh = MshNew();
	if (mesh == NULL) {
		fprintf(stderr, "fatal error: MshNew returned NULL.\n");
		return -1;
	}

	if (MshLoadFile(mesh, meshfile)) {
		MshFree(mesh);
		fprintf(stderr, "error: %s: %s\n", MshGetErrorMessage(MshGetErrorNo()), meshfile);
		return -1;
	}

	printf("mesh->nfaces: %d\n", mesh->nfaces);

	/* count total_ncurves */
	ncurves_on_face = (int *) malloc(sizeof(int) * mesh->nfaces);
	total_ncurves = 0;
	for (i = 0; i < mesh->nfaces; i++) {
		const double *v0, *v1, *v2;
		double area;

		MshGetFaceVertex(mesh, i, &v0, &v1, &v2);
		area = TriComputeArea(v0, v1, v2);

		ncurves_on_face[i] = 100000 * area;
		total_ncurves += ncurves_on_face[i];
	}
	printf("total_ncurves: %d\n", total_ncurves);

	total_ncps = 4 * total_ncurves;
	CPs = VEC3_ALLOC(double, total_ncps);
	width = (double *) malloc(sizeof(double) * total_ncps);
	indices = (int *) malloc(sizeof(int) * total_ncurves);

	sourceP = VEC3_ALLOC(double, total_ncurves);
	sourceN = VEC3_ALLOC(double, total_ncurves);

	curve_id = 0;
	for (i = 0; i < mesh->nfaces; i++) {
		int j;
		int i0, i1, i2;
		const double *P0, *P1, *P2;
		const double *N0, *N1, *N2;

		i0 = mesh->indices[3*i+0];
		i1 = mesh->indices[3*i+1];
		i2 = mesh->indices[3*i+2];

		N0 = &mesh->N[3*i0];
		N1 = &mesh->N[3*i1];
		N2 = &mesh->N[3*i2];

		P0 = &mesh->P[3*i0];
		P1 = &mesh->P[3*i1];
		P2 = &mesh->P[3*i2];

		for (j = 0; j < ncurves_on_face[i]; j++) {
			double gravity;
			double u, v, t;
			double *src_P;
			double *src_N;

			srand(12.34*i + 1232*j);
			u = (((double) rand()) / RAND_MAX);
			srand(21.43*i + 213*j);
			v = (1-u) * (((double) rand()) / RAND_MAX);

			src_P = VEC3_NTH(sourceP, curve_id);
			src_N = VEC3_NTH(sourceN, curve_id);

			t = 1-u-v;
			src_P[0] = t * P0[0] + u * P1[0] + v * P2[0];
			src_P[1] = t * P0[1] + u * P1[1] + v * P2[1];
			src_P[2] = t * P0[2] + u * P1[2] + v * P2[2];

			src_N[0] = t * N0[0] + u * N1[0] + v * N2[0];
			src_N[1] = t * N0[1] + u * N1[1] + v * N2[1];
			src_N[2] = t * N0[2] + u * N1[2] + v * N2[2];

			VEC3_NORMALIZE(src_N);

			srand(i+j);
			gravity = .5 + .5 * (((double) rand()) / RAND_MAX);
			src_N[1] -= gravity;
			VEC3_NORMALIZE(src_N);

			curve_id++;
		}
	}
	assert(curve_id == total_ncurves);

	cp_id = 0;
	curve_id = 0;
	for (i = 0; i < total_ncurves; i++) {
		int vtx;

		for (vtx = 0; vtx < 4; vtx++) {
			const double LENGTH = .02;
			double *CP;
			double *src_P;
			double *src_N;
			double noisevec[3] = {0};
			double noiseamp;

			srand(12*i + 49*vtx);
			if (vtx > 0) {
				noisevec[0] = (((double) rand()) / RAND_MAX);
				noisevec[1] = (((double) rand()) / RAND_MAX);
				noisevec[2] = (((double) rand()) / RAND_MAX);
			}
			noiseamp = .75 * LENGTH;

			CP = VEC3_NTH(CPs, cp_id);
			src_P = VEC3_NTH(sourceP, curve_id);
			src_N = VEC3_NTH(sourceN, curve_id);

			CP[0] = src_P[0] + noiseamp * noisevec[0] + vtx * LENGTH/3. * src_N[0];
			CP[1] = src_P[1] + noiseamp * noisevec[1] + vtx * LENGTH/3. * src_N[1];
			CP[2] = src_P[2] + noiseamp * noisevec[2] + vtx * LENGTH/3. * src_N[2];

			if (vtx == 0) {
				double *w = &width[cp_id];
				w[0] = .003;
				w[1] = .002;
				w[2] = .001;
				w[3] = .0001;
			}
			cp_id++;
		}
		indices[curve_id] = 4*i;
		curve_id++;
	}
	assert(cp_id == total_ncps);

	out = CrvOpenOutputFile(curvefile);
	if (out == NULL) {
		MshFree(mesh);
		fprintf(stderr, "error: %s: %s\n", CrvGetErrorMessage(CrvGetErrorNo()), curvefile);
		return -1;
	}

	/* setup CurveOutput */
	out->nverts = total_ncps;
	out->nvert_attrs = 2;
	out->P = CPs;
	out->width = width;
	out->uv = NULL;
	out->ncurves = total_ncurves;
	out->ncurve_attrs = 1;
	out->indices = indices;

	CrvWriteFile(out);

	/* clean up */
	CrvCloseOutputFile(out);
	MshFree(mesh);
	free(CPs);
	free(width);
	free(indices);
	free(ncurves_on_face);
	free(sourceP);
	free(sourceN);

	return 0;
}

