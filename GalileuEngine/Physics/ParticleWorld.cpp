//
//  ParticleWorld.cpp
//  GalileuEngine
//
//  Created by Weyl on 12/11/24.
//

#include "ParticleWorld.hpp"

// GE includes.
#include "Math.hpp"
#include "UtilMacros.hpp"
#include "Particle.hpp"

// STD library includes.
#include <optional>
#include <iostream>

namespace GE
{
namespace Physics
{

FParticleWorld::FParticleWorld(unsigned maxNumberOfContacts, std::optional<unsigned> numberOfIterations)
    :
    isContactResolverIterationsCalculated{ !numberOfIterations.has_value() },
    ParticleContactResolver{ numberOfIterations.has_value()? *numberOfIterations : 0 },
    ParticleContacts{maxNumberOfContacts}
{
}

void FParticleWorld::startFrame()
{
    for (FParticle* particle : Particles)
    {
        particle->clearAccumulatedForces();
    }
}

unsigned FParticleWorld::generateContacts()
{
    unsigned numberOfAvailableContacts = static_cast<unsigned>(ParticleContacts.size());
    unsigned firstContactIndex = 0;
    for (FParticleContactGenerator* generator : ParticleContactGenerators)
    {
        const std::span<FParticleContact> availableParticleContacts{ ParticleContacts.begin() + firstContactIndex, numberOfAvailableContacts };
        const unsigned numberOfContactsUsed = generator->addContacts(availableParticleContacts);
        numberOfAvailableContacts -= numberOfContactsUsed;
        firstContactIndex += numberOfContactsUsed;
        
        if (numberOfAvailableContacts <= 0)
        {
#if GE_BUILD_DEBUG
            std::cout << "Some contacts might be missing because the world has run out of contacts to fill." << std::endl;
#endif
            break;
        }
    }
    
    const unsigned totalNumberOfContactsUsed = static_cast<unsigned>(ParticleContacts.size()) - numberOfAvailableContacts;
    return totalNumberOfContactsUsed;
}

void FParticleWorld::integrate(FReal deltaTime)
{
    for (FParticle* particle : Particles)
    {
        particle->integrate(deltaTime);
    }
}

void FParticleWorld::runPhysics(FReal deltaTime)
{
    ParticleForcePairManager.updateForces(deltaTime);
    integrate(deltaTime);
    
    if (unsigned totalNumberOfContactsUsed = generateContacts())
    {
        if (isContactResolverIterationsCalculated)
        {
            ParticleContactResolver.setMaxNumberOfIterations(totalNumberOfContactsUsed * 2);
        }
        
        ParticleContactResolver.resolveContacts(ParticleContacts, deltaTime);
    }
}

}   // End of namespace Physics
}   // End of namespace GE
