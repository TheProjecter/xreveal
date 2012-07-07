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

#include "skippy.h"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "key.h"

static int DIE_NOW = 0;

static int
time_in_millis (void)
{
    struct timeval  tp;

    gettimeofday (&tp, 0);
    return(tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

static dlist *
update_clients(MainWin *mw, dlist *clients, Bool *touched)
{
    dlist *stack, *iter;

    stack = dlist_first(wm_get_stack(mw->dpy));
    iter = clients = dlist_first(clients);

    if(touched)
	*touched = False;

    /* Terminate clients that are no longer managed */
    while(iter)
    {
	ClientWin *cw = (ClientWin *)iter->data;
    if(! dlist_find_data(stack, (void *)cw->window))
	{
	    dlist *tmp = iter->next;
	    clientwin_destroy((ClientWin *)iter->data, True);
	    clients = dlist_remove(iter);
	    iter = tmp;
	    if(touched)
		*touched = True;
	    continue;
	}
	clientwin_update(cw);
	iter = iter->next;
    }

    /* Add new clients */
    for(iter = dlist_first(stack); iter; iter = iter->next)
    {
	ClientWin *cw = (ClientWin*)dlist_find(clients, clientwin_cmp_func, iter->data);
	if(! cw && (Window)iter->data != mw->window)
	{
	    cw = clientwin_create(mw, (Window)iter->data);
	    if(! cw)
		continue;
	    clients = dlist_add(clients, cw);
	    clientwin_update(cw);
	    if(touched)
		*touched = True;
	}
    }

    dlist_free(stack);

    return clients;
}

static dlist *
do_layout(MainWin *mw, dlist *clients, Window focus, Window leader)
{
    CARD32 desktop = wm_get_current_desktop(mw->dpy);
    unsigned int width, height;
    float factor;
    int xoff, yoff;
    dlist *iter, *tmp;

    /* Update the client table, pick the ones we want and sort them */
    clients = update_clients(mw, clients, 0);

    if(mw->cod)
	dlist_free(mw->cod);

    tmp = dlist_first(dlist_find_all(clients, (dlist_match_func)clientwin_validate_func, &desktop));
    if(leader != None)
    {
	mw->cod = dlist_first(dlist_find_all(tmp, clientwin_check_group_leader_func, (void*)&leader));
	dlist_free(tmp);
    } else
	mw->cod = tmp;

    if(! mw->cod)
	return clients;

    dlist_sort(mw->cod, clientwin_sort_func, 0);

    /* Move the mini windows around */
    layout_run(mw, mw->cod, &width, &height);
    factor = (float)(mw->width - 100) / width;
    if(factor * height > mw->height - 100)
	factor = (float)(mw->height - 100) / height;

    xoff = (mw->width - (float)width * factor) / 2;
    yoff = (mw->height - (float)height * factor) / 2;
    mainwin_transform(mw, factor);
    for(iter = mw->cod; iter; iter = iter->next)
	clientwin_move((ClientWin*)iter->data, factor, xoff, yoff);

    /* Get the currently focused window and select which mini-window to focus */
    iter = dlist_find(mw->cod, clientwin_cmp_func, (void *)focus);
    if(! iter)
	iter = mw->cod;
    mw->focus = (ClientWin*)iter->data;
    mw->focus->focused = 1;

    /* Map the client windows */
    for(iter = mw->cod; iter; iter = iter->next)
	clientwin_map((ClientWin*)iter->data);
    XWarpPointer(mw->dpy, None, mw->focus->mini.window, 0, 0, 0, 0, mw->focus->mini.width / 2, mw->focus->mini.height / 2);

    return clients;
}

static dlist *
skippy_run(MainWin *mw, dlist *clients, Window focus, Window leader, Bool all_xin)
{
    XEvent ev;
    int die = 0;
    Bool refocus = False;
    int last_rendered;

    /* Update the main window's geometry (and Xinerama info if applicable) */
    mainwin_update(mw);
#ifdef XINERAMA
    if(all_xin)
	mw->xin_active = 0;
#else /* ! XINERAMA */
    if(all_xin);
#endif /* XINERAMA */

    /* Map the main window and run our event loop */
    if(mw->lazy_trans)
    {
	mainwin_map(mw);
	XFlush(mw->dpy);
    }

    clients = do_layout(mw, clients, focus, leader);
    if(! mw->cod)
	return clients;

    /* Map the main window and run our event loop */
    if(! mw->lazy_trans)
	mainwin_map(mw);

    last_rendered = time_in_millis();
    while(! die) {
	int i, j, now, timeout;
	int move_x = -1, move_y = -1;
	struct pollfd r_fd;

	XFlush(mw->dpy);

	r_fd.fd = ConnectionNumber(mw->dpy);
	r_fd.events = POLLIN;
	if(mw->poll_time > 0)
	timeout = MAX(0, mw->poll_time + last_rendered - time_in_millis());
	else
	timeout = -1;
	i = poll(&r_fd, 1, timeout);

	now = time_in_millis();
	if(now >= last_rendered + mw->poll_time)
	{
	REDUCE(if( ((ClientWin*)iter->data)->damaged ) clientwin_repair(iter->data), mw->cod);
	last_rendered = now;
	}

	i = XPending(mw->dpy);
	for(j = 0; j < i; ++j)
	{
	    XNextEvent(mw->dpy, &ev);

	    if (ev.type == MotionNotify)
	    {
		move_x = ev.xmotion.x_root;
		move_y = ev.xmotion.y_root;
	    }
	    else if(ev.type == DestroyNotify || ev.type == UnmapNotify) {
		dlist *iter = dlist_find(clients, clientwin_cmp_func, (void *)ev.xany.window);
		if(iter)
		{
		    ClientWin *cw = (ClientWin *)iter->data;
		    clients = dlist_first(dlist_remove(iter));
		    iter = dlist_find(mw->cod, clientwin_cmp_func, (void *)ev.xany.window);
		    if(iter)
			mw->cod = dlist_first(dlist_remove(iter));
		    clientwin_destroy(cw, True);
		    if(! mw->cod)
		    {
			die = 1;
			break;
		    }
		}
	    }
	    else if (mw->poll_time >= 0 && ev.type == mw->damage_event_base + XDamageNotify)
	    {
		XDamageNotifyEvent *d_ev = (XDamageNotifyEvent *)&ev;
		dlist *iter = dlist_find(mw->cod, clientwin_cmp_func, (void *)d_ev->drawable);
		if(iter)
		{
		    if(mw->poll_time == 0)
			clientwin_repair((ClientWin *)iter->data);
		    else
			((ClientWin *)iter->data)->damaged = True;
		}

	    }
	    else if(ev.type == KeyRelease && ev.xkey.keycode == mw->key_escape)
	    {
		refocus = True;
		die = 1;
		break;
	    }
	    else if(ev.xany.window == mw->window)
		die = mainwin_handle(mw, &ev);
	    else if(ev.type == PropertyNotify)
	    {
		if (ev.xproperty.atom == _NET_CURRENT_DESKTOP ||
		    ev.xproperty.atom == _WIN_WORKSPACE) {
		    die = 1;
		    break;
		} else if(ev.xproperty.atom == ESETROOT_PMAP_ID ||
		    ev.xproperty.atom == _XROOTPMAP_ID)
		{
		    mainwin_update_background(mw);
		    REDUCE(clientwin_render((ClientWin *)iter->data), mw->cod);
		}

	    }
	    else if(mw->tooltip && ev.xany.window == mw->tooltip->window)
		tooltip_handle(mw->tooltip, &ev);
	    else
	    {
		dlist *iter;
		for(iter = mw->cod; iter; iter = iter->next)
		{
		    ClientWin *cw = (ClientWin *)iter->data;
		    if(cw->mini.window == ev.xany.window)
		    {
			die = clientwin_handle(cw, &ev);
			if(die)
			    break;
		    }
		}
		if(die)
		    break;
	    }

	}

	if(mw->tooltip && move_x != -1)
	tooltip_move(mw->tooltip, move_x + 20, move_y + 20);

    }

    /* Unmap the main window and clean up */
    mainwin_unmap(mw);
    XFlush(mw->dpy);

    REDUCE(clientwin_unmap((ClientWin*)iter->data), mw->cod);
    dlist_free(mw->cod);
    mw->cod = 0;

    if(die == 2)
	DIE_NOW = 1;

    if(refocus)
	XSetInputFocus(mw->dpy, focus, RevertToPointerRoot, CurrentTime);

    return clients;
}

int
main(void)
{
    dlist *clients = 0, *config = 0;
    Display *dpy = XOpenDisplay(NULL);
    MainWin *mw;
    const char *tmp, *homedir;
    char cfgpath[8192];
    Bool all_xin = False;

    if(! dpy) {
	fprintf(stderr, "FATAL: Couldn't connect to display.\n");
	return -1;
    }

    wm_get_atoms(dpy);

    if(! wm_check(dpy)) {
	fprintf(stderr, "FATAL: WM not NETWM or GNOME WM Spec compliant.\n");
	return -1;
    }

    homedir = getpwuid(getuid())->pw_dir;
    snprintf(cfgpath, 8191, "%s/%s", homedir, ".skippy-xd.rc");
    config = config_load(cfgpath);

    wm_use_netwm_fullscreen(strcasecmp("true", config_get(config, "general", "useNETWMFullscreen", "true")) == 0);
    wm_ignore_skip_taskbar(strcasecmp("true", config_get(config, "general", "ignoreSkipTaskbar", "false")) == 0);

    mw = mainwin_create(dpy, config);
    if(! mw)
    {
	fprintf(stderr, "FATAL: Couldn't create main window.\n");
	config_free(config);
	XCloseDisplay(mw->dpy);
	return -1;
    }

    all_xin = strcasecmp("true", config_get(config, "xinerama", "showAll", "false")) == 0;

    key_init(dpy);
    key_def_t **keys = key_def_list_create(dpy,
	config_get(config, "general", "keys", "F11"));
    key_def_t **keys_wingrp = key_def_list_create(dpy,
	config_get(config, "general", "keys-wingrp", "F11"));
    if (keys == NULL || keys_wingrp == NULL) {
	fprintf(stderr, "FATAL: out of memory\n", tmp);
	config_free(config);
	key_def_list_free(keys);
	key_def_list_free(keys_wingrp);
	XCloseDisplay(mw->dpy);
	return -1;
    }
    key_def_list_grab(keys, mw->dpy, mw->root);
    key_def_list_grab(keys_wingrp, mw->dpy, mw->root);

    int x_input_mask = PropertyChangeMask;

    useconds_t mouse_delay = (useconds_t)(1000000 * strtod(
	config_get(config, "general", "mouse-delay", "0.5"), NULL));
    int mouse_corner_x = -1;
    int mouse_corner_y = -1;
    const char *mouse_corner = config_get(config, "general", "mouse-corner",
	NULL);
    if (mouse_corner != NULL) {
	x_input_mask |= PointerMotionMask;
	XWindowAttributes attr;
	if (XGetWindowAttributes(mw->dpy, mw->root, &attr) == 0) {
	    fprintf(stderr, "FATAL: could not query the root window\n");
	    config_free(config);
	    key_def_list_free(keys);
	    key_def_list_free(keys_wingrp);
	    XCloseDisplay(mw->dpy);
	}
	if (strcasecmp(mouse_corner, "NW") == 0) {
	    mouse_corner_x = 0;
	    mouse_corner_y = 0;
	} else if (strcasecmp(mouse_corner, "SW") == 0) {
	    mouse_corner_x = 0;
	    mouse_corner_y = attr.height - 1;
	} else if (strcasecmp(mouse_corner, "SE") == 0) {
	    mouse_corner_x = attr.width - 1;
	    mouse_corner_y = attr.height - 1;
	} else if (strcasecmp(mouse_corner, "NE") == 0) {
	    mouse_corner_x = attr.width - 1;
	    mouse_corner_y = 0;
	} else {
	    fprintf(stderr, "WARNING: invalid mouse corner: %s\n",
		mouse_corner);
	    x_input_mask &= (~PointerMotionMask);
	}
    }

    XSelectInput(mw->dpy, mw->root, x_input_mask);

    while(! DIE_NOW)
    {
	XEvent ev;
	XNextEvent(mw->dpy, &ev);
	switch (ev.type) {
	case PropertyNotify:
	    if (ev.xproperty.atom == ESETROOT_PMAP_ID ||
		ev.xproperty.atom == _XROOTPMAP_ID) {
		mainwin_update_background(mw);
	    }
	    break;

	case KeyRelease:
	    if (key_def_list_check(keys, &ev.xkey)) {
		Window focused = wm_get_focused(mw->dpy);
		clients = skippy_run(mw, clients, focused,
		    None, all_xin);
	    } else if (key_def_list_check(keys_wingrp, &ev.xkey)) {
		Window focused = wm_get_focused(mw->dpy);
		if(focused != None) {
		    Window leader = wm_get_group_leader(
			mw->dpy, focused);
		    if(leader) {
			clients = skippy_run(mw, clients,
			focused, leader, all_xin);
		    }
		}
	    }
	    break;

	default:
	    if (ev.xmotion.x == mouse_corner_x &&
		ev.xmotion.y == mouse_corner_y) {
		/*
		 * A more robust solution would use a separate thread to
		 * schedule a future run, but the delay in this case should be
		 * small enough to be mostly unnoticeable.
		 */
		usleep(mouse_delay);

		/*
		 * Remove any events that came in while we were sleeping, that
		 * could trigger a run
		 */
		while (XCheckMaskEvent(mw->dpy,
		    PointerMotionMask | KeyReleaseMask, &ev));

		/*
		 * If we're still in the designated mouse corner, launch the
		 * main window
		 */
		Window root;
		Window child;
		int root_x, root_y;
		int x, y;
		unsigned int mask;
		Bool status = XQueryPointer(mw->dpy, mw->root, &root, &child,
		    &root_x, &root_y, &x, &y, &mask);
		if (status == True && x == mouse_corner_x &&
		    y == mouse_corner_y) {
		    Window focused = wm_get_focused(mw->dpy);
		    clients = skippy_run(mw, clients, focused,
			None, all_xin);
		}
	    }
	    break;
	}
    }

    dlist_free_with_func(clients, (dlist_free_func)clientwin_destroy);

    key_def_list_free(keys);
    key_def_list_free(keys_wingrp);
    XFlush(mw->dpy);

    mainwin_destroy(mw);

    XSync(dpy, True);
    XCloseDisplay(dpy);
    config_free(config);

    return 0;
}
