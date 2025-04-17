#include "send_pldm_cmd.hpp"

#include "pldm_oem_cmds.hpp"

#include <fmt/format.h>
#include <libpldm/oem/ibm/file_io.h>

#include <phosphor-logging/log.hpp>

#include <format>

namespace openpower::dump::pldm
{
using ::phosphor::logging::level;
using ::phosphor::logging::log;

void sendNewDumpCmd(uint32_t dumpId, DumpType dumpType, uint64_t dumpSize)
{
    uint32_t pldmDumpType = 0;
    std::string dumpIdString = std::format("{:0>8X}", dumpId);
    // TODO https://github.com/ibm-openbmc/powervm-handler/issues/9
    switch (dumpType)
    {
        case DumpType::bmc:
            pldmDumpType = 0xF; // PLDM_FILE_TYPE_BMC_DUMP
            break;
        case DumpType::system:
            if (dumpIdString.c_str()[0] == '3' ||
                dumpIdString.c_str()[0] == '4')
                pldmDumpType = 0x10; // PLDM_FILE_TYPE_SBE_DUMP
            else if (dumpIdString.c_str()[0] == '2')
                pldmDumpType = 0x11; // PLDM_FILE_TYPE_HOSTBOOT_DUMP
            else if (dumpIdString.c_str()[0] == '0')
                pldmDumpType = 0x12; // PLDM_FILE_TYPE_HARDWARE_DUMP
            break;
        default:
            std::string err = fmt::format("Unsupported dump type ({}) ",
                                          static_cast<uint32_t>(dumpType));
            log<level::ERR>(err.c_str());
            throw std::out_of_range(err);
            break;
    }

    log<level::INFO>(fmt::format("sendNewDumpCmd Id({}) Size({}) Type({}) "
                                 "PldmDumpType({})",
                                 dumpId, dumpSize,
                                 static_cast<uint32_t>(dumpType), pldmDumpType)
                         .c_str());
    openpower::dump::pldm::newFileAvailable(
        dumpId, static_cast<pldm_fileio_file_type>(pldmDumpType), dumpSize);
}
} // namespace openpower::dump::pldm
