//
//  Precision.hpp
//  GalileuEngine
//
//  Created by lrazevedo on 01/09/24.
//

#pragma once

// GE includes.
#include <type_traits>

namespace GE
{
namespace Math
{

using FReal = float;
static_assert(std::is_floating_point_v<FReal>, "FReal must be a floating point type.");

}   // End of namespace Math
}   // End of namespace GE
