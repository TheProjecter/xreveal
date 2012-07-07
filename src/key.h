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

#ifndef XREVEAL_KEY_H
#define XREVEAL_KEY_H

typedef struct key_def {
    KeyCode keycode;
    unsigned int modifiers;
} key_def_t;

int key_def_list_check(key_def_t **keys, XKeyEvent *event);
key_def_t **key_def_list_create(Display *display, const char *line);
void key_def_list_free(key_def_t **keys);
void key_def_list_grab(key_def_t **keys, Display *display, Window grab_window);
void key_init(Display *dpy);

#endif /* XREVEAL_KEY_H */
