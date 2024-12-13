//
//  ParticleSpringGenerator.hpp
//  GalileuEngine
//
//  Created by lrazevedo on 06/10/24.
//

#pragma once

// GE includes.
#include "ParticleForceGenerator.hpp"

// STD library includes.
#include <stdio.h>
#include <memory>

namespace GE
{
namespace Physics
{
using Math::FReal;

/**
 * A force generator that applies a spring force. It should not be used for stiff springs.
 * The spring is attached to the particle passed to the constructor, but the spring force is only applied to the particle passed to the updateForce() method. So the distance between the particles is defined as the current spring length, then Hook’s law is used to calculate the spring force.
 */
class FParticleSpringGenerator : public FParticleForceGenerator
{
public:
    /**
     * Creates a new spring with the given parameters.
     *
     * @param particle The particle the spring is connect to.
     * @param springConstant The spring constant used in the Hook's law.
     * @param restLength The rest legnth of the spring.
     */
    FParticleSpringGenerator(std::shared_ptr<FParticle> particle, FReal springConstant, FReal restLength);
    
    /**
     * Calculates and updates the spring force applied to a given particle during a deltaTime.
     *
     * @param particle The particle that is requesting the spring force.
     * @param deltaTime The integration time.
     */
    void updateForce(FParticle* particle, FReal deltaTime) override;
private:
    /**
     * The particle at the opposite end of the spring.
     */
    std::shared_ptr<FParticle> OtherParticle;
    
    /**
     * Stores the spring constant.
     */
    FReal SpringConstant;
    
    /**
     * Stores the rest legnth of the spring.
     */
    FReal RestLength;
};

}   // End of namespace Physics
}   // End of namespace GE
