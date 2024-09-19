#pragma once

#include "hmc_state_watch.hpp"
#include "host_offloader_queue.hpp"
#include "host_state_watch.hpp"
#include "offload_handler.hpp"

#include <sdbusplus/bus.hpp>
#include <sdeventplus/source/event.hpp>

#include <memory>

namespace openpower::dump
{
/**
 * @class OffloadManager
 * @brief To offload dumps to PHYP service parition by using PLDM commands
 * @details Retrieves all the suported dumps entries and initiates PLDM
 *         request to offload the dumps to PHYP service partition
 */
class OffloadManager
{
  public:
    OffloadManager() = delete;
    OffloadManager(const OffloadManager&) = delete;
    OffloadManager& operator=(const OffloadManager&) = delete;
    OffloadManager(OffloadManager&&) = delete;
    OffloadManager& operator=(OffloadManager&&) = delete;
    virtual ~OffloadManager() = default;

    /**
     * @brief Constructor
     * @param[in] bus - D-Bus to attach to.
     * @param[in] event - event handler
     */
    OffloadManager(sdbusplus::bus::bus& bus, sdeventplus::Event& event);

    /**
     * @brief Offload dumps existing on the system by sending PLDM request
     */
    void offload();

  private:
    /** @brief D-Bus to connect to */
    sdbusplus::bus::bus& _bus;

    /** @brief Queue to offload dump requests */
    HostOffloaderQueue _dumpQueue;

    /*@brief list of dump offload objects */
    std::vector<std::unique_ptr<OffloadHandler>> _offloadHandlerList;

    /*@brief watch for host state change */
    HostStateWatch _hostStateWatch;

    /*@brief watch for HMC state change */
    HMCStateWatch _hmcStateWatch;
};
} // namespace openpower::dump
