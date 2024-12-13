//
//  Vector3.hpp
//  GalileuEngine
//
//  Created by lrazevedo on 01/09/24.
//

#pragma once

// GE includes.
#include "Precision.hpp"
#include "Math.hpp"
#include "UtilMacros.hpp"

#include <iostream>

namespace GE
{
namespace Math
{

class FVector3
{
public:
    /** A zero vector (0,0,0) */
    static const FVector3 ZeroVector;
    
public:
    union
    {
        struct
        {
            FReal X;
            FReal Y;
            FReal Z;
        };
        
        [[deprecated("For internal use only.")]]
        FReal XYZ[3];
    };

public:
    /** 
     * Default constructor (no initialization).
     */
    FORCE_INLINE FVector3() {}

    /** 
     * Constructor initializing all components to a single value.
     *
     * @param InV Value to set all components to.
     */
    FORCE_INLINE explicit FVector3(FReal InV) : X(InV), Y(InV), Z(InV) {}
    
    /**
     * Constructor using initial values for each component.
     *
     * @param InX Value to set X component to.
     * @param InY Value to set Y component to.
     * @param InZ Value to set Z component to.
     */
    FORCE_INLINE FVector3(FReal InX, FReal InY, FReal InZ) : X(InX), Y(InY), Z(InZ) {}
    
    /**
     * Multiply all components of the vector by -one.
     */
    FORCE_INLINE void invert()
    {
        X = -X;
        Y = -Y;
        Z = -Z;
    }
    
    /**
     * Set all components of the vector to zero.
     */
    FORCE_INLINE void zeroOut()
    {
        X = Y = Z = Zero;
    }
    
    /**
     * Returns the magnitude of this vector.
     *
     * @return The magnitude.
     */
    FORCE_INLINE FReal magnitude() const
    {
        return Math::sqrt(X*X + Y*Y + Z*Z);
    }
    
    /**
     * Returns the squared magnitude of this vector.
     *
     * @return The squared magnitude.
     */
    FORCE_INLINE FReal squareMagnitude() const
    {
        return X*X + Y*Y + Z*Z;
    }
    
    /**
     * Turns a non-zero vector into a vector of unit length.
     */
    FORCE_INLINE void normalize()
    {
        FReal m = magnitude();
        if (m > 0)
        {
            (*this) *= Math::One / m;
        }
    }
    
    /**
     * Gets the vector-vector addition.
     *
     * @param vector The vector to add this vector to.
     * @return Copy of the result after addition.
     */
    FORCE_INLINE FVector3 operator+=(const FVector3& vector)
    {
        X += vector.X;
        Y += vector.Y;
        Z += vector.Z;
        return *this;
    }
    
    /**
     * Gets the vector-vector subtraction.
     *
     * @param vector The vector to add this vector to.
     * @return Copy of the result after subtraction.
     */
    FORCE_INLINE FVector3 operator-=(const FVector3& vector)
    {
        X -= vector.X;
        Y -= vector.Y;
        Z -= vector.Z;
        return *this;
    }
    
    /**
     * Scales the vector.
     *
     * @param scale Amount to scale this vector by.
     * @return Copy of the vector after scaling.
     */
    template <Arithmetic TArg>
    FORCE_INLINE FVector3 operator*=(const TArg scale)
    {
        X *= scale;
        Y *= scale;
        Z *= scale;
        return *this;
    }
    
    /**
     * Gets the vector-vector addition.
     *
     * @param vector The vector to add this vector to.
     * @return Copy of the result after addition
     */
    FORCE_INLINE FVector3 operator+(const FVector3& vector) const
    {
        return FVector3(X + vector.X, Y + vector.Y, Z + vector.Z);
    }
    
    /**
     * Gets the vector-vector subtraction.
     *
     * @param vector The vector to add this vector to.
     * @return Copy of the result after subtraction
     */
    FORCE_INLINE FVector3 operator-(const FVector3& vector) const
    {
        return FVector3(X - vector.X, Y - vector.Y, Z - vector.Z);
    }
    
    /**
     * Gets the vector-scale multiplication.
     *
     * @param scale Amount to scale this vector by.
     * @return Copy of the vector after scaling.
     */
    template <Arithmetic TArg>
    FORCE_INLINE FVector3 operator*(const TArg scale) const
    {
        return FVector3(X*scale, Y*scale, Z*scale);
    }
    
