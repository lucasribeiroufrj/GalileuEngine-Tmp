//
//  ParticleContact.hpp
//  GalileuEngine
//
//  Created by Weyl on 09/11/24.
//

#pragma once

// GE includes.
#include "Math.hpp"
#include "Particle.hpp"

// STD library includes.
//#include <optional>

namespace GE
{
namespace Physics
{
using Math::FReal;

/**
 * This class represents two FParticles that are touching each other.
 * Resolving a contact eliminates their interpenetration and applies enough impulses to keep them apart.
 * Colliding bodies may also experience a rebound effect.
 */
class FParticleContact
{
public:
    /**
     * Reference to particles in contact.
     * The second reference can be nullptr when there is a single particle, and it is colliding with some piece of immovable scenery.
     */
    FParticle* Particles[2];
    //std::optional<FParticle> OtherParticle;
    
    /** Stores the normal restitution coefficient at the point of contact. */
    FReal RestitutionCoefficient;
    
    /** Stores the contact direction in world coordiantes. */
    FVector3 ContactNormal;
    
    /** Stores the contact penetration depth. */
    FReal PenetrationDepth;
    
    /** Stores the displacement applied to each particle during interpenetration resolution. */
    FVector3 Displacements[2];
    
protected:
    /** 
     * Solve the contact, i.e. recalculates velocity and interpenetration.
     *
     * @param deltaTime The integration time.
     */
    void resolveContact(FReal deltaTime);

private:
    friend class FParticleContactResolver;
    
private:
    /** 
     * Solves for the collision impulse.
     *
     * @param deltaTime The integration time.
     */
    void resolveVelocity(FReal deltaTime);
    
    /**
     * Solves for the interpenetration depth.
     *
     * @param deltaTime The integration time.
     */
    void resolveInterpenetration(FReal deltaTime);
    
    /** Computes the separating velocity at the contact point. */
    FReal computeSeparatingVelocity() const;
    
    /** Computes the separating acceleration at the contact point. */
    FReal computeSeparatingAcceleration() const;
    
    FReal computeTotalInverseMass() const;
    
    /** 
     * Examines the velocity increase resulting solely from acceleration to check if we are facing a resting contact.
     *
     * @param separatingVelocity The separating velocity to be updated, if necessary.
     * @param deltaTime The integration time.
     */
    void updateSeparatingVelocityIfRestingContact(FReal* separatingVelocity, FReal deltaTime);
};

}   // End of namespace Physics
}   // End of namespace GE
