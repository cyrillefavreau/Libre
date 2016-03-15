/* Copyright (c) 2011-2016, EPFL/Blue Brain Project
 *                     Ahmet Bilgili <ahmet.bilgili@epfl.ch>
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

#ifndef _Executor_h_
#define _Executor_h_

#include <livre/core/types.h>

namespace livre
{

/**
 * Executor class is the base class for implementing different scheduling
 * algorithms for submited work. i.e. The IO or CPU intensive algorithms
 * can implement their own scheduling algorithms.
 */
class Executor
{
public:

    virtual ~Executor();

    /**
     * Executes the executable. Returns the futures that can be queried for data.
     * @param pipeline to be executed.
     */
    Futures execute( const Executable& executable );

    /**
     * Executes the executable. Returns the futures that can be queried for data.
     * @param pipeline to be executed.
     */
    Futures execute( const Executables& executables );

protected:

    /**
     * Schedules the executables for execution. The deriving class should implement a
     * scheduling algorithm for the execution. ( i.e. there may be a work queue and
     * executables are selected from the work queue according to their pre-post
     * conditions
     * @param executables are the executables to schedule.
     */
    virtual void _schedule( const Executables& executables ) = 0;

    /**
     * Clears the executor ( i.e : Implementation can empty the work queue )
     */
    virtual void clear() {}

};

}

#endif // _Executor_h_
