// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2011 by Katherina Baber, Klaus Mosthaf                    *
 *   Copyright (C) 2008-2009 by Bernd Flemisch, Andreas Lauser               *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/

/*!
 * \file
 *
 * \brief Defines the indices required for the Stokes box model.
 */
#ifndef DUMUX_STOKES_INDICES_HH
#define DUMUX_STOKES_INDICES_HH

#include "stokesproperties.hh"

namespace Dumux
{
// \{

/*!
 * \ingroup BoxStokesModel
 * \ingroup BoxIndices
 * \brief The common indices for the isothermal stokes model.
 *
 * \tparam PVOffset The first index in a primary variable vector.
 */
template <class TypeTag, int PVOffset = 0>
class StokesCommonIndices
{
    // number of dimensions
    typedef typename GET_PROP_TYPE(TypeTag, Grid) Grid;
    static const int dim = Grid::dimension;

public:
    // Primary variable indices
    static const int momentum0Idx = PVOffset + 0; //!< Index of the first component of the momentum equation
    static const int massBalanceIdx = PVOffset + dim; //!< Index of the mass balance equation

    static const int velocity0Idx = PVOffset + 0; //!< Index of the first component of the velocity
    static const int pressureIdx = PVOffset + dim; //!< Index of the pressure in a solution vector
};
} // end namespace

#endif
