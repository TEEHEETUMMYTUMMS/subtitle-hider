#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>

#define RESIZE_MARGIN 20
#define INDICATOR_SIZE 20
#define BORDER_WIDTH 5
#define MIN_WIDTH 50
#define MIN_HEIGHT 50

enum { NONE, DRAG, RESIZE_TL, RESIZE_TR, RESIZE_BL, RESIZE_BR };

int get_resize_region(int x, int y, int w, int h) {
	if (x <= RESIZE_MARGIN && y <= RESIZE_MARGIN) return RESIZE_TL;
	if (x >= w - RESIZE_MARGIN && y <= RESIZE_MARGIN) return RESIZE_TR;
	if (x <= RESIZE_MARGIN && y >= h - RESIZE_MARGIN) return RESIZE_BL;
	if (x >= w - RESIZE_MARGIN && y >= h - RESIZE_MARGIN) return RESIZE_BR;
	return DRAG;
}

void draw_all_indicators(Display *d, Window win, GC gc, int w, int h) {
	XFillRectangle(d, win, gc, 0, 0, INDICATOR_SIZE, INDICATOR_SIZE);
	XFillRectangle(d, win, gc, w - INDICATOR_SIZE, 0, INDICATOR_SIZE, INDICATOR_SIZE);
	XFillRectangle(d, win, gc, 0, h - INDICATOR_SIZE, INDICATOR_SIZE, INDICATOR_SIZE);
	XFillRectangle(d, win, gc, w - INDICATOR_SIZE, h - INDICATOR_SIZE, INDICATOR_SIZE, INDICATOR_SIZE);
}

void draw_border_and_indicators(Display *d, Window win, GC gc) {
	XWindowAttributes att;
	XGetWindowAttributes(d, win, &att);
	XDrawRectangle(d, win, gc,
				BORDER_WIDTH/2, BORDER_WIDTH/2,
				att.width - BORDER_WIDTH,
				att.height - BORDER_WIDTH);
	draw_all_indicators(d, win, gc, att.width, att.height);
}

