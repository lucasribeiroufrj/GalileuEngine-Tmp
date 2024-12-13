//
//  ParticleCable.cpp
//  GalileuEngine
//
//  Created by Weyl on 12/11/24.
//

#include "ParticleCable.hpp"

// GE includes.
#include "Vector3.hpp"
#include "ParticleContact.hpp"

// STD library includes.
#include <span>

namespace GE
{
namespace Physics
{

unsigned FParticleCable::addContactsImplementation(std::span<FParticleContact> particleContacts) const
{
    const FReal length = computeCurrentLength();
    
    // It is not overextended?
    if (length < MaxLength)
    {
        return 0;   // No contact has been (re)generated.
    }
    
    FParticleContact& contact = particleContacts[0];
    contact.Particles[0] = Particles[0];
    contact.Particles[1] = Particles[1];
    
    FVector3 normal = Particles[0]->getPosition() - Particles[0]->getPosition();
    normal.normalize();
    contact.ContactNormal = normal;
    
    contact.PenetrationDepth = length - MaxLength;
    contact.RestitutionCoefficient = RestitutionCoefficient;
    
    return 1;   // Only one contact has been (re)generated.
}

}   // End of namespace Physics
}   // End of namespace GE
