#include "dump_send_pldm_cmd.hpp"

#include "pldm_oem_cmds.hpp"

#include <fmt/format.h>
#include <libpldm/file_io.h>

#include <phosphor-logging/log.hpp>

namespace openpower::dump::pldm
{
using ::phosphor::logging::level;
using ::phosphor::logging::log;

void sendNewDumpCmd(uint32_t dumpId, DumpType dumpType, uint64_t dumpSize)
{
    uint32_t pldmDumpType = 0;
    // TODO https://github.com/ibm-openbmc/powervm-handler/issues/9
    switch (dumpType)
    {
        case DumpType::bmc:
            pldmDumpType = 0xF; // PLDM_FILE_TYPE_BMC_DUMP
            break;
        case DumpType::sbe:
            pldmDumpType = 0x10; // PLDM_FILE_TYPE_SBE_DUMP
            break;
        case DumpType::hostboot:
            pldmDumpType = 0x11; // PLDM_FILE_TYPE_HOSTBOOT_DUMP
            break;
        case DumpType::hardware:
            pldmDumpType = 0x12; // PLDM_FILE_TYPE_HARDWARE_DUMP
            break;
        default:
            std::string err =
                fmt::format("Unsupported dump type ({}) ", dumpType);
            log<level::ERR>(err.c_str());
            throw std::out_of_range(err);
            break;
    }

    log<level::INFO>(fmt::format("sendNewDumpCmd Id({}) Size({}) Type({}) "
                                 "PldmDumpType({})",
                                 dumpId, dumpSize, dumpType, pldmDumpType)
                         .c_str());

    openpower::dump::pldm::ackNewDump(
        dumpId, static_cast<pldm_fileio_file_type>(pldmDumpType), dumpSize);
}
} // namespace openpower::dump::pldm
