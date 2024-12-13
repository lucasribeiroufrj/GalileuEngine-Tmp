//
//  Particle.cpp
//  GalileuEngine
//
//  Created by lrazevedo on 04/09/24.
//

#include "Particle.hpp"

// STD library includes.
#include <limits>
#include <cassert>

using namespace GE::Physics;

void FParticle::integrate(const FReal deltaTime)
{
    // Check whether the particle is immovable or not.
    if (InverseMass <= Math::Zero)
    {
        return;
    }
    
    assert(deltaTime > Math::Zero);
    
    // Position integration.
    Position.addScaledVector(deltaTime, Velocity);
    
    // Velocity integration.
    FVector3 finalAcceleration = Acceleration;
    finalAcceleration.addScaledVector(InverseMass, AccumulatedForces);
    Velocity.addScaledVector(deltaTime, finalAcceleration);
    
    // Apply drag.
    Velocity *= pow(Damping, deltaTime);
}

void FParticle::getPosition(FVector3* position) const
{
    *position = Position;
}

FVector3 FParticle::getPosition() const
{
    return Position;
}

void FParticle::setPosition(const FVector3& position)
{
    Position = position;
}

void FParticle::addDisplacement(const FVector3& displacement)
{
    Position += displacement;
}

void FParticle::getVelocity(FVector3* velocity) const
{
    *velocity = Velocity;
}

FVector3 FParticle::getVelocity() const
{
    return Velocity;
}

void FParticle::setVelocity(const FVector3& velocity)
{
    Velocity = velocity;
}

void FParticle::addVelocity(const FVector3& velocity)
{
    Velocity += velocity;
}

FVector3 FParticle::getAcceleration() const
{
    return Acceleration;
}

bool FParticle::hasFiniteMass() const
{
    return InverseMass > Math::Zero;
}

void FParticle::setMass(const FReal mass)
{
    assert(mass != Math::Zero);
    InverseMass = Math::One / mass;
}

FReal FParticle::getMass() const
{
    if (InverseMass == Math::Zero)
    {
        return Math::Max_number;
    }
    else
    {
        return Math::One / InverseMass;
    }
}

void FParticle::setInverseMass(const FReal inverseMass)
{
    InverseMass = inverseMass;
}

FReal FParticle::getInverseMass() const
{
    return InverseMass;
}

void FParticle::setDamping(const FReal damping)
{
    Damping = damping;
}

void FParticle::clearAccumulatedForces()
{
    AccumulatedForces.zeroOut();
}

void FParticle::addForce(const FVector3& force)
{
    AccumulatedForces += force;
}
