//
//  ParticleContactGenerator.cpp
//  GalileuEngine
//
//  Created by Weyl on 12/11/24.
//

#include "ParticleContactGenerator.hpp"

// GE includes.
#include "Math.hpp"
#include "ParticleContact.hpp"

namespace GE
{
namespace Physics
{

unsigned FParticleContactGenerator::addContacts(std::span<FParticleContact> particleContacts) const
{
    CHECK(particleContacts.size() > 0)
    
    return addContactsImplementation(particleContacts);
}

}   // End of namespace Physics
}   // End of namespace GE
