//
//  UtilMacros.hpp
//  GalileuEngine
//
//  Created by lrazevedo on 17/09/24.
//

#pragma once

#if GE_BUILD_DEBUG
    #define FORCE_INLINE inline
    #define CHECK(expr) GE_CHECK_IMPL(expr)

    #define GE_CHECK_IMPL(expr) \
    { \
        if (!(expr)) \
        { \
            throw;\
        } \
    }

#else
    #define FORCE_INLINE __attribute__ ((always_inline)) inline
    #define CHECK(expr)
#endif

// Call STRINGIFY(hey) in order to produce "Hey" at preprocessing phase.
#define STRINGIFY(s) DO_STRINGIFY(s)
#define DO_STRINGIFY(s) #s

#if defined(GE_BUILD_DEBUG)
    #define TRUE_IF_GE_BUILD_DEBUG true
#else
    #define TRUE_IF_GE_BUILD_DEBUG false
#endif
