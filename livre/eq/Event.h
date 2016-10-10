
/* Copyright (c) 2011-2015, EPFL/Blue Brain Project
 *                          Stefan Eilemann <Stefan.Eilemann@epfl.ch>
 *
 * This file is part of Livre <https://github.com/BlueBrain/Livre>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIVRE_EVENT_H
#define LIVRE_EVENT_H

#include <eq/eq.h>

namespace livre
{

enum ConfigEventType
{
    GRAB_IMAGE = eq::EVENT_USER,
    VOLUME_INFO,
    REDRAW,
    HISTOGRAM_DATA
};

}

#endif
