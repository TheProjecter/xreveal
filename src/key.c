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
#include "key.h"
#include "regex.h"

/*
 * Static data
 */

static int SHIFT_MASK = ShiftMask;
static int CTRL_MASK = ControlMask;
static int ALT_MASK = 0;
static int META_MASK = 0;
static int HYPER_MASK = 0;
static int SUPER_MASK = 0;
static int CAPSLOCK_MASK = LockMask;
static int NUMLOCK_MASK = 0;
static int SCROLLLOCK_MASK = 0;

/* length is 2 ^ X + 1, where X is the number of LOCK masks */
static int LOCK_MASKS[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1
};

/*
 * Static functions
 */

static void
add_to_lock_masks(int mask) {
    int i;
    for (i = 0; LOCK_MASKS[i] != -1; i++) {
        if (LOCK_MASKS[i] == mask) {
	    return;
	}
    }
    LOCK_MASKS[i] = mask;
}

static void
key_set_modifier_masks(Display *display) {
    int min_code, max_code;

#if defined (XlibSpecificationRelease) && XlibSpecificationRelease >= 4
    XDisplayKeycodes(display, &min_code, &max_code);
#else
    min_code = display->min_keycode;
    max_code = display->max_keycode;
#endif

    META_MASK = ALT_MASK = HYPER_MASK = SUPER_MASK = NUMLOCK_MASK =
	SCROLLLOCK_MASK = 0;

    int syms_per_code;
    KeySym *syms = XGetKeyboardMapping(display, min_code,
	max_code - min_code + 1, &syms_per_code);
    XModifierKeymap *mods = XGetModifierMapping(display);

    for (int row = 3; row < 8; row++) {
	for (int col = 0; col < mods->max_keypermod; col++) {
	    KeyCode code = mods->modifiermap[(row * mods->max_keypermod) + col];
	    if (code == 0) {
		continue;
	    }
	    for (int code_col = 0; code_col < syms_per_code; code_col++) {
		int sym = syms[((code - min_code) * syms_per_code) + code_col];
		switch(sym) {
		case XK_Meta_L:
		case XK_Meta_R:
		    META_MASK = 1 << row;
		    break;

		case XK_Alt_L: case XK_Alt_R:
		    ALT_MASK = 1 << row;
		    break;

		case XK_Hyper_L: case XK_Hyper_R:
		    HYPER_MASK = 1 << row;
		    break;

		case XK_Super_L: case XK_Super_R:
		    SUPER_MASK = 1 << row;
		    break;

		case XK_Num_Lock:
		    NUMLOCK_MASK = 1 << row;
		    break;

		case XK_Scroll_Lock:
		    SCROLLLOCK_MASK = 1 << row;
		    break;
		}
	    }
	}
    }

    XFree((char *)syms);
    XFreeModifiermap(mods);

    /*
     * XGrabKey requires modifiers to be specified exactly.  So when grabbing a
     * key, we'll need to specify all different combinations of the "lock"
     * modifiers.
     */
    add_to_lock_masks(0 | 0 | 0);
    add_to_lock_masks(0 | 0 | SCROLLLOCK_MASK);
    add_to_lock_masks(0 | NUMLOCK_MASK | 0);
    add_to_lock_masks(0 | NUMLOCK_MASK | SCROLLLOCK_MASK);
    add_to_lock_masks(CAPSLOCK_MASK | 0 | 0);
    add_to_lock_masks(CAPSLOCK_MASK | 0 | SCROLLLOCK_MASK);
    add_to_lock_masks(CAPSLOCK_MASK | NUMLOCK_MASK | 0);
    add_to_lock_masks(CAPSLOCK_MASK | NUMLOCK_MASK | SCROLLLOCK_MASK);
}

/*
 * Extern functions
 */

#define	REGEX_KEY_MODPREFIX "((any)|(shift)|(ctrl)|(alt)|(meta))-"
#define	REGEX_KEY_LIST "^[[:space:],]*((" REGEX_KEY_MODPREFIX \
    ")*)([^[:space:],]+)(.*)"
#define	REGEX_KEY_MOD "^(" REGEX_KEY_MODPREFIX ")(.*)"

