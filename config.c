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
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include "regex.h"

typedef struct
{
	char *section, *key, *value;
} ConfigEntry;

static ConfigEntry *
entry_new(const char *section, char *key, char *value)
{
	ConfigEntry *e = (ConfigEntry *)malloc(sizeof(ConfigEntry));
	e->section = strdup(section);
	e->key = key;
	e->value = value;
	return e;
}

static dlist *
entry_set(dlist *config, const char *section, char *key, char *value)
{
	dlist *iter = dlist_first(config);
	ConfigEntry *entry;
	for(; iter; iter = iter->next)
	{
		entry = (ConfigEntry *)iter->data;
		if(! strcasecmp(entry->section, section) && ! strcasecmp(entry->key, key)) {
			free(key);
			free(entry->value);
			entry->value = value;
			return config;
		}
	}
	entry = entry_new(section, key, value);
	return dlist_add(config, entry);
}

dlist *
config_load(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "WARNING: couldn't load %s: %s\n", path,
            strerror(errno));
        return 0;
    }

    regex_t re_section, re_empty, re_entry;
    regcomp(&re_section, "^[[:space:]]*\\[[[:space:]]*([[:alnum:]]*?)"
        "[[:space:]]*\\][[:space:]]*$", REG_EXTENDED);
    regcomp(&re_empty, "^[[:space:]]*(#|$)", REG_EXTENDED);
    regcomp(&re_entry, "^[[:space:]]*([-_[:alnum:]]+)[[:space:]]*="
        "[[:space:]]*(.*?)[[:space:]]*$", REG_EXTENDED);
	
    dlist *config = NULL;
    char *section = NULL;
	
    char line[8192];
    int len = 0;
    int eof = 0;
    while (!eof) {
        int eol = 0;
        ssize_t n = read(fd, line + len, 1);
        switch (n) {
        case -1:
            fprintf(stderr, "WARNING: couldn't read %s: %s\n", path,
                strerror(errno));
            eof = 1;
            break;

        case 0:
            eol = 1;
            eof = 1;
            break;

        default:
            assert (n == 1);
            switch (line[len]) {
            case '\n':
            case '\r':
            case '\0':
                eol = 1;
                break;
            default:
                len += n;
                if (len == COUNT(line) - 1) {
                    eol = 1;
                }
            }
        }

        if (eol && len != 0) {
            line[len] = '\0';
            len = 0;

            regmatch_t matches[5];
            if (regexec(&re_empty, line, COUNT(matches), matches, 0) == 0) {
		/* do nothing */
            } else if (regexec(&re_section, line, COUNT(matches), matches, 0)
                == 0) {
		free(section);
		section = re_match_copy(line, &matches[1]);
            } else if (section && regexec(&re_entry, line, COUNT(matches),
                matches, 0) == 0) {

                char *key = re_match_copy(line, &matches[1]);
                char *value = re_match_copy(line, &matches[2]);
                config = entry_set(config, section, key, value);
	    } else  {
                fprintf(stderr, "WARNING: %s: invalid line: %s\n", path, line);
	    }
	}
    }
	
    close(fd);
    free(section);
    
    regfree(&re_section);
    regfree(&re_empty);
    regfree(&re_entry);
    
    return config;
}

static void
entry_free(ConfigEntry *entry)
{
	free(entry->section);
	free(entry->key);
	free(entry->value);
	free(entry);
}

void
config_free(dlist *config)
{
	dlist_free_with_func(config, (dlist_free_func)entry_free);
}

static int
entry_find_func(dlist *l, ConfigEntry *key)
{
	ConfigEntry *entry = (ConfigEntry*)l->data;
	return ! (strcasecmp(entry->section, key->section) || strcasecmp(entry->key, key->key));
}

const char *
config_get(dlist *config, const char *section, const char *key, const char *def)
{
	ConfigEntry needle;
	dlist *iter;
	
	needle.section = (char *)section;
	needle.key = (char *)key;
	
	iter = dlist_find(dlist_first(config), (dlist_match_func)entry_find_func, &needle);
	
	if(! iter)
		return def;
	
	return ((ConfigEntry*)iter->data)->value;
}
