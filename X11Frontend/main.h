/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
    /// @brief main method for all ui backends
    /// @param argc count input args
    /// @param argv data inout args
    /// @return status. 0 - good, 1 - bad
    int run(int argc, char **argv);
#ifdef __cplusplus
}
#endif
