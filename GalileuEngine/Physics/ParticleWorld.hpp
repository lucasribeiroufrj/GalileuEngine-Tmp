//
//  ParticleWorld.hpp
//  GalileuEngine
//
//  Created by Weyl on 12/11/24.
//

#pragma once

// GE includes.
#include "Math.hpp"
#include "Particle.hpp"
#include "ParticleForcePairManager.hpp"
#include "ParticleContactResolver.hpp"
#include "ContactGenerators/ParticleContactGenerator.hpp"
#include "ParticleContact.hpp"

// STD library includes.
#include <vector>
#include <optional>

namespace GE
{
namespace Physics
{
using Math::FReal;

/** 
 * Manages a collection of particles and provides methods to update them collectively.
 * So, this is a particle simulator.
 */
class FParticleWorld
{
public:
    /**
     * Creates a world that can handle at most the given number of contacts per frame.
     * The number of contact-resolution iterations may be specified.
     *
     * @param maxNumberOfContacts The maximum number of contacts per frame allowed.
     * @param numberOfIterations The maximum number of iteratons allowed while resolving contacts. The default value is: 2*(number of contacts).
     */
    FParticleWorld(unsigned maxNumberOfContacts, std::optional<unsigned> numberOfIterations = std::nullopt);
    
    /**
     * Prepares the world for a simulation frame by clearing the force accumulators for all particles.
     * Once startFrame() has been called, forces for the current frame can be applied to the particles.
     */
    void startFrame();
    
    /** Invokes each registered contact generator to report contacts and returns the total number of contacts generated. */
    unsigned generateContacts();
    
    /** Advances all particles in the world forward in time by the specified duration.
     *
     * @param deltaTime The integration time.
     */
    void integrate(FReal deltaTime);
    
    /**
     * Executes all physics calculations for the particle world.
     *
     * @param deltaTime The integration time.
     */
    void runPhysics(FReal deltaTime);
    
public: // TEMPORARY
    std::vector<FParticle*>& getParticles(){ return Particles; }
    FParticleForcePairManager& getParticleForcePairManager(){ return ParticleForcePairManager; };
    
protected:
    /** The collection of particles being managed. */
    std::vector<FParticle*> Particles;
    
    /** Indicates whether the world should determine the number of iterations for the contact resolver each frame. */
    const bool isContactResolverIterationsCalculated;
    
    /** Stores the force generators associated with the particles in this world. */
    FParticleForcePairManager ParticleForcePairManager;
    
    /** Stores the particle contact resolver. */
    FParticleContactResolver ParticleContactResolver;
    
    /** Stores the particle contact generators */
    std::vector<FParticleContactGenerator*> ParticleContactGenerators;
    
    /** Stores the list of particle contacts. */
    std::vector<FParticleContact> ParticleContacts;
};

}   // End of namespace Physics
}   // End of namespace GE
