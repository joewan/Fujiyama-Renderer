/*
Copyright (c) 2011-2014 Hiroshi Tsubokawa
See LICENSE and README
*/

#include "fj_turbulence.h"
#include "fj_vector.h"
#include "fj_memory.h"
#include "fj_noise.h"

#include <stdio.h>
#include <assert.h>

struct Turbulence {
  struct Vector amplitude;
  struct Vector frequency;
  struct Vector offset;
  double lacunarity;
  double gain;
  int octaves;
};

struct Turbulence *TrbNew(void)
{
  struct Turbulence *turbulence = FJ_MEM_ALLOC(struct Turbulence);
  if (turbulence == NULL)
    return NULL;

  VEC3_SET(&turbulence->amplitude, 1, 1, 1);
  VEC3_SET(&turbulence->frequency, 1, 1, 1);
  VEC3_SET(&turbulence->offset, 0, 0, 0);
  turbulence->lacunarity = 2;
  turbulence->gain = .5;
  turbulence->octaves = 8;

  return turbulence;
}

void TrbFree(struct Turbulence *turbulence)
{
  if (turbulence == NULL)
    return;
  FJ_MEM_FREE(turbulence);
}

void TrbSetAmplitude(struct Turbulence *turbulence, double x, double y, double z)
{
  VEC3_SET(&turbulence->amplitude, x, y, z);
}

void TrbSetFrequency(struct Turbulence *turbulence, double x, double y, double z)
{
  VEC3_SET(&turbulence->frequency, x, y, z);
}

void TrbSetOffset(struct Turbulence *turbulence, double x, double y, double z)
{
  VEC3_SET(&turbulence->offset, x, y, z);
}

void TrbSetLacunarity(struct Turbulence *turbulence, double lacunarity)
{
  turbulence->lacunarity = lacunarity;
}

void TrbSetGain(struct Turbulence *turbulence, double gain)
{
  turbulence->gain = gain;
}

void TrbSetOctaves(struct Turbulence *turbulence, int octaves)
{
  assert(octaves > 0);
  turbulence->octaves = octaves;
}

double TrbEvaluate(const struct Turbulence *turbulence, const struct Vector *position)
{
  struct Vector P;
  double noise_value = 0;

  P.x = position->x * turbulence->frequency.x + turbulence->offset.x;
  P.y = position->y * turbulence->frequency.y + turbulence->offset.y;
  P.z = position->z * turbulence->frequency.z + turbulence->offset.z;

  noise_value = PerlinNoise(&P,
      turbulence->lacunarity,
      turbulence->gain,
      turbulence->octaves);

  return noise_value * turbulence->amplitude.x;
}

void TrbEvaluate3d(const struct Turbulence *turbulence, const struct Vector *position,
    struct Vector *out_noise)
{
  struct Vector P;

  P.x = position->x * turbulence->frequency.x + turbulence->offset.x;
  P.y = position->y * turbulence->frequency.y + turbulence->offset.y;
  P.z = position->z * turbulence->frequency.z + turbulence->offset.z;

  PerlinNoise3d(&P,
      turbulence->lacunarity,
      turbulence->gain,
      turbulence->octaves,
      out_noise);

  out_noise->x *= turbulence->amplitude.x;
  out_noise->y *= turbulence->amplitude.y;
  out_noise->z *= turbulence->amplitude.z;
}
