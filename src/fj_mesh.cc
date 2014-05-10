/*
Copyright (c) 2011-2014 Hiroshi Tsubokawa
See LICENSE and README
*/

#include "fj_mesh.h"
#include "fj_intersection.h"
#include "fj_primitive_set.h"
#include "fj_triangle.h"
#include "fj_tex_coord.h"
#include "fj_memory.h"
#include "fj_vector.h"
#include "fj_color.h"
#include "fj_ray.h"
#include "fj_box.h"

#include <string.h>
#include <float.h>

#if 0
#include <stdio.h>
/* TODO TEST VELOCITY */
void MbSampleVelocityPoint(const struct Vector *P, const struct Vector *velocity,
    double time, double time_offset, double shutter_speed,
    struct Vector *P_sampled)
{
  /*
  time_offset = -1 : trailing blur
  time_offset = 0  : frame center blur
  time_offset = 1  : leading center blur
  */

  const double s[] = {-2, -1.5, -1, -.5, 0, .5, 1, 2};
  int i;
  printf("--------------------------------------\n");
  for (i = 0; i < sizeof(s)/sizeof(s[0]); i++) {
    printf("| %5.2G", s[i]);
  }
  printf("\n");
  for (i = 0; i < sizeof(s)/sizeof(s[0]); i++) {
    double t = (.5 * s[i] - .5);
    /*
    t = s[i] - .5;
    */
    printf("| %5.2G", t);
  }
  printf("\n");
}
#endif

static int triangle_ray_intersect(const void *prim_set, int prim_id, double time,
    const struct Ray *ray, struct Intersection *isect);
static void triangle_bounds(const void *prim_set, int prim_id, struct Box *bounds);
static void triangleset_bounds(const void *prim_set, struct Box *bounds);
static int triangle_count(const void *prim_set);

struct Mesh {
  int nverts;
  int nfaces;

  struct Vector *P;
  struct Vector *N;
  struct Color *Cd;
  struct TexCoord *uv;
  struct Vector *velocity;
  struct TriIndex *indices;

  struct Box bounds;
};

struct Mesh *MshNew(void)
{
  struct Mesh *mesh = FJ_MEM_ALLOC(struct Mesh);
  if (mesh == NULL)
    return NULL;

  mesh->nverts = 0;
  mesh->nfaces = 0;
  BOX3_SET(&mesh->bounds, 0, 0, 0, 0, 0, 0);

  mesh->P = NULL;
  mesh->N = NULL;
  mesh->Cd = NULL;
  mesh->uv = NULL;
  mesh->velocity = NULL;
  mesh->indices = NULL;

  return mesh;
}

void MshFree(struct Mesh *mesh)
{
  if (mesh == NULL)
    return;

  VecFree(mesh->P);
  VecFree(mesh->N);
  ColFree(mesh->Cd);
  TexCoordFree(mesh->uv);
  VecFree(mesh->velocity);
  FJ_MEM_FREE(mesh->indices);

  FJ_MEM_FREE(mesh);
}

void MshClear(struct Mesh *mesh)
{
  if (mesh->P != NULL)
    VecFree(mesh->P);
  if (mesh->N != NULL)
    VecFree(mesh->N);
  if (mesh->Cd != NULL)
    ColFree(mesh->Cd);
  if (mesh->uv != NULL)
    TexCoordFree(mesh->uv);
  if (mesh->velocity != NULL)
    VecFree(mesh->velocity);
  if (mesh->indices != NULL)
    FJ_MEM_FREE(mesh->indices);

  mesh->nverts = 0;
  mesh->nfaces = 0;
  BOX3_SET(&mesh->bounds, 0, 0, 0, 0, 0, 0);

  mesh->P = NULL;
  mesh->N = NULL;
  mesh->Cd = NULL;
  mesh->uv = NULL;
  mesh->velocity = NULL;
  mesh->indices = NULL;
}

