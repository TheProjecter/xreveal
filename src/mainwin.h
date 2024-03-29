/*
 * Copyright (C) 2004 Hyriand <hyriand@thegraveyard.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef XREVEAL_MAINWIN_H
#define XREVEAL_MAINWIN_H

struct _Tooltip;

struct _MainWin
{
	Display *dpy;
	int screen;
	Visual *visual;
	Colormap colormap;
	int depth;
	Window root;
	int damage_event_base;
	
	int poll_time;
	Bool lazy_trans;
	
	Window window;
	Picture background;
	Pixmap bg_pixmap;
	int x, y;
	unsigned int width, height, distance;
	XRenderPictFormat *format;
	XTransform transform;
	
	XRenderColor normalTint, highlightTint;
	Pixmap normalPixmap, highlightPixmap;
	Picture normalPicture, highlightPicture;
	
	ClientWin *pressed, *focus;
	dlist *cod;
	struct _Tooltip *tooltip;
	
	KeyCode key_up, key_down, key_left, key_right, key_enter, key_space, key_escape;
	
#ifdef XINERAMA
	int xin_screens;
	XineramaScreenInfo *xin_info, *xin_active;
#endif /* XINERAMA */
};
typedef struct _MainWin MainWin;

MainWin *mainwin_create(Display *, dlist *config);
void mainwin_destroy(MainWin *);
void mainwin_map(MainWin *);
void mainwin_unmap(MainWin *);
int mainwin_handle(MainWin *, XEvent *);
void mainwin_update_background(MainWin *mw);
void mainwin_update(MainWin *mw);
void mainwin_transform(MainWin *mw, float f);

#endif /* XREVEAL_MAINWIN_H */
