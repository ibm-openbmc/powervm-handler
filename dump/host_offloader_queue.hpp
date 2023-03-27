#pragma once

#include "utility.hpp"

#include <map>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/source/event.hpp>
#include <sdeventplus/utility/timer.hpp>

namespace openpower::dump
{
using ::openpower::dump::utility::DumpType;
using ::sdbusplus::message::object_path;
using ::sdeventplus::ClockId::Monotonic;
using ::sdeventplus::utility::Timer;

/**
 * @class HostOffloaderQueue
 * @brief To queue the dump offload requests to be sent to the host.
 * @details PHYP could not handle multiple dump offload requests at the same
 *          time, queueing the requests and sending when one offload is done
 */
class HostOffloaderQueue
{
  public:
    HostOffloaderQueue() = delete;
    HostOffloaderQueue(const HostOffloaderQueue&) = delete;
    HostOffloaderQueue& operator=(const HostOffloaderQueue&) = delete;
    HostOffloaderQueue(HostOffloaderQueue&&) = delete;
    HostOffloaderQueue& operator=(HostOffloaderQueue&&) = delete;
    virtual ~HostOffloaderQueue() = default;

    /**
     * @brief Constructor
     * @param[in] bus - D-Bus to attach to
     * @param[in] event - event handler
     */
    HostOffloaderQueue(sdbusplus::bus::bus& bus, sdeventplus::Event& event);

    /**
     * @brief Queue the dumps for offloading
     * @param[in] path - D-Bus path of the dump object
     * @param[in] type - type of the dump to offload
     */
    void enqueue(const object_path& path, DumpType type);

    /**
     * @brief DeQueue the dump object from offloading
     *        Dequeue can happen after succesfull offload or when dump objects
     *        are deleted by redfish client
     * @param[in] path - D-Bus path of the dump object
     */
    void dequeue(const object_path& path);

    /**
     * @brief Host state change notification form host state watch
     * @param[in] isRunning - True if host is in running state
     */
    void hostStateChange(bool isRunning);

    /**
     * @brief HMC state change notification form HMC state watch
     * @param[in] hmcManaged - True if system is HMC managed
     */
    void hmcStateChange(bool hmcManaged);

  private:
    /**
     * @brief Check the states and start the timer for offloading dumps
     */
    void startTimer();

    /**
     * @brief Stop the timer
     */
    void stopTimer();

    /**
     * @brief Offload the next available dump from the queue
     */
    void offload();

    /** @brief timer expired offload any existing dumps */
    void timerExpired();

    /** @brief D-Bus to connect to */
    sdbusplus::bus::bus& _bus;

    /** @brief sdevent event handle */
    sdeventplus::Event& _event;

    /** @brief map of property change request for the corresponding entry */
    std::map<std::string, DumpType> _offloadDumpList;

    /** @brief dump object currently in offload */
    std::string _offloadObjPath;

    /** @brief Flag set when offload is in progress */
    bool _offloadInProgress = false;

    /** @brief Flag to indicate whether the host is in running state */
    bool isHostRunning = false;

    /** @brief Flag to indicate whether the system is HMC managed */
    bool isHMCManagedSystem = false;
    /**
     * @brief Attempt dump offload at every 5 seconds
     */
    const std::chrono::milliseconds _offloadTimeout;

    /**
     * @brief timer is used as for offload/delete do not want to block
     *  the caller thread. If we get deleteall request, we might assume dump
     *  offloaded and try offload the next dump, as the next entry
     *  might have been deleted we will log error and try with next dump.
     *  This happens as the D-Bus thread blocks all other interfacesRemoved
     *  callbacks.
     */
    Timer<Monotonic> _offloadTimer;
};
} // namespace openpower::dump
