#pragma once

typedef struct Camera Camera;

// Create a list of display rectangles (x0, y0, x1, y1). Each set of
// four integers is a rectangle, and the first is always the primary
// monitor. The count is the number of integer elements and should be
// divisible by 4. Returns the number of integers filled.
__declspec(dllimport) int     GetDisplayRects(int *rects, int count);

// Create a camera capable of taking screenshots at the rectangle.
__declspec(dllimport) Camera *CreateCamera(int *rect);

// Destroy a camera, releasing all of its resources.
__declspec(dllimport) void    DestroyCamera(Camera *);

// Take a screenshot, returning the 24-bit pixel data. A given camera
// always returns the same pixel data pointer. This buffer may be freely
// modified, but its contents destroyed on the next screenshot.
__declspec(dllimport) void   *CaptureScreenshot(Camera *);
