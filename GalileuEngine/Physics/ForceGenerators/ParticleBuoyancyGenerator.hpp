//
//  ParticleBuoyancyGenerator.hpp
//  GalileuEngine
//
//  Created by lrazevedo on 08/10/24.
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
 * A force generator that applies a buoyancy force for a plane of liquid parallel to XZ plane.
 */
class FParticleBuoyancyGenerator : public FParticleForceGenerator
{
public:
    /**
     * Pure water has a density of 1000 kg per cubic meter.
     */
    static constexpr FReal PureWaterDensity = 1000.0f;
    
public:
    /**
     * Constructor inititializing the generator with the given parameters.
     * 
     */
    FParticleBuoyancyGenerator(FReal maxDepth, FReal objectVolume, FReal liquidHeight, FReal liquidDensity = PureWaterDensity);
    
    /**
     * Calculates and updates the buoyancy force applied to a given particle during a deltaTime.
     *
     * @param particle The particle that is requesting the buoyancy force.
     * @param deltaTime The integration time.
     */
    void updateForce(FParticle *particle, FReal deltaTime) override;
private:
    /**
     * The maximum submersion depth of the object before it generates its maximum bouyance force.
     */
    FReal MaxDepth;
    
    /**
     * The volume of the object the bouyance force is applied to.
     */
    FReal ObjectVolume;
    
    /**
     * The height of the liquid plane above y = 0. The plane is parallel to the XY plane.
     */
    FReal LiquidHeight;
    
    /**
     * The density of the liquid, e.g. pure water has a density of 1000 kg per cubic meter.
     */
    FReal LiquidDensity;
};

}   // End of namespace Physics
}   // End of namespace GE
