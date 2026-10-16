/**
    \file   ver.c
    \author Aleksey Lekhtin
    \date   April 5, 2024
    \brief  The version
*/

#include "ver.h"

/**
 * Вывод версии
 */
inline const char *version()
{
    return APP_NAME " Ver. " VER_NUM " " VER_DATE " [built " __DATE__ " " __TIME__ "]";
}
