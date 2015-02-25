// Filename: StaggeredStokesSolver.cpp
// Created on 16 Aug 2012 by Boyce Griffith
//
// Copyright (c) 2002-2014, Boyce Griffith
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of The University of North Carolina nor the names of
//      its contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/////////////////////////////// INCLUDES /////////////////////////////////////

#include <stddef.h>
#include <ostream>
#include <vector>

#include "SAMRAI/hier/IntVector.h"
#include "SAMRAI/solv/LocationIndexRobinBcCoefs.h"
#include "SAMRAI/solv/PoissonSpecifications.h"
#include "SAMRAI/solv/RobinBcCoefStrategy.h"
#include "ibamr/StaggeredStokesPhysicalBoundaryHelper.h"
#include "ibamr/StaggeredStokesSolver.h"
#include "ibamr/ibamr_utilities.h"
#include "ibamr/namespaces.h" // IWYU pragma: keep
#include "SAMRAI/tbox/Database.h"

#include "SAMRAI/tbox/Utilities.h"

/////////////////////////////// NAMESPACE ////////////////////////////////////

namespace IBAMR
{
/////////////////////////////// STATIC ///////////////////////////////////////

/////////////////////////////// PUBLIC ///////////////////////////////////////

StaggeredStokesSolver::StaggeredStokesSolver()
    : d_U_problem_coefs("U_problem_coefs"),
      d_default_U_bc_coef(new LocationIndexRobinBcCoefs(DIM, "default_U_bc_coef", boost::shared_ptr<Database>())),
      d_U_bc_coefs(std::vector<RobinBcCoefStrategy*>(NDIM, d_default_U_bc_coef)),
      d_default_P_bc_coef(new LocationIndexRobinBcCoefs(DIM, "default_P_bc_coef", boost::shared_ptr<Database>())),
      d_P_bc_coef(d_default_P_bc_coef)
{
    // Setup a default boundary condition object that specifies homogeneous
    // Dirichlet boundary conditions for the velocity and homogeneous Neumann
    // boundary conditions for the pressure.
    for (unsigned int d = 0; d < NDIM; ++d)
    {
        auto p_default_U_bc_coef = CPP_CAST<LocationIndexRobinBcCoefs*>(d_default_U_bc_coef);
        TBOX_ASSERT(p_default_U_bc_coef);
        p_default_U_bc_coef->setBoundaryValue(2 * d, 0.0);
        p_default_U_bc_coef->setBoundaryValue(2 * d + 1, 0.0);
        auto p_default_P_bc_coef = CPP_CAST<LocationIndexRobinBcCoefs*>(d_default_P_bc_coef);
        TBOX_ASSERT(p_default_P_bc_coef);
        p_default_P_bc_coef->setBoundarySlope(2 * d, 0.0);
        p_default_P_bc_coef->setBoundarySlope(2 * d + 1, 0.0);
    }

    // Initialize the boundary conditions objects.
    setPhysicalBcCoefs(std::vector<RobinBcCoefStrategy*>(NDIM, d_default_U_bc_coef), d_default_P_bc_coef);
    return;
} // StaggeredStokesSolver()

StaggeredStokesSolver::~StaggeredStokesSolver()
{
    delete d_default_U_bc_coef;
    d_default_U_bc_coef = NULL;
    delete d_default_P_bc_coef;
    d_default_P_bc_coef = NULL;
    return;
} // ~StaggeredStokesSolver()

void StaggeredStokesSolver::setVelocityPoissonSpecifications(const PoissonSpecifications& U_problem_coefs)
{
    d_U_problem_coefs = U_problem_coefs;
    return;
} // setVelocityPoissonSpecifications

void StaggeredStokesSolver::setPhysicalBcCoefs(const std::vector<RobinBcCoefStrategy*>& U_bc_coefs,
                                               RobinBcCoefStrategy* P_bc_coef)
{
    TBOX_ASSERT(U_bc_coefs.size() == NDIM);
    for (unsigned int d = 0; d < NDIM; ++d)
    {
        if (U_bc_coefs[d])
        {
            d_U_bc_coefs[d] = U_bc_coefs[d];
        }
        else
        {
            d_U_bc_coefs[d] = d_default_U_bc_coef;
        }
    }

    if (P_bc_coef)
    {
        d_P_bc_coef = P_bc_coef;
    }
    else
    {
        d_P_bc_coef = d_default_P_bc_coef;
    }
    return;
} // setPhysicalBcCoefs

void
StaggeredStokesSolver::setPhysicalBoundaryHelper(boost::shared_ptr<StaggeredStokesPhysicalBoundaryHelper> bc_helper)
{
    TBOX_ASSERT(bc_helper);
    d_bc_helper = bc_helper;
    return;
} // setPhysicalBoundaryHelper

/////////////////////////////// PRIVATE //////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////
