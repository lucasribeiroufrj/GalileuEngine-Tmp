//
//  ParticleRod.cpp
//  GalileuEngine
//
//  Created by Weyl on 12/11/24.
//

#include "ParticleRod.hpp"

// GE includes.
#include "Math.hpp"
#include "Vector3.hpp"
#include "ParticleContact.hpp"

// STD library includes.
#include <span>

namespace GE
{
namespace Physics
{

unsigned FParticleRod::addContactsImplementation(std::span<FParticleContact> particleContacts) const
{
    const FReal currentLength = computeCurrentLength();
    
    // It is not overextended?
    if (Math::abs(Length - currentLength) < Math::Kinda_Small_number)
    {
        return 0;   // No contact has been (re)generated.
    }
    
    FParticleContact& contact = particleContacts[0];
    contact.Particles[0] = Particles[0];
    contact.Particles[1] = Particles[1];
    
    FVector3 normal = Particles[0]->getPosition() - Particles[0]->getPosition();
    normal.normalize();
    
    if (currentLength > Length)
    {
        contact.ContactNormal = normal;
        contact.PenetrationDepth = currentLength - Length;
    }
    else
    {
        contact.ContactNormal = normal * -1;
        contact.PenetrationDepth = Length - currentLength;
    }
    
    // No bounciness.
    contact.RestitutionCoefficient = 0;
    
    return 1;   // Only one contact has been (re)generated.
}

}   // End of namespace Physics
}   // End of namespace GE