    /**
     * Calculates and returns the cross product of this vector with the given vector.
     *
     * @param vector The given vector to multiply with.
     * @return A copy of the result.
     */
    FORCE_INLINE FVector3 operator^(const FVector3& vector) const
    {
        return FVector3(Y*vector.Z - Z*vector.Y, Z*vector.X - X*vector.Z, X*vector.Y - Y*vector.X);
    }
    
    /**
     * Gets the vector-scale subdivision.
     *
     * @param scale Amount to subdivide this vector by.
     * @return Copy of the vector after subdivision.
     */
    template <Arithmetic TArg>
    FORCE_INLINE FVector3 operator/(const TArg scale) const
    {
        return FVector3(X/scale, Y/scale, Z/scale);
    }
    
    /**
     * Performs the dot product between this and another vector.
     *
     * @param vector The other vector.
     * @return The result of the dot product.
     */
    FORCE_INLINE FReal operator|(const FVector3& vector) const
    {
        return X*vector.X + Y*vector.Y + Z*vector.Z;
    }
    
    /**
     * Performs the cross product of this vector with the given vector and sets this vector to this result.
     *
     * @param vector The other vector.
     */
    FORCE_INLINE void operator^=(const FVector3& vector)
    {
        *this = crossProduct(vector);
    }
    
    /**
     * Performs the dot product between this and another vector.
     *
     * @param vector The other vector.
     * @return The result of the dot product.
     */
    FORCE_INLINE FReal dotProduct(const FVector3& vector) const
    {
        return *this | vector;
    }
    
    /**
     * Gets the component-wise product (multiplication) of this vector with the given vector.
     *
     * @param vector The given vector to multiply with.
     * @return A copy of the result.
     */
    FORCE_INLINE FVector3 componentProduct(const FVector3& vector) const
    {
        return FVector3(X*vector.X, Y*vector.Y, Z*vector.Z);
    }
    
    /**
     * Calculates and returns the cross product of this vector with the given vector.
     *
     * @param vector The given vector to multiply with.
     * @return A copy of the result.
     */
    FORCE_INLINE FVector3 crossProduct(const FVector3& vector) const
    {
        return *this ^ vector;
    }
    
    /**
     * Performs a component-wise product (multiplication) of this vector with the given vector and sets this vector to its result.
     *
     * @param vector The given vector to multiply with.
     */
    FORCE_INLINE void componentProductUpdate(const FVector3& vector)
    {
        X *= vector.X;
        Y *= vector.Y;
        Z *= vector.Z;
    }
    
    /**
     * Scales the given vector, then add to this one.
     *
     * @param vector The other vector, the one to be scaled, then added to this one.
     * @param scale The scale.
     */
    template <Arithmetic TArg>
    FORCE_INLINE void addScaledVector(const TArg scale, const FVector3& vector)
    {
        X += vector.X * scale;
        Y += vector.Y * scale;
        Z += vector.Z * scale;
    }
    
    /**
     * Construct a triple of mutually orthogonal vectors, where each vector is at right angles to the other two.
     * The xAxis and yAxis vectors must be nonparallel.
     * The three vectors will be normalized.
     *
     * @param xAxis The input basis' x axis, and upon return the orthonormal basis' x axis.
     * @param yAxis The input basis' y axis, and upon return the orthonormal basis' y axis.
     * @param zAxis Any vector, and upon return the orthonormal basis' z axis.
     */
    FORCE_INLINE static void makeOrthonormalBasis(FVector3* xAxis, FVector3* yAxis, FVector3* zAxis)
    {
        xAxis->normalize();
        (*zAxis) = (*xAxis) ^ (*yAxis);
        if (zAxis->squareMagnitude() < Math::Small_number)
        {
            return;
        }
        zAxis->normalize();
        (*yAxis) = (*zAxis) ^ (*xAxis);
    }
    
private:
    friend std::ostream& operator<<(std::ostream& ostream, const FVector3& vector)
    {
        ostream << "[" << vector.X << "," << vector.Y << "," << vector.Z << "]";
        return ostream;
    }
};  // End of class FVector3

}   // End of namespace Math
}   // End of namespace GE
