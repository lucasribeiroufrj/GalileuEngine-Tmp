//
//  ParticleContactGenerator.hpp
//  GalileuEngine
//
//  Created by Weyl on 12/11/24.
//

#pragma once

// GE includes.
#include "Math.hpp"
#include "ParticleContact.hpp"

// STD library includes.
#include <span>

namespace GE
{
namespace Physics
{
using Math::FReal;

class FParticleContactGenerator
{
public:
    /**
     * Generates new contacts and fills in an array with those.
     *
     * @param particleContacts The array to be filled in. There must be size for at least one element.
     * @return The number of array elements actually used.
     */
    unsigned addContacts(std::span<FParticleContact> particleContacts) const;
    
private:
    /** See @ref addContacts. */
    virtual unsigned addContactsImplementation(std::span<FParticleContact> particleContacts) const = 0;
};

}   // End of namespace Physics
}   // End of namespace GE
