# Fast Poly2tri

![Baseline vs optimized Poly2tri](/fast-poly2tri.png "Baseline vs optimized Poly2tri")

Rewrite in C of the [poly2tri](https://github.com/jhasse/poly2tri) library with focus on speed.

No validation is performed, other than basic debug-time assert.
Passing malformed data will result in a crash, you _must_ pre-validate the data.

**NOTE**: Unlike unlike poly2tri's original implementation I removed
the ability to add Steiner points explicitly, but they can be added
by manipulating the internal state of the context.

You can find out more information on the performance optimizations compared to the original library in the blog post [Chronicle of an Optimization with perf and C](https://metricpanda.com).



## How to include

The library is a single file header.

To include in your project `#define MPE_POLY2TRI_IMPLEMENTATION` before you include this file in *one* C or C++ file to create the implementation like so:

```cpp
#include ...
#define MPE_POLY2TRI_IMPLEMENTATION
#include "MPE_fastpoly2tri.h"
```

On multiple compilation unit builds you must also `#define insternal_static extern`.

## Sample usage

```cpp

// The maximum number of points you expect to need
// This value is used by the library to calculate
// working memory required
uint32_t MaxPointCount = 10000;

// Request how much memory (in bytes) you should
// allocate for the library
size_t MemoryRequired = MPE_PolyMemoryRequired(MaxPointCount);

// Allocate a void* memory block of size MemoryRequired
// IMPORTANT: The memory must be zero initialized
void* Memory = calloc(MemoryRequired, 1);

// Initialize the poly context by passing the memory pointer,
// and max number of points from before
MPEPolyContext PolyContext;
if (MPE_PolyInitContext(&PolyContext, Memory, MaxPointCount))
{
  // Populate the points of the polyline for the shape
  // you want to triangulate using one of the following
  // two methods:

  // Option A - One point at a time
  for(...)
  {
    MPEPolyPoint* Point = MPE_PolyPushPoint(&PolyContext);
    Point->X = ...;
    Point->Y = ...;
  }

  // Option B - memcpy
  uint32_t PointCount = ...;
  // This option requires your point data structure to
  // correspond to MPEPolyPoint. See below.
  MPEPolyPoint* FirstPoint = MPE_PolyPushPointArray(&PolyContext, PointCount);
  memcpy(FirstPoint, YOUR_POINT_DATA, sizeof(MPEPolyPoint)*PointCount);

  // IMPORTANT: Both push functions perform no validation other
  // than an assert for bounds checking. You must make sure your
  // point data is correct:
  //  - Duplicate points are not supported
  //  - Bounds checking is not implemented other than debug asserts

  // Add the polyline for the edge. This will consume all points added so far.
  MPE_PolyAddEdge(&PolyContext);

  // If you want to add holes to the shape you can do:
  if (AddHoles)
  {
    MPEPolyPoint* Hole = MPE_PolyPushPointArray(&PolyContext, 4);
    Hole[0].X = 325; Hole[0].Y = 437;
    Hole[1].X = 320; Hole[1].Y = 423;
    Hole[2].X = 329; Hole[2].Y = 413;
    Hole[3].X = 332; Hole[3].Y = 423;
    MPE_PolyAddHole(&PolyContext);
  }

  // Triangulate the shape
  MPE_PolyTriangulate(&PolyContext);

  // The resulting triangles can be used like so
  for (uxx TriangleIndex = 0; TriangleIndex < PolyContext.TriangleCount; ++TriangleIndex)
  {
    MPEPolyTriangle* Triangle = PolyContext.Triangles[TriangleIndex];
    MPEPolyPoint* PointA = Triangle->Points[0];
    MPEPolyPoint* PointB = Triangle->Points[1];
    MPEPolyPoint* PointC = Triangle->Points[2];
  }
  // You may want to copy the resulting triangle soup into
  // your own data structures if you plan on using them often
  // as the Points and Triangles from the PolyContext are not
  // compact in memory.
}
```

## Configuration

The following defines can be used to tweak the library:

- `#define MPE_POLY2TRI_USE_FAST_ATAN`
  To enable approximate Atan2 function. The implementation provided has an
  error of ~0.005 radians that is good enough for most use cases.

- `#define MPE_POLY2TRI_USE_DOUBLE`
  To use double precision floating point for calculations

- `#define MPE_POLY2TRI_USE_CUSTOM_SORT`
  To use the a custom merge sort implementation. Enabling this option will
  require more working memory.

## Standard library overrides

- `#define MPE_Atan2(Y, X)`
  To replace the math.h atan2f function when not using the approximate atan function

- `#define MPE_MemorySet, MPE_MemoryCopy`
  To avoid including string.h

- `#define internal_static`
  Defaults to static, but you can change it if not using a unity-style build.

## License

Fast Poly2Tri is licensed under the MIT License, see LICENSE for more information.
