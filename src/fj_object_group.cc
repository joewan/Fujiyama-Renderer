/*
Copyright (c) 2011-2014 Hiroshi Tsubokawa
See LICENSE and README
*/

#include "fj_object_group.h"
#include "fj_volume_accelerator.h"
#include "fj_object_instance.h"
#include "fj_primitive_set.h"
#include "fj_accelerator.h"
#include "fj_interval.h"
#include "fj_matrix.h"
#include "fj_memory.h"
#include "fj_array.h"
#include "fj_ray.h"
#include "fj_box.h"

#include <vector>
#include <cassert>
#include <cstdio>
#include <cfloat>

namespace fj {

class ObjectList {
public:
  ObjectList()
  {
    objects = ArrNew(sizeof(struct ObjectInstance *));
    BoxReverseInfinite(&bounds);
  }
  ~ObjectList()
  {
    if (objects != NULL)
      ArrFree(objects);
  }

  Index GetObjectCount() const
  {
    return objects->nelems;
  }
  const Box &GetBounds() const
  {
    return bounds;
  }
  const ObjectInstance *GetObject(Index index) const
  {
    const ObjectInstance **objs = (const ObjectInstance **) objects->data;
    return objs[index];
  }
  void AddObject(const ObjectInstance *obj)
  {
    Box other_bounds;
    ObjGetBounds(obj, &other_bounds);

    ArrPushPointer(objects, obj);
    BoxAddBox(&bounds, other_bounds);
  }
  void ComputeBounds()
  {
    for (int i = 0; i < GetObjectCount(); i++) {
      const ObjectInstance *obj = GetObject(i);
      struct Box obj_bounds;

      ObjGetBounds(obj, &obj_bounds);
      BoxAddBox(&bounds, obj_bounds);
    }
  }

private:
  struct Array *objects;
  //std::vector<ObjectInstance*> objects;
  Box bounds;
};

static struct ObjectList *obj_list_new(void);
static void obj_list_free(struct ObjectList *list);
static void obj_list_add(struct ObjectList *list, const struct ObjectInstance *obj);
static const struct ObjectInstance *get_object(const struct ObjectList *list, int index);

static void object_bounds(const void *prim_set, int prim_id, struct Box *bounds);
static int object_ray_intersect(const void *prim_set, int prim_id, double time,
    const struct Ray *ray, struct Intersection *isect);
static int volume_ray_intersect(const void *prim_set, int prim_id, double time,
    const struct Ray *ray, struct Interval *interval);
static void object_list_bounds(const void *prim_set, struct Box *bounds);
static int object_count(const void *prim_set);

class ObjectGroup : PrimitiveSet {
public:
  ObjectGroup();
  ~ObjectGroup();

  struct ObjectList *surface_list;
  struct ObjectList *volume_list;

  struct Accelerator *surface_acc;
  struct VolumeAccelerator *volume_acc;
};

ObjectGroup::ObjectGroup()
{
}

ObjectGroup::~ObjectGroup()
{
}

struct ObjectGroup *ObjGroupNew(void)
{
  struct ObjectGroup *grp = FJ_MEM_ALLOC(struct ObjectGroup);
  if (grp == NULL)
    return NULL;

  grp->surface_list = NULL;
  grp->volume_list = NULL;
  grp->surface_acc = NULL;
  grp->volume_acc = NULL;

  grp->surface_list = obj_list_new();
  if (grp->surface_list == NULL) {
    ObjGroupFree(grp);
    return NULL;
  }

  grp->volume_list = obj_list_new();
  if (grp->volume_list == NULL) {
    ObjGroupFree(grp);
    return NULL;
  }

  grp->surface_acc = AccNew(ACC_BVH);
  if (grp->surface_acc == NULL) {
    ObjGroupFree(grp);
    return NULL;
  }

  grp->volume_acc = VolumeAccNew(VOLACC_BVH);
  if (grp->volume_acc == NULL) {
    ObjGroupFree(grp);
    return NULL;
  }

