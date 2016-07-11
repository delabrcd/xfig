/*
 *  pathmax.h
 *  This file is part of FIG : Facility for Interactive Generation of figures
 *
 *  Copyright (c) 2016 by Thomas Loimer
 *
 *  This file is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef	PATH_MAX

#ifdef	HAVE_LIMITS_H
#include <limits.h>
#else
#define	PATH_MAX	1024
#endif

#endif
