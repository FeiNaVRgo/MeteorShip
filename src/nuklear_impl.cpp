//
// Created by iformat on 10/19/26.
//
//
// Created by iformat on 10/19/26.
//
// This is the ONLY translation unit that compiles Nuklear's core implementation.
// Do NOT include nuklear_config.h here, because it includes <nuklear.h>
// without NK_IMPLEMENTATION, and the include guard would prevent us from
// defining the implementation afterwards.

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define NK_IMPLEMENTATION
#include <nuklear.h>