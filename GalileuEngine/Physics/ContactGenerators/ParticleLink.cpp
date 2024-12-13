//
//  ParticleLink.cpp
//  GalileuEngine
//
//  Created by Weyl on 12/11/24.
//

// GE includes.
#include "ParticleLink.hpp"
#include "Vector3.hpp"

namespace GE
{
namespace Physics
{

FReal FParticleLink::computeCurrentLength() const
{
    CHECK(Particles[0] != nullptr)
    CHECK(Particles[1] != nullptr)
    
    const FVector3 deltaPosition = Particles[0]->getPosition() - Particles[1]->getPosition();
    const FReal length = deltaPosition.magnitude();
    return length;
}

}   // End of namespace Physics
}   // End of namespace GE
