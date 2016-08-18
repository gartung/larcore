/**
 * This is a wrapper to ROOT's Math/Functor.h that disables a warning about deprecated stuff.
 * That header is known to use std::auto_ptr.
 *
 * A unit test is connected with this header.
 * When that unit test fails, it means the workaround is not needed any more.
 * Then, this wrapper should be removed and all the include directives restored.
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "Math/Functor.h"

#pragma GCC diagnostic pop 
