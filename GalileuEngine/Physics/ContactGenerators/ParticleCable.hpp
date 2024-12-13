//
//  ParticleCable.hpp
//  GalileuEngine
//
//  Created by Weyl on 12/11/24.
//

#pragma once

// GE includes.
#include "Math.hpp"
#include "ParticleLink.hpp"

// STD library includes.
#include <span>

namespace GE
{
namespace Physics
{
using Math::FReal;

class FParticleCable : public FParticleLink
{
public:
    /** Stores the cable's maximum length. */
    FReal MaxLength;
    
    /** Stores the cable's bounciness. */
    FReal RestitutionCoefficient;
    
private:
    /** See @ref FParticleContactGenerator::addContacts. */
    virtual unsigned addContactsImplementation(std::span<FParticleContact> particleContacts) const;
};

}   // End of namespace Physics
}   // End of namespace GE
