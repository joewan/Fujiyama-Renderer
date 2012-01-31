/*
Copyright (c) 2011-2012 Hiroshi Tsubokawa
See LICENSE and README
*/

#ifndef STRING_H
#define STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

extern char *StrDup(const char *src);
extern char *StrFree(char *s);

extern char *StrCopyMax(char *dst, const char *src, size_t maxsize);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XXX_H */
