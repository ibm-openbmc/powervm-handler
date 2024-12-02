// SPDX-License-Identifier: Apache-2.0
#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>
#include <libpldm/base.h>
#include <libpldm/instance-id.h>
#include <libpldm/pldm.h>
#include <libpldm/transport/mctp-demux.h>
#include <poll.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/log.hpp>
#include <pldm_utils.hpp>

namespace openpower::dump::pldm
{
using namespace phosphor::logging;
namespace internal
{
std::string getService(sdbusplus::bus::bus& bus, const std::string& path,
                       const std::string& interface)
{
    using namespace phosphor::logging;
    constexpr auto objectMapperName = "xyz.openbmc_project.ObjectMapper";
    constexpr auto objectMapperPath = "/xyz/openbmc_project/object_mapper";

    auto method = bus.new_method_call(objectMapperName, objectMapperPath,
                                      objectMapperName, "GetObject");

    method.append(path);
    method.append(std::vector<std::string>({interface}));

    std::vector<std::pair<std::string, std::vector<std::string>>> response;

    try
    {
        auto reply = bus.call(method);
        reply.read(response);
        if (response.empty())
        {
            log<level::ERR>(fmt::format("Error in mapper response for getting "
                                        "service name, PATH({}), INTERFACE({})",
                                        path, interface)
                                .c_str());
            return std::string{};
        }
    }
    catch (const sdbusplus::exception::exception& e)
    {
        log<level::ERR>(fmt::format("Error in mapper method call, "
                                    "errormsg({}), PATH({}), INTERFACE({})",
                                    e.what(), path, interface)
                            .c_str());
        return std::string{};
    }
    return response[0].first;
}
} // namespace internal

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

void PLDMInstanceManager::initPLDMInstanceIdDb()
{
    auto rc = pldm_instance_db_init_default(&pldmInstanceIdDb);
    if (rc)
    {
        lg2::error("Error calling pldm_instance_db_init_default, rc = {RC}",
                   "RC", rc);
        elog<NotAllowed>(Reason("Failed to init PLDM instance ID"));
    }
}

void PLDMInstanceManager::destroyPLDMInstanceIdDb()
{
    auto rc = pldm_instance_db_destroy(pldmInstanceIdDb);
    if (rc)
    {
        lg2::error("pldm_instance_db_destroy failed rc = {RC}", "RC", rc);
    }
}

int openPLDM(mctp_eid_t eid)
{
    auto fd = -1;
    if (pldmTransport)
    {
        lg2::error("open: pldmTransport already setup!");
        elog<NotAllowed>(Reason("pldmTransport already setup"));
        return fd;
    }

    fd = openMctpDemuxTransport(eid);
    if (fd < 0)
    {
        auto e = errno;
        lg2::error("openMctpDemuxTransport failed, errno: {ERRNO}, FD: FD",
                   "ERRNO", e, "FD", fd);
        elog<NotAllowed>(Reason("Failed to opem MCTP demux transport"));
    }
    return fd;
}

int openMctpDemuxTransport(mctp_eid_t eid)
{
    int rc = pldm_transport_mctp_demux_init(&mctpDemux);
    if (rc)
    {
        lg2::error(
            "openMctpDemuxTransport: Failed to init MCTP demux transport. rc = {RC}",
            "RC", rc);
        return rc;
    }

    rc = pldm_transport_mctp_demux_map_tid(mctpDemux, eid, eid);
    if (rc)
    {
        lg2::error(
            "openMctpDemuxTransport: Failed to setup tid to eid mapping. rc = {RC}",
            "RC", rc);
        pldmClose();
        return rc;
    }
    pldmTransport = pldm_transport_mctp_demux_core(mctpDemux);

    struct pollfd pollfd;
    rc = pldm_transport_mctp_demux_init_pollfd(pldmTransport, &pollfd);
    if (rc)
    {
        lg2::error("openMctpDemuxTransport: Failed to get pollfd. rc = {RC}",
                   "RC", rc);
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

pldm_instance_id_t getPLDMInstanceID(uint8_t tid)
{
    pldm_instance_id_t instanceID = 0;

    auto rc = pldm_instance_id_alloc(pldmInstanceIdDb, tid, &instanceID);
    if (rc == -EAGAIN)
    {
        lg2::info(
            "Failed to get instance id trying again after 100ms, rc = {RC}",
            "RC", rc);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        rc = pldm_instance_id_alloc(pldmInstanceIdDb, tid, &instanceID);
    }

    if (rc)
    {
        lg2::error("Failed to get instance id, rc = {RC}", "RC", rc);
        elog<NotAllowed>(Reason("Failed to get PLDM instance ID"));
    }
    lg2::info("Got instanceId: {INSTANCE_ID} from PLDM eid: {EID}",
              "INSTANCE_ID", instanceID, "EID", tid);

    return instanceID;
}

void freePLDMInstanceID(pldm_instance_id_t instanceID, uint8_t tid)
{
    auto rc = pldm_instance_id_free(pldmInstanceIdDb, tid, instanceID);
    if (rc)
    {
        lg2::error(
            "pldm_instance_id_free failed to free id = {ID} of tid = {TID} rc = {RC}",
            "ID", instanceID, "TID", tid, "RC", rc);
    }
}

} // namespace openpower::dump::pldm
