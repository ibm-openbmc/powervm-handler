// SPDX-License-Identifier: Apache-2.0

#include "pldm_utils.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>
#include <libpldm/base.h>
#include <libpldm/pldm.h>
#include <libpldm/transport.h>
#include <libpldm/transport/mctp-demux.h>
#include <poll.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

namespace openpower::dump::pldm
{

using namespace phosphor::logging;
using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

pldm_instance_db* pldmInstanceIdDb = nullptr;
pldm_transport* pldmTransport = nullptr;
pldm_transport_mctp_demux* mctpDemux = nullptr;

PLDMInstanceManager::PLDMInstanceManager()
{
    initPLDMInstanceIdDb();
}

PLDMInstanceManager::~PLDMInstanceManager()
{
    destroyPLDMInstanceIdDb();
}

void initPLDMInstanceIdDb()
{
    auto rc = pldm_instance_db_init_default(&pldmInstanceIdDb);
    if (rc)
    {
        log<level::ERR>(
            std::format("Error calling pldm_instance_db_init_default RC{}", rc)
                .c_str());
        elog<NotAllowed>(
            Reason("Required host dump action via pldm is not allowed due "
                   "to initPLDMInstanceIdDb failed"));
    }
}

void destroyPLDMInstanceIdDb()
{
    auto rc = pldm_instance_db_destroy(pldmInstanceIdDb);
    if (rc)
    {
        log<level::ERR>(
            std::format("pldm_instance_db_destroy failed RC{}", rc).c_str());
    }
}

pldm_instance_id_t getPLDMInstanceID(uint8_t tid)
{
    pldm_instance_id_t instanceID = 0;

    auto rc = pldm_instance_id_alloc(pldmInstanceIdDb, tid, &instanceID);
    if (rc == -EAGAIN)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        rc = pldm_instance_id_alloc(pldmInstanceIdDb, tid, &instanceID);
    }

    if (rc)
    {
        log<level::ERR>(
            std::format(
                "getPLDMInstanceID: Failed to alloc ID for TID {}. RC{}", tid,
                rc)
                .c_str());
        elog<NotAllowed>(
            Reason("Failure in communicating with libpldm service, "
                   "service may not be running"));
    }
    log<level::INFO>(
        std::format("got tid {} and set instanceID to {}", tid, instanceID)
            .c_str());

    return instanceID;
}

void freePLDMInstanceID(pldm_instance_id_t instanceID, uint8_t tid)
{
    auto rc = pldm_instance_id_free(pldmInstanceIdDb, tid, instanceID);
    if (rc)
    {
        log<level::ERR>(
            std::format(
                "freePLDMInstanceID: Failed to free ID {} for TID {}. RC{}",
                instanceID, tid, rc)
                .c_str());
    }
}

int openPLDM(mctp_eid_t eid)
{
    auto fd = -1;
    if (pldmTransport)
    {
        log<level::ERR>("openPLDM: pldmTransport already setup!");
        elog<NotAllowed>(
            Reason("Required host dump action via pldm is not allowed because "
                   "pldmTransport already setup"));
        return fd;
    }
    fd = openMctpDemuxTransport(eid);
    if (fd < 0)
    {
        auto e = errno;
        log<level::ERR>(
            fmt::format("openPLDM failed, errno({}), FD({})", e, fd).c_str());
        elog<NotAllowed>(
            Reason("Required host dump action via pldm is not allowed due "
                   "to openPLDM failed"));
    }
    return fd;
}

int openMctpDemuxTransport(mctp_eid_t eid)
{
    int rc = pldm_transport_mctp_demux_init(&mctpDemux);
    if (rc)
    {
        log<level::ERR>(std::format("openMctpDemuxTransport: Failed to init "
                                    "MCTP demux transport, errno={}/{}",
                                    rc, strerror(rc))
                            .c_str());
        return rc;
    }

    rc = pldm_transport_mctp_demux_map_tid(mctpDemux, eid, eid);
    if (rc)
    {
        log<level::ERR>(std::format("openMctpDemuxTransport: Failed to setup "
                                    "tid to eid mapping, errno={}/{}",
                                    errno, strerror(errno))
                            .c_str());
        pldmClose();
        return rc;
    }
    pldmTransport = pldm_transport_mctp_demux_core(mctpDemux);

    struct pollfd pollfd;
    rc = pldm_transport_mctp_demux_init_pollfd(pldmTransport, &pollfd);
    if (rc)
    {
        log<level::ERR>(
            std::format(
                "openMctpDemuxTransport: Failed to get pollfd , errno={}/{}",
                errno, strerror(errno))
                .c_str());
        pldmClose();
        return rc;
    }
    return pollfd.fd;
}

void pldmClose()
{
    pldm_transport_mctp_demux_destroy(mctpDemux);
    mctpDemux = nullptr;
    pldmTransport = nullptr;
}

} // namespace openpower::dump::pldm
