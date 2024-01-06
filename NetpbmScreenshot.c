// Screenshot the primary display to standard output as a Netpbm (P6)
// $ cc -nostartfiles -O -o NetpbmScreenshot.exe NetpbmScreenshot.c
//      WindowsScreenCapture.dll
// Demonstrates WindowsScreenCapture.dll usage.
#include "WindowsScreenCapture.h"

#define countof(a) (int)(sizeof(a) / sizeof(*(a)))

#define W32(r) __declspec(dllimport) r __stdcall
W32(void *) GetStdHandle(int);
W32(int)    WriteFile(void *, void *, int, int *, void *);
W32(void)   ExitProcess(int);

typedef struct {
    char *buf;
    int   cap;
    int   len;
} output;

static void append(output *o, char c)
{
    if (o->len < o->cap) {
        o->buf[o->len++] = c;
    }
}

static void appendint(output *o, int x)
{
    char buf[16];
    char *beg = buf + countof(buf);
    do {
        *--beg = (char)(x%10) + '0';
    } while (x /= 10);
    while (beg < buf+countof(buf)) {
        append(o, *beg++);
    }
}

static int run(void)
{
    int rects[4];  // only get the primary monitor
    int len = GetDisplayRects(rects, countof(rects));
    if (!len) {
        return 1;
    }

    int width  = rects[2] - rects[0];
    int height = rects[3] - rects[1];
    Camera *camera = CreateCamera(rects);
    if (!camera) return 0;

    unsigned char *data = CaptureScreenshot(camera);
    if (!data) return 0;
    for (int i = 0; i < width*height*3; i+= 3) {
        unsigned char t = data[i+0];  // swap red-blue
        data[i+0] = data[i+2];
        data[i+2] = t;
    }

    // Write a Netpbm header
    char buf[64];
    output hdr = {0};
    hdr.buf = buf;
    hdr.cap = countof(buf);
    append(&hdr, 'P');
    append(&hdr, '6');
    append(&hdr, '\n');
    appendint(&hdr, width);
    append(&hdr, ' ');
    appendint(&hdr, height);
    append(&hdr, '\n');
    appendint(&hdr, 255);
    append(&hdr, '\n');

    int dummy;
    int err = 0;
    void *stdout = GetStdHandle(-11);
    err |= !WriteFile(stdout, hdr.buf, hdr.len, &dummy, 0);
    err |= !WriteFile(stdout, data, width*height*3, &dummy, 0);

    DestroyCamera(camera);  // for testing/demonstration
    return err;
}

void mainCRTStartup(void)
{
    int r = run();
    ExitProcess(r);
}
