#!/usr/bin/env bash
#
# Math/Functor.h uses std::auto_ptr, which triggers a warning
#
# Usage:  MathFunctor_wrapper_test.sh [include directories]
#
#

declare -a IncludeDirectives
for Dir in "$@" ; do
	IncludeDirectives=( "${IncludeDirectives[@]}" "-I${Dir}" )
done

Cmd=( g++ -std='c++14' -Wall -Werror=deprecated-declarations -pedantic "${IncludeDirectives[@]}" -x'c++' -c )
cat <<EOH
===============================================================================
CMD> ${Cmd[@]}
===============================================================================
EOH
"${Cmd[@]}" - <<EOC
#include "Math/Functor.h"
EOC
res=$?
cat <<EOM
-------------------------------------------------------------------------------
EXIT CODE: ${res} (errors are expected in the compilation above)
EOM
if [[ $res == 0 ]]; then
	echo "Unexpected success in compiling 'Math/Functor.h'. May be time for dropping its wrapper." >&2
	return 1
fi

# this should work instead:
cat <<EOH
===============================================================================
CMD> ${Cmd[@]}
===============================================================================
EOH
Cmd=( g++ -std='c++14' -Wall -Werror=deprecated-declarations -pedantic "${IncludeDirectives[@]}" -x'c++' -c )
"${Cmd[@]}" - <<EOC
#include "larcore/CoreUtils/quiet_Math_Functor.h"
EOC
res=$?
cat <<EOM
-------------------------------------------------------------------------------
EXIT CODE: ${res} (errors are NOT expected in the compilation above)
===============================================================================
EOM