int main() {
	Display *d = XOpenDisplay(NULL);
	if (!d) exit(1);
	int s = DefaultScreen(d);
	Window root = RootWindow(d, s);
	int x0 = 500, y0 = 900, w0 = 1600, h0 = 100;

	XVisualInfo vinfo;
	if (!XMatchVisualInfo(d, s, 32, TrueColor, &vinfo)) {
		vinfo.visual = DefaultVisual(d, s);
		vinfo.depth = DefaultDepth(d, s);
	}
	XSetWindowAttributes a;
	a.colormap = XCreateColormap(d, root, vinfo.visual, AllocNone);
	a.background_pixel = 0;
	a.border_pixel = 0;
	a.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask |
		Button1MotionMask | PointerMotionMask |
		EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
	Window win = XCreateWindow(d, root, x0, y0, w0, h0, 0, vinfo.depth,
							InputOutput, vinfo.visual,
							CWColormap|CWBackPixel|CWBorderPixel|CWEventMask, &a);

	unsigned long opa = (unsigned long)(0.3*0xFFFFFFFF);
	Atom prop_opacity = XInternAtom(d, "_NET_WM_WINDOW_OPACITY", False);
	XChangeProperty(d, win, prop_opacity, XA_CARDINAL, 32,
				 PropModeReplace, (unsigned char *)&opa, 1);

	Atom prop_motif = XInternAtom(d, "_MOTIF_WM_HINTS", False);
	unsigned long mhints[5] = {2,0,0,0,0};
	XChangeProperty(d, win, prop_motif, prop_motif, 32,
				 PropModeReplace, (unsigned char *)mhints, 5);

	Atom prop_state = XInternAtom(d, "_NET_WM_STATE", False);
	Atom prop_above = XInternAtom(d, "_NET_WM_STATE_ABOVE", False);
	XChangeProperty(d, win, prop_state, XA_ATOM, 32,
				 PropModeReplace, (unsigned char *)&prop_above, 1);

	GC gc = XCreateGC(d, win, 0, NULL);
	Colormap cm = DefaultColormap(d, s);
	XColor rc;
	XParseColor(d, cm, "red", &rc);
	XAllocColor(d, cm, &rc);
	XSetForeground(d, gc, rc.pixel);
	XSetLineAttributes(d, gc, BORDER_WIDTH, LineSolid, CapButt, JoinMiter);

	XClassHint *ch = XAllocClassHint();
	ch->res_name = "subtitlehider";
	ch->res_class = "SubtitleHider";
	XSetClassHint(d, win, ch);
	XFree(ch);

	XMapWindow(d, win);

	int dragging = 0, mode = NONE;
	int start_x, start_y, start_w, start_h;
	int win_x, win_y;
	XEvent e;

	while (1) {
		XNextEvent(d, &e);
		if (e.type == Expose) {
			if (!dragging) draw_border_and_indicators(d, win, gc);
		} else if (e.type == EnterNotify) {
			XClearWindow(d, win);
			draw_border_and_indicators(d, win, gc);
		} else if (e.type == LeaveNotify) {
			if (!dragging) {
				mode = NONE;
				XClearWindow(d, win);
			}
		} else if (e.type == MotionNotify) {
			if (dragging) {
				if (mode == DRAG) {
					int nx = e.xmotion.x_root - start_x;
					int ny = e.xmotion.y_root - start_y;
					XMoveWindow(d, win, nx, ny);
				} else {
					XWindowAttributes att;
					XGetWindowAttributes(d, win, &att);
					int dx = e.xmotion.x_root - (win_x + start_x);
					int dy = e.xmotion.y_root - (win_y + start_y);
					int nw = start_w;
					int nh = start_h;
					int nx = win_x;
					int ny = win_y;
					if (mode == RESIZE_TL) { nw -= dx; nh -= dy; nx += dx; ny += dy; }
					if (mode == RESIZE_TR) { nw += dx; nh -= dy; ny += dy; }
					if (mode == RESIZE_BL) { nw -= dx; nh += dy; nx += dx; }
					if (mode == RESIZE_BR) { nw += dx; nh += dy; }
					if (nw < MIN_WIDTH) nw = MIN_WIDTH;
					if (nh < MIN_HEIGHT) nh = MIN_HEIGHT;
					XMoveResizeWindow(d, win, nx, ny, nw, nh);
					draw_border_and_indicators(d, win, gc);
				}
			} else {
				XWindowAttributes att;
				XGetWindowAttributes(d, win, &att);
				int new_mode = get_resize_region(e.xmotion.x, e.xmotion.y, att.width, att.height);
				if (new_mode != mode) {
					mode = new_mode;
					XClearWindow(d, win);
					if (mode != NONE) draw_border_and_indicators(d, win, gc);
				}
			}
		} else if (e.type == ButtonPress && e.xbutton.button == Button1) {
			Window child;
			XTranslateCoordinates(d, win, root, 0, 0, &win_x, &win_y, &child);
			start_x = e.xbutton.x_root - win_x;
			start_y = e.xbutton.y_root - win_y;
			XWindowAttributes att;
			XGetWindowAttributes(d, win, &att);
			start_w = att.width;
			start_h = att.height;
			mode = get_resize_region(e.xbutton.x, e.xbutton.y, att.width, att.height);
			dragging = 1;
		} else if (e.type == ButtonRelease && e.xbutton.button == Button1) {
			dragging = 0;
			mode = NONE;
			XClearWindow(d, win);
		} else if (e.type == ClientMessage) {
			Atom wmDel = XInternAtom(d, "WM_DELETE_WINDOW", False);
			if ((Atom)e.xclient.data.l[0] == wmDel) break;
		}
	}

	XFreeGC(d, gc);
	XDestroyWindow(d, win);
	XCloseDisplay(d);
	return 0;
}
