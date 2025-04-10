#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESIZE_MARGIN 16

enum {
    NONE, DRAG, RESIZE_TL, RESIZE_TR, RESIZE_BL, RESIZE_BR
};

int get_resize_region(int x, int y, int width, int height) {
    if (x <= RESIZE_MARGIN && y <= RESIZE_MARGIN) return RESIZE_TL;
    if (x >= width - RESIZE_MARGIN && y <= RESIZE_MARGIN) return RESIZE_TR;
    if (x <= RESIZE_MARGIN && y >= height - RESIZE_MARGIN) return RESIZE_BL;
    if (x >= width - RESIZE_MARGIN && y >= height - RESIZE_MARGIN) return RESIZE_BR;
    return DRAG;
}

int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    int win_x = 500, win_y = 900, win_w = 1600, win_h = 100;

    Window win = XCreateSimpleWindow(display, root, win_x, win_y, win_w, win_h, 0,
                                     BlackPixel(display, screen),
                                     WhitePixel(display, screen));

    //Set opacity
    //unsigned long opacity = (unsigned long)(0.1 * 0xFFFFFFFF); // 80% opaque
    //Atom opacity_atom = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);
    //XChangeProperty(display, win, opacity_atom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&opacity, 1);

    //Change color
    XSetWindowBackground(display, win, 0x808080);

    // Frameless
    Atom wmHints = XInternAtom(display, "_MOTIF_WM_HINTS", False);
    struct {
        unsigned long flags, functions, decorations;
        long input_mode;
        unsigned long status;
    } hints = {2, 0, 0, 0, 0};
    XChangeProperty(display, win, wmHints, wmHints, 32, PropModeReplace,
                    (unsigned char *)&hints, 5);

    // Always on top
    Atom state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(display, win, state, XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)&above, 1);

    // Set WM_CLASS and WM_NAME
    XClassHint *classHint = XAllocClassHint();
    classHint->res_name = "subtitlehider";
    classHint->res_class = "SubtitleHider";
    XSetClassHint(display, win, classHint);
    XFree(classHint);

    XStoreName(display, win, "Subtitle Hider");

    // Handle close event (WM_DELETE_WINDOW)
    Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, win, &wm_delete, 1);

    // Select input
    XSelectInput(display, win, ExposureMask | ButtonPressMask |
                              ButtonReleaseMask | Button1MotionMask |
                              StructureNotifyMask);

    XMapWindow(display, win);

    int dragging = 0;
    int drag_start_x = 0, drag_start_y = 0;
    int win_start_x = 0, win_start_y = 0;
    int win_start_w = win_w, win_start_h = win_h;
    int mode = NONE;

    XEvent event;
    while (1) {
        XNextEvent(display, &event);

        if (event.type == Expose) {
            // (Optional) draw something
        }

        else if (event.type == ButtonPress && event.xbutton.button == Button1) {
            int x = event.xbutton.x;
            int y = event.xbutton.y;
            Window child;
            XWindowAttributes attrs;
            XGetWindowAttributes(display, win, &attrs);
            mode = get_resize_region(x, y, attrs.width, attrs.height);

            drag_start_x = event.xbutton.x_root;
            drag_start_y = event.xbutton.y_root;

            XTranslateCoordinates(display, win, root, 0, 0,
                                  &win_start_x,
                                  &win_start_y,
                                  &child);
            win_start_w = attrs.width;
            win_start_h = attrs.height;

            dragging = 1;
        }

        else if (event.type == MotionNotify && dragging) {
            int dx = event.xmotion.x_root - drag_start_x;
            int dy = event.xmotion.y_root - drag_start_y;

            if (mode == DRAG) {
                XMoveWindow(display, win, win_start_x + dx, win_start_y + dy);
            } else {
                int new_w = win_start_w;
                int new_h = win_start_h;
                int new_x = win_start_x;
                int new_y = win_start_y;

                if (mode == RESIZE_TL) {
                    new_w -= dx;
                    new_h -= dy;
                    new_x += dx;
                    new_y += dy;
                } else if (mode == RESIZE_TR) {
                    new_w += dx;
                    new_h -= dy;
                    new_y += dy;
                } else if (mode == RESIZE_BL) {
                    new_w -= dx;
                    new_h += dy;
                    new_x += dx;
                } else if (mode == RESIZE_BR) {
                    new_w += dx;
                    new_h += dy;
                }

                // Minimum size
                if (new_w < 100) new_w = 100;
                if (new_h < 100) new_h = 100;

                XMoveResizeWindow(display, win, new_x, new_y, new_w, new_h);
            }
        }

        else if (event.type == ButtonRelease && event.xbutton.button == Button1) {
            dragging = 0;
            mode = NONE;
        }

        else if (event.type == ClientMessage) {
            if ((Atom)event.xclient.data.l[0] == wm_delete) {
                break;  // Clean exit
            }
        }
    }

    XDestroyWindow(display, win);
    XCloseDisplay(display);
    return 0;
}
