//
//  ParticleForcePairManager.cpp
//  GalileuEngine
//
//  Created by lrazevedo on 28/09/24.
//

#include "ParticleForcePairManager.hpp"

// STD library includes.
#include <algorithm>

namespace GE
{
namespace Physics
{

void FParticleForcePairManager::add(FParticle* particle, FParticleForceGenerator* particleForceGenerator)
{
    ParticleForcePairs.emplace_back(FParticleForcePair{particle, particleForceGenerator});
}

void FParticleForcePairManager::remove(FParticle* particle, FParticleForceGenerator* particleForceGenerator)
{
    auto isEqualPredicate = [=](FParticleForcePair& pair)
    {
        return (pair.Particle == particle) && (pair.ParticleForceGenerator == particleForceGenerator);
    };
    std::erase_if(ParticleForcePairs, isEqualPredicate);
}

void FParticleForcePairManager::clear()
{
    ParticleForcePairs.clear();
}

void FParticleForcePairManager::updateForces(FReal deltaTime)
{
    for (FParticleForcePair& pair : ParticleForcePairs)
    {
        pair.ParticleForceGenerator->updateForce(pair.Particle, deltaTime);
    }
}

}   // End of namespace Physics
}   // End of namespace GE
