/*****************************************************************************
 *   Copyright (C) 2011 by Andreas Lauser                                    *
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
 * \brief Represents the primary variables used in the M-phase,
 *        N-component box model.
 *
 * This class is basically a Dune::FieldVector which can retrieve its
 * contents from an aribitatry fluid state.
 */
#ifndef DUMUX_MPNC_PRIMARY_VARIABLES_HH
#define DUMUX_MPNC_PRIMARY_VARIABLES_HH

#include <dune/common/fvector.hh>

#include <dumux/material/constraintsolvers/ncpflash.hh>
#include "energy/mpncvolumevariablesenergy.hh"

namespace Dumux
{
/*!
 * \ingroup MPNCModel
 *
 * \brief Represents the primary variables used in the M-phase,
 *        N-component box model.
 *
 * This class is basically a Dune::FieldVector which can retrieve its
 * contents from an aribitatry fluid state.
 */
template <class TypeTag>
class MPNCPrimaryVariables 
    : public Dune::FieldVector<typename GET_PROP_TYPE(TypeTag, Scalar),
                               GET_PROP_VALUE(TypeTag, NumEq) >
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    enum { numEq = GET_PROP_VALUE(TypeTag, NumEq) };
    typedef Dune::FieldVector<Scalar, numEq> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

    typedef typename GET_PROP_TYPE(TypeTag, MPNCIndices) Indices;
    enum { p0Idx = Indices::p0Idx };
    enum { S0Idx = Indices::S0Idx };
    enum { fug0Idx = Indices::fug0Idx };

    enum { numPhases = GET_PROP_VALUE(TypeTag, NumPhases) };
    enum { numComponents = GET_PROP_VALUE(TypeTag, NumComponents) };
    typedef Dune::FieldVector<Scalar, numComponents> ComponentVector;

    enum { enableEnergy = GET_PROP_VALUE(TypeTag, EnableEnergy) };
    enum { enableKineticEnergy = GET_PROP_VALUE(TypeTag, EnableKineticEnergy) };
    typedef MPNCVolumeVariablesEnergy<TypeTag, enableEnergy, enableKineticEnergy> EnergyModule;

    typedef Dumux::NcpFlash<Scalar, FluidSystem> NcpFlash;

public:
    /*!
     * \brief Default constructor
     */
    MPNCPrimaryVariables()
        : ParentType()
    { };

    /*!
     * \brief Constructor with assignment from scalar
     */
    MPNCPrimaryVariables(Scalar value)
        : ParentType(value)
    { };

    /*!
     * \brief Copy constructor
     */
    MPNCPrimaryVariables(const MPNCPrimaryVariables &value)
        : ParentType(value)
    { };

    /*!
     * \brief Set the primary variables from an arbitrary fluid state
     *        in a mass conservative way.
     *
     * If an energy equation is included, the fluid temperatures are
     * the same as the one given in the fluid state, *not* the
     * enthalpy.
     *
     * \param fluidState The fluid state which should be represented
     *                   by the primary variables. The temperatures,
     *                   pressures, compositions and densities of all
     *                   phases must be defined.
     * \param matParams The capillary pressure law parameters
     * \param isInEquilibrium If true, the fluid state expresses
     *                        thermodynamic equilibrium assuming the
     *                        relations expressed by the fluid
     *                        system. This implies that in addition to
     *                        the quantities mentioned above, the
     *                        fugacities are also defined.
     */
    template <class MaterialLaw, class FluidState>
    void assignMassConservative(const FluidState &fluidState,
                                const typename MaterialLaw::Params &matParams,
                                bool isInEquilibrium = false)
    {
#ifndef NDEBUG
        // make sure the temperature is the same in all fluid phases
        for (int phaseIdx = 1; phaseIdx < numPhases; ++phaseIdx) {
            assert(fluidState.temperature(0) == fluidState.temperature(phaseIdx));
        }
#endif // NDEBUG

        // for the equilibrium case, we don't need complicated
        // computations.
        if (isInEquilibrium) {
            assignNaive_(fluidState);
            return;
        }

        // use a flash calculation to calculate a fluid state in
        // thermodynamic equilibrium
        typename FluidSystem::ParameterCache paramCache;
        Dumux::CompositionalFluidState<Scalar, FluidSystem> fsFlash;
             
        // calculate the "global molarities" 
        ComponentVector globalMolarities(0.0);
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
                globalMolarities[compIdx] +=
                    fluidState.saturation(phaseIdx)*fluidState.molarity(phaseIdx, compIdx);
            }
        }

        // use the externally given fluid state as initial value for
        // the flash calculation
        fsFlash.assign(fluidState);
        //NcpFlash::guessInitial(fsFlash, paramCache, globalMolarities);       

        // run the flash calculation
        NcpFlash::template solve<MaterialLaw>(fsFlash, paramCache, matParams, globalMolarities);
        
        // use the result to assign the primary variables
        assignNaive_(fsFlash);
    };

protected:
    template <class FluidState>
    void assignNaive_(const FluidState &fluidState)
    {
        // assign the phase temperatures. this is out-sourced to
        // the energy module
        EnergyModule::setPriVarTemperatures(*this, fluidState);
        
        // assign fugacities
        for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            (*this)[fug0Idx + compIdx] =  fluidState.fugacity(/*phaseIdx=*/0, compIdx);
        
        // assign pressure
        (*this)[p0Idx] = fluidState.pressure(/*phaseIdx=*/0);
        
        // assign first M - 1 saturations
        for (int phaseIdx = 0; phaseIdx < numPhases - 1; ++phaseIdx)
            (*this)[S0Idx + phaseIdx] = fluidState.saturation(phaseIdx);
    }
 };

} // end namepace

#endif