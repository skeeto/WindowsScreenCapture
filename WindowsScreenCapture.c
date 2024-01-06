// $ cc -shared -nostartfiles -O -mwindows --entry 0
//      -o WindowsScreenCapture.dll WindowsScreenCapture.c
#include <stddef.h>

enum {
    MEM_COMMIT  = 0x1000,
    MEM_RESERVE = 0x2000,
    MEM_RELEASE = 0x8000,

    PAGE_READWRITE = 4,

    SRCCOPY = 0x00cc0020,
};

#define W32(r) __declspec(dllimport) r __stdcall
W32(int)    BitBlt(void *, int, int, int, int, void *, int, int, int);
W32(void *) CreateCompatibleDC(void *);
W32(void *) CreateDIBSection(void *, void *, int, void **, void *, int);
W32(int)    DeleteDC(void *);
W32(int)    DeleteObject(void *);
W32(int)    EnumDisplayMonitors(void *, void *, void *, void *);
W32(void *) GetDC(void *);
W32(int)    GetMonitorInfoA(void *, void *);
W32(int)    ReleaseDC(void *, void *);
W32(void *) SelectObject(void *, void *);
W32(void *) VirtualAlloc(void *, size_t, int, int);
W32(int)    VirtualFree(void *, size_t, int);

typedef struct {
    void *pixels;
    int   rect[4];
    void *dc;
    void *screen;
    void *hbmp;
} Camera;

#define API __declspec(dllexport)
API int     GetDisplayRects(int *, int);
API Camera *CreateCamera(int *);
API void    DestroyCamera(Camera *);
API void   *CaptureScreenshot(Camera *);

static void copy(int *dst, int *src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

typedef struct {
    int *rects;
    int  cap;
    int  len;
    int  primary[4];
    int  primaryi;
} cbdata;

static int __stdcall callback(void *h, void *hdc, int *rect, cbdata *data)
{
    struct {
        int size;
        int rect0[4];
        int rect1[4];
        int flags;
    } info = {0};
    info.size = sizeof(info);

    if (GetMonitorInfoA(h, &info)) {
        if (info.flags & 1) {
            data->primaryi = data->len;
            copy(data->primary, info.rect0);
        }
        if (data->len < data->cap) {
            copy(data->rects+data->len, info.rect0);
            data->len += 4;
        }
    }

    return data->primaryi<0 || data->len<data->cap;
}

int GetDisplayRects(int *rects, int cap)
{
    cap &= -3;  // round down
    if (!cap) {
        return 0;
    }

    cbdata data = {0};
    data.rects = rects;
    data.cap = cap;
    data.primaryi = -1;

    EnumDisplayMonitors(0, 0, callback, &data);
    if (data.primaryi != -1) {
        if (data.primaryi < data.len) {
            copy(rects+data.primaryi, rects+0);
        }
        copy(rects+0, data.primary);
    }

    return data.len;
}

Camera *CreateCamera(int *rect)
{
    Camera *c = VirtualAlloc(
        0, sizeof(Camera), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE
    );
    if (!c) {
        return 0;
    }
    c->screen = GetDC(0);
    c->dc = CreateCompatibleDC(c->screen);
    copy(c->rect, rect);

    int width  = rect[2] - rect[0];
    int height = rect[3] - rect[1];

    struct {
        int size;
        int width;
        int height;
        short planes;
        short depth;
        int compression;
        int sizeimage;
        int resx;
        int resy;
        int colors;
        int important;
    } bmp = {0};
    bmp.size = sizeof(bmp);
    bmp.width  = width;
    bmp.height = -height;
    bmp.depth  = 24;
    bmp.planes = 1;

    c->hbmp = CreateDIBSection(c->dc, &bmp, 0, &c->pixels, 0, 0);
    if (!c->hbmp) {
        ReleaseDC(0, c->screen);
        DeleteDC(c->dc);
        VirtualFree(c, 0, MEM_RELEASE);
    }

    return c;
}

void DestroyCamera(Camera *c)
{
    DeleteObject(c->hbmp);
    ReleaseDC(0, c->screen);
    DeleteDC(c->dc);
    VirtualFree(c, 0, MEM_RELEASE);
}

void *CaptureScreenshot(Camera *c)
{
    SelectObject(c->dc, c->hbmp);
    int x = c->rect[0];
    int y = c->rect[1];
    int w = c->rect[2] - c->rect[0];
    int h = c->rect[3] - c->rect[1];
    if (!BitBlt(c->dc, 0, 0, w, h, c->screen, x, y, SRCCOPY)) {
        return 0;
    }
    return c->pixels;
}
