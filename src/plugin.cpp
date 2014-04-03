/*
**
* BEGIN_COPYRIGHT
*
* PARADIGM4 INC.
* Copyright © 2010 - 2012 Paradigm4 Inc.
* All Rights Reserved.
*
* END_COPYRIGHT
*/

/*
 * @file plugin.cpp
 *
 * @author poliocough@gmail.com
 *
 * @brief Implementation of some plugin functions.
 */

#include <vector>

#include "SciDBAPI.h"
#include "system/Constants.h"


EXPORTED_FUNCTION void GetPluginVersion(uint32_t& major, uint32_t& minor, uint32_t& patch, uint32_t& build)
{
    major = scidb::SCIDB_VERSION_MAJOR();
    minor = scidb::SCIDB_VERSION_MINOR();
    patch = scidb::SCIDB_VERSION_PATCH();
    build = scidb::SCIDB_VERSION_BUILD();
}
