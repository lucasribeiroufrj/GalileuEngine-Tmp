//
//  ParticleGravityGenerator.cpp
//  GalileuEngine
//
//  Created by lrazevedo on 01/10/24.
//

#include "ParticleGravityGenerator.hpp"

namespace GE
{
namespace Physics
{

FParticleGravityGenerator::FParticleGravityGenerator(const FVector3& gravityAcceleration)
: GravityAcceleration{gravityAcceleration}
{
    
}

void FParticleGravityGenerator::updateForce(FParticle* particle, FReal deltaTime)
{
    CHECK(particle != nullptr)
    
    // Don't apply gravity to particles with infinite mass.
    if (!particle->hasFiniteMass())
    {
        return;
    }
    
    particle->addForce(GravityAcceleration * particle->getMass());
}

}   // End of namespace Physics
}   // End of namespace GE
