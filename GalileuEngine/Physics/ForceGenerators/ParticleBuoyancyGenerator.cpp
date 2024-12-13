//
//  ParticleBuoyancyGenerator.cpp
//  GalileuEngine
//
//  Created by lrazevedo on 08/10/24.
//

// GE includes.
#include "ParticleBuoyancyGenerator.hpp"
#include "Vector3.hpp"

namespace GE
{
namespace Physics
{

FParticleBuoyancyGenerator::FParticleBuoyancyGenerator(FReal maxDepth, FReal objectVolume, FReal liquidHeight, FReal liquidDensity)
: MaxDepth(maxDepth), ObjectVolume(objectVolume), LiquidHeight(liquidHeight), LiquidDensity(liquidDensity)
{
    
}

void FParticleBuoyancyGenerator::updateForce(FParticle *particle, FReal deltaTime)
{
    FReal submersionDepth = particle->getPosition().Y;
    
    // Is the object out of the liquid?
    if ((submersionDepth) >= (LiquidHeight + MaxDepth))
    {
        return;
    }
    FVector3 buoyancyforce = FVector3::ZeroVector;
    
    // Is the object fully submerged?
    if ((submersionDepth) <= (LiquidHeight - MaxDepth))
    {
        buoyancyforce.Y = LiquidDensity * ObjectVolume;
        particle->addForce(buoyancyforce);
        return;
    }
    
    // Otherwise it is partly submerged.
    FReal relativeDepth = (submersionDepth - LiquidHeight - MaxDepth) / (2 * MaxDepth);
    buoyancyforce.Y = LiquidDensity * ObjectVolume * relativeDepth;
    particle->addForce(buoyancyforce);
}

}   // End of namespace Physics
}   // End of namespace GE
