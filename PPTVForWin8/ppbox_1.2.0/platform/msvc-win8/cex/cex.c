// cex.c

#include <windows.h>

#include <stdlib.h>

namespace cex {

    char * __cdecl getenv(
        _In_z_ const char * _VarName
        )
    {
        LPCSTR * p = NULL;
        if (GetEnvironmentVariableA(_VarName, (LPSTR)&p, 0) > 0) {
            return (char *)p;
        }
        return NULL;
    }

}
