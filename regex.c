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
#include "regex.h"

char *
re_match_copy(const char *line, regmatch_t *match)
{
	char *r;
	r = (char *)malloc(match->rm_eo + 1);
	strncpy(r, line + match->rm_so, match->rm_eo - match->rm_so);
	r[match->rm_eo - match->rm_so] = 0;
	return r;
}

int
re_match_check(regmatch_t *match)
{
	return match->rm_so != -1;
}

regoff_t
re_match_len(regmatch_t *match)
{
	return match->rm_eo - match->rm_so;
}
