#pragma once

#include "dump_offload_handler.hpp"

#include <memory>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/source/event.hpp>

namespace openpower::dump
{
/**
 * @class DumpOffloadManager
 * @brief To offload dumps to PHYP service parition by using PLDM commands
 * @details Retrieves all the suported dumps entries and initiates PLDM
 *         request to offload the dumps to PHYP service partition
 */
class DumpOffloadManager
{
  public:
    DumpOffloadManager() = delete;
    DumpOffloadManager(const DumpOffloadManager&) = delete;
    DumpOffloadManager& operator=(const DumpOffloadManager&) = delete;
    DumpOffloadManager(DumpOffloadManager&&) = delete;
    DumpOffloadManager& operator=(DumpOffloadManager&&) = delete;
    virtual ~DumpOffloadManager() = default;

    /**
     * @brief Constructor
     * @param[in] bus - D-Bus to attach to.
     */
    DumpOffloadManager(sdbusplus::bus::bus& bus);

    /**
     * @brief Offload dumps existing on the system by sending PLDM request
     */
    void offload();

  private:
    /** @brief D-Bus to connect to */
    sdbusplus::bus::bus& _bus;

    /*@brief list of dump offload objects */
    std::vector<std::unique_ptr<DumpOffloadHandler>> _dumpOffloadList;
};
} // namespace openpower::dump
