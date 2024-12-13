//
//  ParticleRod.hpp
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

class FParticleRod : public FParticleLink
{
public:
    /** Stores the rod's length. */
    FReal Length;
    
private:
    /** See @ref FParticleContactGenerator::addContacts. */
    virtual unsigned addContactsImplementation(std::span<FParticleContact> particleContacts) const;
};

}   // End of namespace Physics
}   // End of namespace GE
