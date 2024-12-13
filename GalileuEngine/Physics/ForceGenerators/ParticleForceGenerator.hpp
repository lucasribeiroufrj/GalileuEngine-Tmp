//
//  ParticleForceGenerator.hpp
//  GalileuEngine
//
//  Created by lrazevedo on 28/09/24.
//

#pragma once

// GE includes.
#include "Math.hpp"
#include "Particle.hpp"

namespace GE
{
namespace Physics
{
using Math::FReal;

/**
 * A mechanism for applying force to an object. The instance is asked to add a force to one of more particles.
 */
class FParticleForceGenerator
{
public:
    /**
     * Overload this to implement calculate and update the force applied to a given particle during a deltaTime.
     *
     * @param particle The particle that it requesting the force.
     * @param deltaTime The integration time.
     */
    virtual void updateForce(FParticle* particle, FReal deltaTime) = 0;
    
    /**
     * Destructor.
     */
    virtual ~FParticleForceGenerator() {}
};

}   // End of namespace Physics
}   // End of namespace GE