void MshComputeBounds(struct Mesh *mesh)
{
  int i;
  BOX3_SET(&mesh->bounds, FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

  for (i = 0; i < mesh->nfaces; i++) {
    struct Box bounds;
    triangle_bounds(mesh, i, &bounds);
    BoxAddBox(&mesh->bounds, &bounds);
  }
}

void MshComputeNormals(struct Mesh *mesh)
{
  const int nverts = MshGetVertexCount(mesh);
  const int nfaces = MshGetFaceCount(mesh);
  struct Vector *P = mesh->P;
  struct Vector *N = mesh->N;
  struct TriIndex *indices = mesh->indices;
  int i;

  if (P == NULL || indices == NULL)
    return;

  if (N == NULL) {
    MshAllocateVertex(mesh, "N", nverts);
    N = mesh->N;
  }

  /* initialize N */
  for (i = 0; i < nverts; i++) {
    struct Vector *nml = &N[i];
    VEC3_SET(nml, 0, 0, 0);
  }

  /* compute N */
  for (i = 0; i < nfaces; i++) {
    struct Vector *P0, *P1, *P2;
    struct Vector *N0, *N1, *N2;
    struct Vector Ng;
    const struct TriIndex *face = &indices[i];

    P0 = &P[face->i0];
    P1 = &P[face->i1];
    P2 = &P[face->i2];
    N0 = &N[face->i0];
    N1 = &N[face->i1];
    N2 = &N[face->i2];

    TriComputeFaceNormal(&Ng, P0, P1, P2);

    N0->x += Ng.x;
    N0->y += Ng.y;
    N0->z += Ng.z;

    N1->x += Ng.x;
    N1->y += Ng.y;
    N1->z += Ng.z;

    N2->x += Ng.x;
    N2->y += Ng.y;
    N2->z += Ng.z;
  }

  /* normalize N */
  for (i = 0; i < nverts; i++) {
    struct Vector *nml = &N[i];
    VEC3_NORMALIZE(nml);
  }
}

void *MshAllocateVertex(struct Mesh *mesh, const char *attr_name, int nverts)
{
  void *ret = NULL;

  if (strcmp(attr_name, "P") == 0) {
    mesh->P = VecRealloc(mesh->P, nverts);
    ret = mesh->P;
  }
  else if (strcmp(attr_name, "N") == 0) {
    mesh->N = VecRealloc(mesh->N, nverts);
    ret = mesh->N;
  }
  else if (strcmp(attr_name, "uv") == 0) {
    mesh->uv = TexCoordRealloc(mesh->uv, nverts);
    ret = mesh->uv;
  }
  else if (strcmp(attr_name, "velocity") == 0) {
    mesh->velocity = VecRealloc(mesh->velocity, nverts);
    ret = mesh->velocity;
  }

  mesh->nverts = nverts;
  return ret;
}

void *MshAllocateFace(struct Mesh *mesh, const char *attr_name, int nfaces)
{
  void *ret = NULL;

  if (strcmp(attr_name, "indices") == 0) {
    /* TODO define TriIndexRealloc */
    mesh->indices = FJ_MEM_REALLOC_ARRAY(mesh->indices, struct TriIndex, nfaces);
    ret = mesh->indices;
  }

  mesh->nfaces = nfaces;
  return ret;
}

void MshGetFaceVertexPosition(const struct Mesh *mesh, int face_index,
    struct Vector *P0, struct Vector *P1, struct Vector *P2)
{
  const struct TriIndex *face = &mesh->indices[face_index];

  *P0 = mesh->P[face->i0];
  *P1 = mesh->P[face->i1];
  *P2 = mesh->P[face->i2];
}

void MshGetFaceVertexNormal(const struct Mesh *mesh, int face_index,
    struct Vector *N0, struct Vector *N1, struct Vector *N2)
{
  const struct TriIndex *face = &mesh->indices[face_index];

  *N0 = mesh->N[face->i0];
  *N1 = mesh->N[face->i1];
  *N2 = mesh->N[face->i2];
}

void MshSetVertexPosition(struct Mesh *mesh, int index, const struct Vector *P)
{
  if (mesh->P == NULL)
    return;
  if (index < 0 || index >= mesh->nverts)
    return;

  mesh->P[index] = *P;
}

void MshSetVertexNormal(struct Mesh *mesh, int index, const struct Vector *N)
{
  if (mesh->N == NULL)
    return;
  if (index < 0 || index >= mesh->nverts)
    return;

  mesh->N[index] = *N;
}

void MshSetVertexColor(struct Mesh *mesh, int index, const struct Color *Cd)
{
  if (mesh->Cd == NULL)
    return;
  if (index < 0 || index >= mesh->nverts)
    return;

  mesh->Cd[index] = *Cd;
}

void MshSetVertexTexture(struct Mesh *mesh, int index, const struct TexCoord *uv)
{
  if (mesh->uv == NULL)
    return;
  if (index < 0 || index >= mesh->nverts)
    return;

  mesh->uv[index] = *uv;
}

void MshSetVertexVelocity(struct Mesh *mesh, int index, const struct Vector *velocity)
{
  if (mesh->velocity == NULL)
    return;
  if (index < 0 || index >= mesh->nverts)
    return;

  mesh->velocity[index] = *velocity;
}

void MshSetFaceVertexIndices(struct Mesh *mesh, int face_index,
    const struct TriIndex *tri_index)
{
  if (mesh->indices == NULL)
    return;
  if (face_index < 0 || face_index >= mesh->nfaces)
    return;

  mesh->indices[face_index] = *tri_index;
}

void MshGetVertexPosition(const struct Mesh *mesh, int index, struct Vector *P)
{
  *P = mesh->P[index];
}

void MshGetVertexNormal(const struct Mesh *mesh, int index, struct Vector *N)
{
  *N = mesh->N[index];
}

void MshGetFaceVertexIndices(const struct Mesh *mesh, int face_index,
    struct TriIndex *tri_index)
{
  *tri_index = mesh->indices[face_index];
}

int MshGetVertexCount(const struct Mesh *mesh)
{
  return mesh->nverts;
}

int MshGetFaceCount(const struct Mesh *mesh)
{
  return mesh->nfaces;
}

void MshGetPrimitiveSet(const struct Mesh *mesh, struct PrimitiveSet *primset)
{
  MakePrimitiveSet(primset,
      "Mesh",
      mesh,
      triangle_ray_intersect,
      triangle_bounds,
      triangleset_bounds,
      triangle_count);
}

static int triangle_ray_intersect(const void *prim_set, int prim_id, double time,
    const struct Ray *ray, struct Intersection *isect)
{
  const struct Mesh *mesh = (const struct Mesh *) prim_set;
  const struct TriIndex *face = &mesh->indices[prim_id];
  struct Vector P0, P1, P2;
  const struct Vector *N0, *N1, *N2;
  double u, v;
  double t_hit;
  int hit;

  /* TODO make function */
  P0 = mesh->P[face->i0];
  P1 = mesh->P[face->i1];
  P2 = mesh->P[face->i2];

  if (mesh->velocity != NULL) {
    const struct Vector *velocity0, *velocity1, *velocity2;
    velocity0 = &mesh->velocity[face->i0];
    velocity1 = &mesh->velocity[face->i1];
    velocity2 = &mesh->velocity[face->i2];

    P0.x += time * velocity0->x;
    P0.y += time * velocity0->y;
    P0.z += time * velocity0->z;

    P1.x += time * velocity1->x;
    P1.y += time * velocity1->y;
    P1.z += time * velocity1->z;

    P2.x += time * velocity2->x;
    P2.y += time * velocity2->y;
    P2.z += time * velocity2->z;
  }

  hit = TriRayIntersect(
      &P0, &P1, &P2,
      &ray->orig, &ray->dir, DO_NOT_CULL_BACKFACES,
      &t_hit, &u, &v);

  if (!hit)
    return 0;

  if (isect == NULL)
    return 1;

  /* we don't know N at time sampled point with velocity motion blur */
  /* just using N from mesh data */
  /* intersect info */
  N0 = &mesh->N[face->i0];
  N1 = &mesh->N[face->i1];
  N2 = &mesh->N[face->i2];
  TriComputeNormal(&isect->N, N0, N1, N2, u, v);

  /* TODO TMP uv handling */
  /* UV = (1-u-v) * UV0 + u * UV1 + v * UV2 */
  if (mesh->uv != NULL) {
    const struct TexCoord *uv0, *uv1, *uv2;
    const float t = 1-u-v;
    uv0 = &mesh->uv[face->i0];
    uv1 = &mesh->uv[face->i1];
    uv2 = &mesh->uv[face->i2];
    isect->uv.u = t * uv0->u + u * uv1->u + v * uv2->u;
    isect->uv.v = t * uv0->v + u * uv1->v + v * uv2->v;

    TriComputeDerivatives(
        &P0, &P1, &P2,
        uv0, uv1, uv2,
        &isect->dPdu, &isect->dPdv);
  }
  else {
    isect->uv.u = 0;
    isect->uv.v = 0;

    VEC3_SET(&isect->dPdu, 0, 0, 0);
    VEC3_SET(&isect->dPdv, 0, 0, 0);
  }

  POINT_ON_RAY(&isect->P, &ray->orig, &ray->dir, t_hit);
  isect->object = NULL;
  isect->prim_id = prim_id;
  isect->t_hit = t_hit;

  return 1;
}

static void triangle_bounds(const void *prim_set, int prim_id, struct Box *bounds)
{
  const struct Mesh *mesh = (const struct Mesh *) prim_set;
  const struct TriIndex *face = &mesh->indices[prim_id];
  const struct Vector *P0, *P1, *P2;

  P0 = &mesh->P[face->i0];
  P1 = &mesh->P[face->i1];
  P2 = &mesh->P[face->i2];

  TriComputeBounds(bounds, P0, P1, P2);

  if (mesh->velocity != NULL) {
    const struct Vector *velocity0, *velocity1, *velocity2;
    struct Vector P0_close, P1_close, P2_close;

    velocity0 = &mesh->velocity[face->i0];
    velocity1 = &mesh->velocity[face->i1];
    velocity2 = &mesh->velocity[face->i2];

    P0_close.x = P0->x + velocity0->x;
    P0_close.y = P0->y + velocity0->y;
    P0_close.z = P0->z + velocity0->z;

    P1_close.x = P1->x + velocity1->x;
    P1_close.y = P1->y + velocity1->y;
    P1_close.z = P1->z + velocity1->z;

    P2_close.x = P2->x + velocity2->x;
    P2_close.y = P2->y + velocity2->y;
    P2_close.z = P2->z + velocity2->z;

    BoxAddPoint(bounds, &P0_close);
    BoxAddPoint(bounds, &P1_close);
    BoxAddPoint(bounds, &P2_close);
  }
}

static void triangleset_bounds(const void *prim_set, struct Box *bounds)
{
  const struct Mesh *mesh = (const struct Mesh *) prim_set;
  *bounds = mesh->bounds;
}

static int triangle_count(const void *prim_set)
{
  const struct Mesh *mesh = (const struct Mesh *) prim_set;
  return mesh->nfaces;
}