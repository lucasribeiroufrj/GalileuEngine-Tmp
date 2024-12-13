//
//  Particle.hpp
//  GalileuEngine
//
//  Created by lrazevedo on 04/09/24.
//

#pragma once

// GE includes.
#include "Math.hpp"
#include "Vector3.hpp"

namespace GE
{
namespace Physics
{
using Math::FVector3;
using Math::FReal;

/**
 * Abstracts a point mass, this is the simplest object that can be simulated in this physics engine.
 */
class FParticle
{
public:
    /**
     * Advances the particle forward in time by the specified amount. This function applies the Newton-Euler integration method, a linear approximation of the exact integral.
     *
     * @param deltaTime The integration time.
     */
    void integrate(const FReal deltaTime);
    
    /**
     * Sets the given vector with the position of the particle.
     *
     * @param position Pointer to a vector for writing the position.
     */
    void getPosition(FVector3* position) const;
    
    /**
     * Returns the current particle's position.
     */
    FVector3 getPosition() const;
    
    /**
     * Sets the current particle's position.
     *
     * @param position The new position of the particle.
     */
    void setPosition(const FVector3& position);
    
    /**
     * Adds a displacement to the current particle's position.
     *
     * @param displacement The displacement amount.
     */
    void addDisplacement(const FVector3& displacement);
    
    /**
     * Sets the given vector with the velocity of the particle.
     *
     * @param velocity Pointer to a vector for writing the velocity.
     */
    void getVelocity(FVector3* velocity) const;
    
    /**
     * Returns the current particle's velocity.
     */
    FVector3 getVelocity() const;
    
    /**
     * Sets the current particle's velocity.
     *
     * @param velocity The new velocity of the particle.
     */
    void setVelocity(const FVector3& velocity);
    
    /**
     * Adds a velocity vector to the current particle's velocity.
     *
     *@param velocity The velocity to be added.
     */
    void addVelocity(const FVector3& velocity);
    
    /**
     * Returns the current particle's acceleration.
     */
    FVector3 getAcceleration() const;
    
    /**
     * Check the particle’s mass.
     *
     * @return The value is true if the particle’s mass non-infinite.
     */
    bool hasFiniteMass() const;
    
    /**
     * Sets the particle’s mass. Use it in case of a finite-mass object, otherwise use setInverseMass() function.
     *
     * @param mass The new mass of the particle. It cannot be zero.
     */
    void setMass(const FReal mass);
    
    /**
     * Gets the particle’s mass.
     *
     * @return The current mass of the particle.
     */
    FReal getMass() const;
    
    /**
     * Sets the particle’s inverse mass. Use it in case of a infinite-mass object,
     *
     * @param inverseMass The new inverse mass of the particle. It can be zero in case of a particle with infinite mass (unmovable)
     */
    void setInverseMass(const FReal inverseMass);
    
    /**
     * Gets the particle’s inverse mass.
     *
     * @return The current inverse mass of the particle.
     */
    FReal getInverseMass() const;
    
    /**
     * Sets the particle’s damping. See damping member for better description.
     *
     * @param damping The new damping. It should be in the [0, 1] interval.
     */
    void setDamping(const FReal damping);
    
    /**
     * Clears all the forces applied to the particle.
     */
    void clearAccumulatedForces();
    
    /**
     * Adds the specified force to the particle, which will be applied in the next simulation iteration only.
     */
    void addForce(const FVector3& force);
    
protected:
    /**
     * Stores the linear position of the particle in world space.
     */
    FVector3 Position;
    
    /**
     * Stores the linear velocity of the particle in world space.
     */
    FVector3 Velocity;
    
    /**
     * Stores the linear acceleration of the particle in world space.
     * Primarily used to define acceleration due to gravity, but it can also be used for any other constant acceleration.
     */
    FVector3 Acceleration;
    
    /**
     * Stores the accumulated force to be used by the integrator. It is zeroed out at each integration step.
     * AccumulatedForces = Force = sum_i Force_i => Accel = (1.0 / Mass) * Force.
     * These forces are accumulated by mean of FParticle's addForce(Force_i).
     */
    FVector3 AccumulatedForces;
    
    /**
     * Stores the damping factor applied to linear motion.
     * Damping is necessary to eliminate excess energy introduced by numerical instability in the integrator.
     */
    FReal Damping = Math::One;
    
    /**
     * Stores (1.0 / Mass), instead of just (Mass), because this way it is easy to set infinite-mass objects (immovable), but difficult to set zero-mass objects (create numerical problems).
     */
    FReal InverseMass;
};

}   // End of namespace Physics
}   // End of namespace GE
