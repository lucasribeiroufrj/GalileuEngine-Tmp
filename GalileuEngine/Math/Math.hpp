//
//  Math.hpp
//  GalileuEngine
//
//  Created by lrazevedo on 02/09/24.
//

#pragma once


#include "Precision.hpp"

// STD library includes.
#include <cmath>

// BEG - basic concepts
template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;
// END - basic concepts

namespace GE
{
namespace Math
{

template<Arithmetic TArg>
TArg sqrt(TArg arg)
{
    return std::sqrt(arg);
}

template<Arithmetic TArg>
TArg abs(TArg arg)
{
    return std::abs(arg);
}

constexpr FReal Max_number = std::numeric_limits<FReal>::max();
constexpr FReal Small_number = (FReal) 1.e-8;
constexpr FReal Kinda_Small_number = (FReal) 1.e-4;
constexpr FReal Zero = (FReal) 0.0;
constexpr FReal One = (FReal) 1.0;

}   // End of namespace Math
}   // End of namespace GE
