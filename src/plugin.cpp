/*
**
* BEGIN_COPYRIGHT
*
* PARADIGM4 INC.
* This file is part of the Paradigm4 Enterprise SciDB distribution kit
* and may only be used with a valid Paradigm4 contract and in accord
* with the terms and conditions specified by that contract.
*
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


/**
 * EXPORT FUNCTIONS
 * Functions from this section will be used by LOAD LIBRARY operator.
 */
EXPORTED_FUNCTION void GetPluginVersion(uint32_t& major, uint32_t& minor, uint32_t& patch, uint32_t& build)
{
    // Provide correct values here. SciDB check it and does not allow to load too new plugins.
    major = 13;
    minor = 6;
    patch = 0;
    build = 0;
}
