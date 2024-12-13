//
//  ParticleGravityGenerator.hpp
//  GalileuEngine
//
//  Created by lrazevedo on 01/10/24.
//

#pragma once

// GE includes.
#include "Vector3.hpp"
#include "ParticleForceGenerator.hpp"

namespace GE
{
namespace Physics
{
using Math::FVector3;

/**
 * A force generator that applies gravitational force. A single instance can be shared across multiple particles.
 */
class FParticleGravityGenerator : public FParticleForceGenerator
{
public:
    /**
     * Constructor inititializing the generator with the given acceleration vector.
     *
     * @param gravityAcceleration The gravity acceleration vector.
     */
    explicit FParticleGravityGenerator(const FVector3& gravityAcceleration);
    
    /**
     * Calculates and updates the gravity force applied to a given particle during a deltaTime.
     *
     * @param particle The particle that it requesting the gravity force.
     * @param deltaTime The integration time.
     */
    void updateForce(FParticle* particle, FReal deltaTime) override;
    
private:
    /**
     * Stores the gravity acceleration.
     */
    FVector3 GravityAcceleration;
};

}   // End of namespace Physics
}   // End of namespace GE