int
key_def_list_check(key_def_t **keys, XKeyEvent *event) {
    for (int i = 0; keys[i] != NULL; i++) {
        if (event->keycode == keys[i]->keycode) {
	    if (keys[i]->modifiers & AnyModifier) {
		return 1;
	    }
	    for (int j = 0; LOCK_MASKS[j] != -1; j++) {
		if ((keys[i]->modifiers | LOCK_MASKS[j]) == event->state) {
		    return 1;
		}
	    }
	}
    }
    return 0;
}

key_def_t **
key_def_list_create(Display *display, const char *line) {
    regex_t re_list;
    regcomp(&re_list, REGEX_KEY_LIST, REG_EXTENDED | REG_ICASE);
    regmatch_t list_match[11];

    regex_t re_mod;
    regcomp(&re_mod, REGEX_KEY_MOD, REG_EXTENDED | REG_ICASE);
    regmatch_t mod_match[11];

    int n = 1;
    key_def_t **list = malloc(sizeof(key_def_t *));
    if (list == NULL) {
	return NULL;
    }
    list[n] = NULL;

    while (regexec(&re_list, line, COUNT(list_match), list_match, 0) == 0) {
	char *symstr = re_match_copy(line, &list_match[9]);
	if (symstr == NULL) {
	    key_def_list_free(list);
	    return NULL;
	}
	KeySym keysym = XStringToKeysym(symstr);
	if (keysym == NoSymbol) {
	    fprintf(stderr, "WARNING: unknown keysym: %s\n", symstr);
	} else {
	    KeyCode keycode = XKeysymToKeycode(display, keysym);
	    if (keycode == 0) {
		fprintf(stderr, "WARNING: no keycode for keysym: %s\n", symstr);
	    } else {
		key_def_t **old_list = list;
		list = realloc(list, ++n * sizeof(key_def_t *));
		if (list == NULL) {
		    free(symstr);
		    key_def_list_free(old_list);
		    return NULL;
		}

		list[n - 1] = NULL;
		key_def_t *key = calloc(1, sizeof(key_def_t));
		if (key == NULL) {
		    free(symstr);
		    key_def_list_free(list);
		    return NULL;
		}
		list[n - 2] = key;
		key->keycode = keycode;
		key->modifiers = 0;

		if (re_match_len(&list_match[1]) > 0) {
		    char *modstr = re_match_copy(line, &list_match[1]);
		    if (modstr == NULL) {
			free(symstr);
			key_def_list_free(list);
			return NULL;
		    }

		    const char *mline = (const char *)modstr;
		    while (regexec(&re_mod, mline, COUNT(mod_match), mod_match,
			0) == 0) {
			if (re_match_check(&mod_match[3])) {
			    key->modifiers = AnyModifier;
			    break;
			} else if (re_match_check(&mod_match[4])) {
			    key->modifiers |= SHIFT_MASK;
			} else if (re_match_check(&mod_match[5])) {
			    key->modifiers |= CTRL_MASK;
			} else if (re_match_check(&mod_match[6])) {
			    key->modifiers |= ALT_MASK;
			} else if (re_match_check(&mod_match[7])) {
			    key->modifiers |= META_MASK;
			}
			mline = mod_match[8].rm_sp;
		    }
		    free(modstr);
		}
	    }
	}

	free(symstr);

	line = list_match[10].rm_sp;
    }

    regfree(&re_list);
    regfree(&re_mod);

    return (list);
}

void
key_def_list_free(key_def_t **keys) {
    if (keys != NULL) {
	for (int i = 0; keys[i] != NULL; i++) {
	    free(keys[i]);
	}
	free(keys);
    }
}

void
key_def_list_grab(key_def_t **keys, Display *display, Window grab_window) {
    for (int i = 0; keys[i] != NULL; i++) {
	if (keys[i]->modifiers & AnyModifier) {
	    XGrabKey(display, keys[i]->keycode, AnyModifier, grab_window, False,
		GrabModeAsync, GrabModeAsync);
	} else {
	    for (int j = 0; LOCK_MASKS[j] != -1; j++) {
		int modmask = keys[i]->modifiers | LOCK_MASKS[j];
		XGrabKey(display, keys[i]->keycode,
		    keys[i]->modifiers | LOCK_MASKS[j], grab_window, False,
		    GrabModeAsync, GrabModeAsync);
	    }
	}
    }
}

void
key_init(Display *dpy) {
    key_set_modifier_masks(dpy);
}
