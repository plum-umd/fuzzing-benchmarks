/*
 * version.c
 * Copyright (C) 1995 Alexander Lehmann
 * For conditions of distribution and use, see the file COPYING
 */

#include "config.h"
#include "zlib.h"
#include "png.h"

#ifndef PNGLIB
#  define PNGLIB "pnglib (unknown version)"
#endif
#ifndef ZLIB
#  define ZLIB "zlib (unknown version)"
#endif

const char version[] = "gif2png " VERSION;

const char compile_info[]=
#if defined(PNG_LIBPNG_VER_STRING) && defined(ZLIB_VERSION)
  "compiled " __DATE__ " with libpng " PNG_LIBPNG_VER_STRING " and zlib " ZLIB_VERSION ".";
#else
  "compiled " __DATE__ " with " PNGLIB " and " ZLIB ".";
#endif
