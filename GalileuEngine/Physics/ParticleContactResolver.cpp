//
//  ParticleContactResolver.cpp
//  GalileuEngine
//
//  Created by Weyl on 12/11/24.
//

#include "ParticleContactResolver.hpp"

namespace GE
{
namespace Physics
{

FParticleContactResolver::FParticleContactResolver(unsigned maxNumberOfIterations)
: MaxNumberOfIterations(maxNumberOfIterations)
{
}

void FParticleContactResolver::setMaxNumberOfIterations(unsigned maxNumberOfIterations)
{
    MaxNumberOfIterations = maxNumberOfIterations;
}

void FParticleContactResolver::resolveContacts(std::vector<FParticleContact>& contacts, const FReal deltaTime)
{
    UsedNumberOfIterations = 0;
    
    while( UsedNumberOfIterations < MaxNumberOfIterations )
    {
        // Identify the contact with the highest closing velocity, i.e. mininum separating velocity.
        FReal minSeparatingVelocity = Math::Max_number;
        size_t minContactIndex = contacts.size();
        for (size_t contactIndex = 0; contactIndex < contacts.size(); ++contactIndex)
        {
            const FReal separatingVelocity = contacts[contactIndex].computeSeparatingVelocity();
            if ((separatingVelocity < minSeparatingVelocity) && ((separatingVelocity < 0) || (contacts[contactIndex].PenetrationDepth > 0)))
            {
                minSeparatingVelocity = separatingVelocity;
                minContactIndex = contactIndex;
            }
        }
        
        // Did not find a minimum?
        if (minContactIndex == contacts.size())
        {
            break;
        }
        
        contacts[minContactIndex].resolveContact(deltaTime);
        ++UsedNumberOfIterations;
    }
}

}   // End of namespace Physics
}   // End of namespace GE
