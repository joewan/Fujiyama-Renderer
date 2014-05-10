/*
Copyright (c) 2011-2014 Hiroshi Tsubokawa
See LICENSE and README
*/

#include "fj_noise.h"
#include "fj_vector.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

#define PERMUTAION \
151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69, \
142,8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252, \
219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168, \
68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211, \
133,230,220,105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,80, \
73,209,76,132,187,208,89,18,169,200,196,135,130,116,188,159,86,164,100, \
109,198,173,186,3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82, \
85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248, \
152,2,44,154,163,70,221,153,101,155,167,43,172,9,129,22,39,253,19,98,108, \
110,79,113,224,232,178,185,112,104,218,246,97,228,251,34,242,193,238,210, \
144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181, \
199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93, \
222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180

static const int perm[] = {
  PERMUTAION,
  /* repeat */
  PERMUTAION
};

static double fade(double t);
static double lerp(double t, double a, double b);
static double grad(int hash, double x, double y, double z); 

extern double PerlinNoise(const struct Vector *position,
    double lacunarity, double persistence, int octaves)
{
  struct Vector P;
  double noise_value = 0;
  double amp = 1;
  int i;

  P.x = position->x;
  P.y = position->y;
  P.z = position->z;

  for (i = 0; i < octaves; i++) {
    noise_value += amp * PeriodicNoise3d(P.x, P.y, P.z);

    amp *= persistence;
    P.x *= lacunarity;
    P.y *= lacunarity;
    P.z *= lacunarity;
  }

  return noise_value;
}

void PerlinNoise3d(const struct Vector *position,
    double lacunarity, double persistence, int octaves,
    struct Vector *P_out)
{
  struct Vector P;

  P.x = position->x;
  P.y = position->y;
  P.z = position->z;
  P_out->x = PerlinNoise(&P, lacunarity, persistence, octaves);

  P.x = position->x + 131.977;
  P.y = position->y + 21.1823;
  P.z = position->z + 71.0231;
  P_out->y = PerlinNoise(&P, lacunarity, persistence, octaves);

  P.x = position->x + 237.492;
  P.y = position->y + 11.1312;
  P.z = position->z + 133.129;
  P_out->z = PerlinNoise(&P, lacunarity, persistence, octaves);
}

double PeriodicNoise3d(double x, double y, double z)
{
  /* Find unit cube that contains point. */
  const int X = (int) floor(x) & 255;
  const int Y = (int) floor(y) & 255;
  const int Z = (int) floor(z) & 255;

  /* Find relative x,y,z of point in cube. */
  const double xx = x - floor(x);
  const double yy = y - floor(y);
  const double zz = z - floor(z);

  /* Compute fade curves for each of x,y,z. */
  const double u = fade(xx);
  const double v = fade(yy);
  const double w = fade(zz);

  /* Hash coordinates of the 8 cube corners. */
  const int A =  perm[X] + Y;
  const int AA = perm[A] + Z;
  const int AB = perm[A + 1] + Z;
  const int B =  perm[X + 1] + Y;
  const int BA = perm[B] + Z;
  const int BB = perm[B + 1] + Z;

  /* Add blended results from 8 corners of cube */
  const double result = 
    lerp(w,
      lerp(v,
        lerp(u, grad(perm[AA],   xx,   yy,   zz),
            grad(perm[BA],   xx-1, yy,   zz)),
        lerp(u, grad(perm[AB],   xx,   yy-1, zz),
            grad(perm[BB],   xx-1, yy-1, zz))),
      lerp(v,
        lerp(u, grad(perm[AA+1], xx,   yy,   zz-1 ),
            grad(perm[BA+1], xx-1, yy,   zz-1)),
        lerp(u, grad(perm[AB+1], xx,   yy-1, zz-1),
            grad(perm[BB+1], xx-1, yy-1, zz-1))));

  return result;
}

static double fade(double t)
{
  return t * t * t * (t * (t * 6 - 15) + 10);
}

static double lerp(double t, double a, double b)
{
  return a + t * (b - a);
}

static double grad(int hash, double x, double y, double z) 
{
  /* Convert lo 4 bits of hash code into 12 gradient directions. */
  const int h = hash & 15;
  const double u = h < 8 ? x : y;
  const double v = h < 4 ? y : h==12 || h==14 ? x : z;

  return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}
