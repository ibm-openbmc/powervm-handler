#pragma once

#include "dump_dbus_watch.hpp"
#include "dump_utility.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

namespace openpower::dump
{
using ::openpower::dump::utility::DBusInteracesMap;
using ::openpower::dump::utility::ManagedObjectType;

/**
 * @class DumpOffloadHandler
 * @brief Class to offload dumps to PHYP service patition using PLDM
 * @details Fetch all the completed dumps present and initiate PLDM command
 *      to send dump offload requests to server. Add watch to watch on
 *      new dumps created to cater for offload.
 */

class DumpOffloadHandler
{
  public:
    DumpOffloadHandler() = delete;
    virtual ~DumpOffloadHandler() = default;
    DumpOffloadHandler(const DumpOffloadHandler&) = delete;
    DumpOffloadHandler& operator=(const DumpOffloadHandler&) = delete;
    DumpOffloadHandler(DumpOffloadHandler&&) = delete;
    DumpOffloadHandler& operator=(DumpOffloadHandler&&) = delete;

    /**
     * @brief constructor
     * @param[in] bus - D-Bus handle
     * @param[in] entryIntf - entry interface to watch
     * @param[in] dumpType - type of the dump to watch
     */
    DumpOffloadHandler(sdbusplus::bus::bus& bus, const std::string& entryIntf,
                       DumpType dumpType);

    /**
     * @brief Offload dump by sending request to PLDM
     * @param[in] existing dump objects
     */
    void offload(const ManagedObjectType& objects);

  protected:
    /* @brief sdbusplus DBus bus connection. */
    sdbusplus::bus::bus& _bus;

    /* @brief entry interface this object supports */
    const std::string _entryIntf;

    /* @brief dump type this object supports */
    const DumpType _dumpType;

    /* @brief watch on interfaces added/removed and property */
    openpower::dump::DumpDBusWatch _dumpWatch;
};
} // namespace openpower::dump
