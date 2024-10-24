#include "pldm_oem_cmds.hpp"

#include "pldm_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>
#include <libpldm/base.h>
#include <libpldm/platform.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <fstream>

namespace openpower::dump::pldm
{
using namespace phosphor::logging;

constexpr auto eidPath = "/usr/share/pldm/host_eid";
constexpr mctp_eid_t defaultEIDValue = 9;

PLDMInstanceManager instanceManager;
using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

namespace internal
{
mctp_eid_t readEID()
{
    mctp_eid_t eid(defaultEIDValue);

    std::ifstream eidFile{eidPath};
    if (!eidFile.good())
    {
        log<level::ERR>("Could not open host EID file");
        elog<NotAllowed>(Reason("Required host dump action via pldm is not "
                                "allowed due to mctp end point read failed"));
    }
    else
    {
        std::string strEid;
        eidFile >> strEid;
        if (!strEid.empty())
        {
            eid = strtol(strEid.c_str(), nullptr, 10);
        }
        else
        {
            log<level::ERR>("EID file was empty");
            elog<NotAllowed>(Reason(
                "Required host dump action via pldm is not "
                "allowed due to mctp end point read failed"));
        }
    }

    return eid;
}
} // namespace internal

void newFileAvailable(uint32_t dumpId, pldm_fileio_file_type pldmDumpType,
                      uint64_t dumpSize)
{
    const size_t pldmMsgHdrSize = sizeof(pldm_msg_hdr);
    std::array<uint8_t, pldmMsgHdrSize + PLDM_NEW_FILE_REQ_BYTES>
        newFileAvailReqMsg;

    mctp_eid_t mctpEndPointId = internal::readEID();

    auto pldmInstanceId = getPLDMInstanceID(mctpEndPointId);
    log<level::INFO>(
        fmt::format("encode_new_file_req Instance ID ({}) "
                    "DumpID ({}) DumpType ({}) DumpSize({})  ReqMsgSize({})",
                    pldmInstanceId, dumpId, static_cast<uint16_t>(pldmDumpType),
                    dumpSize, newFileAvailReqMsg.size())
            .c_str());
    int retCode = encode_new_file_req(
        pldmInstanceId, pldmDumpType, dumpId, dumpSize,
        reinterpret_cast<pldm_msg*>(newFileAvailReqMsg.data()));
    if (retCode != PLDM_SUCCESS)
    {
        freePLDMInstanceID(pldmInstanceId, mctpEndPointId);
        log<level::ERR>(
            fmt::format(
                "Failed to encode pldm New file req for new dump available "
                "dumpId({}), pldmDumpType({}),rc({})",
                dumpId, static_cast<uint16_t>(pldmDumpType), retCode)
                .c_str());
        elog<NotAllowed>(Reason(
            "Acknowledging new file request failed due to encoding error"));
    }

    internal::CustomFd pldmFd = openPLDM();
    if (pldmFd() < 0)
    {
        freePLDMInstanceID(pldmInstanceId, mctpEndPointId);
        log<level::ERR>(
            fmt::format("Failed to openPLDM for new dump available "
                        "dumpId({}), pldmDumpType({}),rc({})",
                        dumpId, static_cast<uint16_t>(pldmDumpType), retCode)
                .c_str());
        elog<NotAllowed>(
            Reason("Failed to open PLDM for new dump available request"));
    }

    retCode = pldm_send(mctpEndPointId, pldmFd(), newFileAvailReqMsg.data(),
                        newFileAvailReqMsg.size());
    if (retCode != PLDM_REQUESTER_SUCCESS)
    {
        freePLDMInstanceID(pldmInstanceId, mctpEndPointId);
        auto errorNumber = errno;
        log<level::ERR>(
            fmt::format(
                "Failed to send pldm new file request for new dump available, "
                "dumpId({}), pldmDumpType({}), "
                "rc({}), errno({}), errmsg({})",
                dumpId, static_cast<uint16_t>(pldmDumpType), retCode,
                errorNumber, strerror(errorNumber))
                .c_str());
        elog<NotAllowed>(Reason("New file available  via pldm is not "
                                "allowed due to new file request send failed"));
    }
    freePLDMInstanceID(pldmInstanceId, mctpEndPointId);
    lg2::info("Done. PLDM message, id: {ID}, RC: {RC}", "ID", pldmInstanceId,
              "RC", retCode);
}
} // namespace openpower::dump::pldm
