#include "Application.hpp"
#include <iostream>
#include "Vector3.hpp"
#include "Particle.hpp"
#include "ParticleGravityGenerator.hpp"
#include "ParticleWorld.hpp"

#include <cmath>
#include <cassert>
#include <thread>
#include <chrono>

void debugParticle(const GE::Physics::FParticle& particle)
{
    std::cout << "Mass: " << particle.getMass() << std::endl;
    std::cout << "InverseMass: " << particle.getInverseMass() << std::endl;
    std::cout << "Position: " << particle.getPosition() << std::endl;
    std::cout << "Velocity: " << particle.getVelocity() << std::endl;
}

void updatePhysics(bool& isPhysicsEnabled)
{
    using namespace GE::Math;
    using namespace GE::Physics;
    
    FReal timeSinceStart = 0.0f;
    FReal deltaTime = 1.0f / 60.0f;
    
    std::vector<FParticle> particles;
    
    struct FFlyingParticle
    {
        size_t particleIndex;
        FVector3 InitialPosition;
        FVector3 InitialVelocity;
    };
    std::optional<FFlyingParticle> flyingParticle{std::nullopt};
    
    // Add a dummy particle.
    {
        flyingParticle = FFlyingParticle
        {
            .InitialPosition = {5.2, 2.0, 9.2},
            .InitialVelocity = {3.0, 5.5, -4.0f},
        };
        
        FParticle particle{};
        particle.setMass(1.0f);
        particle.setPosition(flyingParticle->InitialPosition);
        particle.setVelocity(flyingParticle->InitialVelocity);
        particles.push_back(std::move(particle));
        flyingParticle->particleIndex = particles.size() - 1;
    }
    
    // Add a dummy particle.
    {
        FParticle particle{};
        particle.setInverseMass(Zero);  // Immovable particle.
        particles.push_back(std::move(particle));
    }
    
    // Print debug.
    std::cout << "==== Debug info before simulation starts ====" << std::endl;
    for (const auto& p : particles) {
        debugParticle(p);
    }
    
    // Force generators.
    const FVector3 gravityVector{0.03, -10.0, 0.2};
    FParticleGravityGenerator gravityGenerator{gravityVector};
    
    // BEG - World setup.
    const unsigned maxNumberOfContacts = 20;
    FParticleWorld world{maxNumberOfContacts};
    std::vector<FParticle*>& worldParticles = world.getParticles();
    
    for (auto& p : particles)
    {
        worldParticles.push_back(&p);
        world.getParticleForcePairManager().add(&p, &gravityGenerator);
    }
    // END - World setup.
    
    // BEG - Run simulation.
    while (isPhysicsEnabled)
    {
        auto durationInSeconds = std::chrono::duration<FReal>(deltaTime);
        std::this_thread::sleep_for(durationInSeconds);
        timeSinceStart += deltaTime;

        world.startFrame();
        world.runPhysics(deltaTime);
    }
    // END - Run simulation.
    
    // Print debug.
    std::cout << "==== Debug info after simulation starts =====" << std::endl;
    for (const auto& p : particles) {
        debugParticle(p);
    }
    
    // Analytical solution comparison.
    if (flyingParticle != std::nullopt)
    {
        auto analyticalFlightSolution = [](FVector3 pos, FVector3 vel, FVector3 acc, FReal finalTime)
        {
            return pos + vel*finalTime + acc*(finalTime*finalTime / (FReal)2.0);
        };
        
        const FVector3 finalPositional = analyticalFlightSolution(flyingParticle->InitialPosition, flyingParticle->InitialVelocity, gravityVector, timeSinceStart);
        const FParticle& engineFlyingParticle = particles[flyingParticle->particleIndex];
        const FVector3 diffVector = finalPositional - engineFlyingParticle.getPosition();
        const FReal error = diffVector.magnitude() / finalPositional.magnitude() * 100.0;
        std::cout << "==== Comparison with analytical solution ====" << std::endl;
        std::cout << "Analytical position: " << finalPositional << std::endl;
        std::cout << "Relative error: " << error << "%" << std::endl;
        assert(error < 2.0);        // Error should be below this percentage!
        std::cout << "=============================================" << std::endl;
    }
    
    std::cout << "The physics engine has run for " << timeSinceStart << "s.\n";
}

template<class T = decltype(std::chrono::high_resolution_clock::now())>
class FStopwatch
{
public:
    FStopwatch()
        : Start{now()}
    {
        // Nothing to do.
    }
    
    void end()
    {
        End = now();
        Elapsed = End - Start;
    }

public:
    static T now()
    {
        return std::chrono::high_resolution_clock::now();
    }

public:
    T Start;
    T End;
    std::chrono::duration<float> Elapsed;
};

int main()
{
    try
    {
        FStopwatch stopwatch{};
        bool isPhysicsEnabled = true;
        std::thread physicsThread(updatePhysics, std::ref(isPhysicsEnabled));
        GE::Vulkan::FApplication application{};
        application.run();
        isPhysicsEnabled = false;
        physicsThread.join();
        stopwatch.end();
        std::cout << "The program has run for " << stopwatch.Elapsed << ".\n";
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
