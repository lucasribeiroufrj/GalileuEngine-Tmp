//
//  ParticleLink.hpp
//  GalileuEngine
//
//  Created by Weyl on 12/11/24.
//

#pragma once

// GE includes.
#include "Math.hpp"
#include "Particle.hpp"
#include "ParticleContactGenerator.hpp"

// STD library includes.
#include <span>

namespace GE
{
namespace Physics
{
using Math::FReal;

/**
 * This class constraints a pair of particles, generating a contact if they violate the contraint (link).
 * Can be used as a base class for cables and rods.
 */
class FParticleLink : public FParticleContactGenerator
{
public:
    /** Stores the pair of particles that composes this link. */
    FParticle* Particles[2];

protected:
    /** Gets the current link's length. */
    FReal computeCurrentLength() const;
    
private:
    /** See @ref FParticleContactGenerator::addContacts. */
    virtual unsigned addContactsImplementation(std::span<FParticleContact> particleContacts) const = 0;
};

}   // End of namespace Physics
}   // End of namespace GE
