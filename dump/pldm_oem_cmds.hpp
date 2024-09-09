#pragma once

#include <libpldm/oem/ibm/file_io.h>
#include <libpldm/pldm.h>

namespace openpower::dump::pldm
{
namespace internal
{
/**
 * @brief Reads the MCTP endpoint ID out of a file
 */
mctp_eid_t readEID();
} // namespace internal
/**
 * @brief Send new file available PLDM command
 *
 * @param[in] id - Dump id
 * @param[in] dumpType - Type of the dump.
 * @param[in] dumpSize - size of the dump
 * @return NULL
 *
 */
void newFileAvailable(uint32_t id, pldm_fileio_file_type dumpType,
                      uint64_t dumpSize);
} // namespace openpower::dump::pldm
