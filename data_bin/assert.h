#ifndef CUSTOM_ASSERT_H
#define CUSTOM_ASSERT_H

#include <iostream>
#include <cstdlib>

#define assert(expression)                                                      \
    do                                                                          \
    {                                                                           \
        if (!(expression))                                                      \
        {                                                                       \
            std::cerr << "Assertion failed: (" << #expression << "), file "     \
                      << __FILE__ << ", line " << __LINE__ << "." << std::endl; \
            std::abort();                                                       \
        }                                                                       \
    } while (false)

#endif // CUSTOM_ASSERT_H
