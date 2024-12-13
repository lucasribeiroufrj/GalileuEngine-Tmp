//
//  ParticleContact.cpp
//  GalileuEngine
//
//  Created by Weyl on 09/11/24.
//

#include "ParticleContact.hpp"

namespace GE
{
namespace Physics
{

void FParticleContact::resolveContact(FReal deltaTime)
{
    resolveVelocity(deltaTime);
    resolveInterpenetration(deltaTime);
}

void FParticleContact::resolveVelocity(FReal deltaTime)
{
    CHECK(Particles[0] != nullptr)      // Note that Particles[1] can be nullptr (see Particles's declaration comment).
    
    FReal separatingVelocity = computeSeparatingVelocity();
    
    // Is the contact either separating or at rest?
    if (separatingVelocity > 0)
    {
        // So, no impulse required.
        return;
    }
    
    const FReal restitutedSeparatingVelocity = -separatingVelocity * RestitutionCoefficient;
    
    updateSeparatingVelocityIfRestingContact(&separatingVelocity, deltaTime);
    
    const FReal deltaVelocity = restitutedSeparatingVelocity - separatingVelocity;
    const FReal totalInverseMass = computeTotalInverseMass();
    
    // Do all particles have infinite mass?
    if (totalInverseMass <= 0)
    {
        // So, they are immovable.
        return;
    }
    
    const FReal impulse = deltaVelocity / totalInverseMass;
    const FVector3 impulsePerInverseMass = ContactNormal * impulse;
    const auto applyImpulse = [&impulsePerInverseMass](FParticle* const particle, const FReal direction)
    {
        if (particle == nullptr) return;
        
        particle->addVelocity(impulsePerInverseMass * (direction * particle->getInverseMass()));
    };
    applyImpulse(Particles[0], +1);
    applyImpulse(Particles[1], -1);     // Opposite direction.
}

void FParticleContact::resolveInterpenetration(FReal deltaTime)
{
    CHECK(Particles[0] != nullptr)      // Note that Particles[1] can be nullptr (see Particles's declaration comment).
    
    // No penetration?
    if (PenetrationDepth <= 0)
    {
        // So, nothing to solve for.
        return;
    }
    
    const FReal totalInverseMass = computeTotalInverseMass();
    
    // Do all particles have infinite mass?
    if (totalInverseMass <= 0)
    {
        // So, they are immovable.
        return;
    }
    
    FVector3 movePerInverseMass = ContactNormal * (PenetrationDepth / totalInverseMass);
    Displacements[0] = movePerInverseMass * Particles[0]->getInverseMass();
    if (Particles[1] != nullptr)
    {
        Displacements[1] = movePerInverseMass * -Particles[1]->getInverseMass();
    }
    else
    {
        Displacements[1].zeroOut();
    }
    
    Particles[0]->addDisplacement(Displacements[0]);
    if (Particles[1] != nullptr)
    {
        Particles[1]->addDisplacement(Displacements[1]);
    }
}

FReal FParticleContact::computeSeparatingVelocity() const
{
    FVector3 relativeVelocity = Particles[0]->getVelocity();
    if (Particles[1])
    {
        relativeVelocity -= Particles[1]->getVelocity();
    }
    return relativeVelocity | ContactNormal;
}

FORCE_INLINE FReal FParticleContact::computeSeparatingAcceleration() const
{
    FVector3 relativeAcceleration = Particles[0]->getAcceleration();
    if (Particles[1])
    {
        relativeAcceleration -= Particles[1]->getAcceleration();
    }
    return relativeAcceleration | ContactNormal;
}

FORCE_INLINE FReal FParticleContact::computeTotalInverseMass() const
{
    const bool isThereSecondParticle = Particles[1] != nullptr;
    const FReal mass0 = Particles[0]->getInverseMass();
    const FReal mass1 = isThereSecondParticle ? Particles[1]->getInverseMass() : 0;
    const FReal totalInverseMass = mass0 + mass1;
    return totalInverseMass;
}

FORCE_INLINE void FParticleContact::updateSeparatingVelocityIfRestingContact(FReal* separatingVelocity, FReal deltaTime)
{
    const FReal separatingAcceleration = computeSeparatingAcceleration();
    
    // Are they interpenetrating solely from acceleration?
    if (separatingAcceleration < 0)
    {
        const FReal accelerationCausedSeparatingVelocity = separatingAcceleration * deltaTime;
        *separatingVelocity += accelerationCausedSeparatingVelocity * RestitutionCoefficient;
        
        // Ensure we do not remove more than was there to remove.
        if (*separatingVelocity < 0)
        {
            *separatingVelocity = 0;
        }
    }
}

}   // End of namespace Physics
}   // End of namespace GE