  return grp;
}

void ObjGroupFree(struct ObjectGroup *grp)
{
  if (grp == NULL)
    return;

  obj_list_free(grp->surface_list);
  obj_list_free(grp->volume_list);

  AccFree(grp->surface_acc);
  VolumeAccFree(grp->volume_acc);

  FJ_MEM_FREE(grp);
}

void ObjGroupAdd(struct ObjectGroup *grp, const struct ObjectInstance *obj)
{
  if (ObjIsSurface(obj)) {
    struct PrimitiveSet primset;
    obj_list_add(grp->surface_list, obj);

    MakePrimitiveSet(&primset,
        "ObjectInstance:Surface",
        grp->surface_list,
        object_ray_intersect,
        object_bounds,
        object_list_bounds,
        object_count);
    AccSetPrimitiveSet(grp->surface_acc, &primset);
  }
  else if (ObjIsVolume(obj)) {
    obj_list_add(grp->volume_list, obj);

    VolumeAccSetTargetGeometry(grp->volume_acc,
        grp->volume_list,
        grp->volume_list->GetObjectCount(),
        &grp->volume_list->GetBounds(),
        volume_ray_intersect,
        object_bounds);
  }
}

const struct Accelerator *ObjGroupGetSurfaceAccelerator(const struct ObjectGroup *grp)
{
  return grp->surface_acc;
}

const struct VolumeAccelerator *ObjGroupGetVolumeAccelerator(const struct ObjectGroup *grp)
{
  return grp->volume_acc;
}

void ObjGroupComputeBounds(struct ObjectGroup *grp)
{
  grp->surface_list->ComputeBounds();
  grp->volume_list->ComputeBounds();
#if 0
  int N = 0;
  int i;

  N = grp->surface_list->GetObjectCount();
  for (i = 0; i < N; i++) {
    const struct ObjectInstance *obj = get_object(grp->surface_list, i);
    struct Box bounds;

    ObjGetBounds(obj, &bounds);
    BoxAddBox(&grp->surface_list->bounds, bounds);
  }

  N = grp->volume_list->GetObjectCount();
  for (i = 0; i < N; i++) {
    const struct ObjectInstance *obj = get_object(grp->volume_list, i);
    struct Box bounds;

    ObjGetBounds(obj, &bounds);
    BoxAddBox(&grp->volume_list->bounds, bounds);
  }
#endif
}

static void object_bounds(const void *prim_set, int prim_id, struct Box *bounds)
{
  const struct ObjectList *list = (const struct ObjectList *) prim_set;
  const struct ObjectInstance *obj = get_object(list, prim_id);
  ObjGetBounds(obj, bounds);
}

static int object_ray_intersect(const void *prim_set, int prim_id, double time,
    const struct Ray *ray, struct Intersection *isect)
{
  const struct ObjectList *list = (const struct ObjectList *) prim_set;
  const struct ObjectInstance *obj = get_object(list, prim_id);
  return ObjIntersect(obj, time, ray, isect);
}

static int volume_ray_intersect(const void *prim_set, int prim_id, double time,
    const struct Ray *ray, struct Interval *interval)
{
  const struct ObjectList *list = (const struct ObjectList *) prim_set;
  const struct ObjectInstance *obj = get_object(list, prim_id);
  return ObjVolumeIntersect(obj, time, ray, interval);
}

static void object_list_bounds(const void *prim_set, struct Box *bounds)
{
  const struct ObjectList *list = (const struct ObjectList *) prim_set;
  *bounds = list->GetBounds();
}

static int object_count(const void *prim_set)
{
  const struct ObjectList *list = (const struct ObjectList *) prim_set;
  return list->GetObjectCount();
}

static struct ObjectList *obj_list_new(void)
{
  return new ObjectList();
#if 0
  struct ObjectList *list;

  list = FJ_MEM_ALLOC(struct ObjectList);
  if (list == NULL)
    return NULL;

  list->objects = ArrNew(sizeof(struct ObjectInstance *));
  if (list->objects == NULL) {
    obj_list_free(list);
    return NULL;
  }

  BoxReverseInfinite(&list->bounds);

  return list;
#endif
}

static void obj_list_free(struct ObjectList *list)
{
  delete list;
#if 0
  if (list == NULL)
    return;

  if (list->objects != NULL)
    ArrFree(list->objects);

  FJ_MEM_FREE(list);
#endif
}

static void obj_list_add(struct ObjectList *list, const struct ObjectInstance *obj)
{
  list->AddObject(obj);
#if 0
  struct Box bounds;

  ObjGetBounds(obj, &bounds);

  ArrPushPointer(list->objects, obj);
  BoxAddBox(&list->bounds, bounds);
#endif
}

static const struct ObjectInstance *get_object(const struct ObjectList *list, int index)
{
  return list->GetObject(index);
#if 0
  const struct ObjectInstance **objects =
      (const struct ObjectInstance **) list->objects->data;
  return objects[index];
#endif
}

} // namespace xxx
