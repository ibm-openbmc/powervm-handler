// SPDX-License-Identifier: Apache-2.0

#include "xyz/openbmc_project/Common/error.hpp"

#include <fmt/core.h>
#include <libpldm/base.h>
#include <libpldm/pldm.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

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

int openPLDM()
{
    auto fd = pldm_open();
    if (fd < 0)
    {
        auto e = errno;
        log<level::ERR>(fmt::format("pldm_open failed, errno({}), FD({})", e,
                                    static_cast<int>(fd))
                            .c_str());
        elog<NotAllowed>(Reason("Required host dump action via pldm is not "
                                "allowed due to pldm_open failed"));
    }
    return fd;
}

uint8_t getPLDMInstanceID(uint8_t eid)
{
    constexpr auto pldmRequester = "xyz.openbmc_project.PLDM.Requester";
    constexpr auto pldm = "/xyz/openbmc_project/pldm";

    auto bus = sdbusplus::bus::new_default();
    auto service = internal::getService(bus, pldm, pldmRequester);

    auto method = bus.new_method_call(service.c_str(), pldm, pldmRequester,
                                      "GetInstanceId");
    method.append(eid);
    auto reply = bus.call(method);

    uint8_t instanceID = 0;
    reply.read(instanceID);

    return instanceID;
}
} // namespace openpower::dump::pldm
