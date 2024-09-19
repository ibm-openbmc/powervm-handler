#pragma once

#include "host_offloader_queue.hpp"
#include "utility.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include <map>
#include <memory>

namespace openpower::dump
{

using ::openpower::dump::utility::DumpType;
using ::sdbusplus::message::object_path;

/**
 * @class DumpWatch
 * @brief Add watch on new dump entries created/deleted so as to offload
 * @details Adds watch on the dump progress property for the newly created
 *  dumps. Initiates offload when dump progress property is changed to complete
 */
class DumpWatch
{
  public:
    DumpWatch() = delete;
    DumpWatch(const DumpWatch&) = delete;
    DumpWatch& operator=(const DumpWatch&) = delete;
    DumpWatch(DumpWatch&&) = delete;
    DumpWatch& operator=(DumpWatch&&) = delete;
    virtual ~DumpWatch() = default;

    /**
     * @brief Watch on new dump objects created and property change
     * @param[in] bus - Bus to attach to
     * @param[in] dumpQueue - To queue and offload dump
     * @param[in] entryObjPath - dump entry object path
     * @param[in] dumpType - dump type to watch
     */
    DumpWatch(sdbusplus::bus::bus& bus, HostOffloaderQueue& dumpQueue,
              const std::string& entryObjPath, DumpType dumpType);

    /**
     * @brief Add all in progress dumps to property watch
     * @param[in] objects dump object paths
     * @return void
     */
    void addInProgressDumpsToWatch(std::vector<std::string> paths);

  private:
    /**
     * @brief Callback method for creation of dump entry object
     * @param[in] msg response msg from D-Bus request
     * @return void
     */
    void interfaceAdded(sdbusplus::message::message& msg);

    /**
     * @brief Callback method for deletion of dump entry object
     * @param[in] msg response msg from D-Bus request
     * @return void
     */
    void interfaceRemoved(sdbusplus::message::message& msg);

    /**
     * @brief Callback method for property change on the entry object
     * @param[in] objPath Object path of the dump entry
     * @param[in] msg response msg from D-Bus request
     * @return void
     */
    void propertiesChanged(const object_path& objPath,
                           sdbusplus::message::message& msg);

    /** @brief D-Bus to connect to */
    sdbusplus::bus::bus& _bus;

    /** @brief Queue to offload dump requests */
    HostOffloaderQueue& _dumpQueue;

    /** @brief type of the dump to watch for */
    DumpType _dumpType;

    /** @brief watch pointer for interfaces added */
    std::unique_ptr<sdbusplus::bus::match_t> _intfAddWatch;

    /** @brief watch pointer for interfaces removed */
    std::unique_ptr<sdbusplus::bus::match_t> _intfRemWatch;

    /** @brief map of property change request for the corresponding entry */
    std::map<object_path, std::unique_ptr<sdbusplus::bus::match_t>>
        _entryPropWatchList;
};
} // namespace openpower::dump
