//
//  ParticleForcePairManager.hpp
//  GalileuEngine
//
//  Created by lrazevedo on 28/09/24.
//

#pragma once

// GE includes.
#include "Math.hpp"
#include "Vector3.hpp"
#include "Particle.hpp"
#include "ParticleForceGenerator.hpp"

// STD library includes.
#include <vector>

namespace GE
{
namespace Physics
{
using Math::FVector3;
using Math::FReal;

/**
 * Stores all the force generators and the particles they act upon.
 */
class FParticleForcePairManager
{
public:
    /**
     * Registers a particle-force pair.
     */
    void add(FParticle* particle, FParticleForceGenerator* particleForceGenerator);
    
    /**
     * Tries to unresgister a particle-force pair.
     * No particle, or force, will be deleted, only the relation between them will be deleted.
     * There will be no effect in case the pair is not registered.
     */
    void remove(FParticle* particle, FParticleForceGenerator* particleForceGenerator);
    
    /**
     * Deletes all particle-force pairs at once.
     * No particle, or force, will be deleted, only the relation between them will be deleted.
     */
    void clear();
    
    /**
     * Requests all force generators to update the forces acting on their respective particles.
     *
     * @param deltaTime The integration time.
     */
    void updateForces(FReal deltaTime);
    
private:
    /**
     * Keeps track of the force generator and the particle it applies to.
     */
    struct FParticleForcePair
    {
        FParticle* Particle;
        FParticleForceGenerator* ParticleForceGenerator;
    };
    
    /**
     * Stores the particle-force pairs.
     */
    std::vector<FParticleForcePair> ParticleForcePairs;
};

}   // End of namespace Physics
}   // End of namespace GE
