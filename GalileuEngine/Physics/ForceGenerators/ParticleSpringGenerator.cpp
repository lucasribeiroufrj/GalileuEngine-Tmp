//
//  ParticleSpringGenerator.cpp
//  GalileuEngine
//
//  Created by lrazevedo on 06/10/24.
//

// GE includes.
#include "ParticleSpringGenerator.hpp"
#include "Vector3.hpp"

namespace GE
{
namespace Physics
{

FParticleSpringGenerator::FParticleSpringGenerator(std::shared_ptr<FParticle> particle, FReal springConstant, FReal restLength)
    : OtherParticle(particle), SpringConstant(springConstant), RestLength(restLength)
{
    
}

void FParticleSpringGenerator::updateForce(FParticle* particle, FReal deltaTime)
{
    CHECK(particle != nullptr);
    
    // Calculates the spring vector.
    FVector3 force;
    particle->getPosition(&force);
    force -= OtherParticle->getPosition();
    
    // Calculates the magnitude of the spring force.
    FReal magnitude = force.magnitude();
    magnitude = abs(magnitude - RestLength);
    magnitude *= SpringConstant;
    
    // Calculates the final force (Hooke's law) and apply it.
    force.normalize();
    force *= -magnitude;
    particle->addForce(force);
}

}   // End of namespace Physics
}   // End of namespace GE
