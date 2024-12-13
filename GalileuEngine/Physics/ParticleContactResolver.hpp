//
//  ParticleContactResolver.hpp
//  GalileuEngine
//
//  Created by Weyl on 12/11/24.
//

#pragma once

// GE includes.
#include "Math.hpp"
#include "Particle.hpp"
#include "ParticleContact.hpp"

// STD library includes.
#include <vector>

namespace GE
{
namespace Physics
{
using Math::FReal;

/**
 * This class is responsible for resolving particle contacts.
 */
class FParticleContactResolver
{
public:
    /**
     * The basic contructor.
     *
     * @param maxNumberOfIterations The maximum number of iteratons allowed while resolving contacts.
     */
    explicit FParticleContactResolver(unsigned maxNumberOfIterations);
    
    /**
     * Sets the maximum number of iteratons allowed while resolving contacts.
     *
     * @param maxNumberOfIterations The new value.
     */
    void setMaxNumberOfIterations(unsigned maxNumberOfIterations);
    
    /**
     * Handles a set of particle contacts to resolve both velocity and penetration.
     */
    void resolveContacts(std::vector<FParticleContact>& contacts, const FReal deltaTime);
    
protected:
    /** Stores the maximum number of iteratons allowed while resolving contacts. */
    unsigned MaxNumberOfIterations;
    
    /** Stores the actual number of iterations performed last time the solver has run. */
    unsigned UsedNumberOfIterations;

};

}   // End of namespace Physics
}   // End of namespace GE
