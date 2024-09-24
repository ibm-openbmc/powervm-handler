#pragma once

#include "utility.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

#include <cstdint>

namespace openpower::dump
{
using ::openpower::dump::utility::DBusPropertiesMap;
using ::phosphor::logging::level;
using ::phosphor::logging::log;
using ::sdbusplus::message::object_path;

using BaseBIOSTableItem = std::tuple<
    std::string, bool, std::string, std::string, std::string,
    std::variant<int64_t, std::string>, std::variant<int64_t, std::string>,
    std::vector<std::tuple<std::string, std::variant<int64_t, std::string>,
                           std::string>>>;

using BaseBIOSTableItemList = std::map<std::string, BaseBIOSTableItem>;

/**
 * @brief Read progress property from the interface map object
 * @param[in] propMap map of properties and its values
 * @return true if progress is complete else false
 */
bool isDumpProgressCompleted(const DBusPropertiesMap& propMap);

/**
 * @brief Read progress property from the D-Bus object
 * @param[in] bus - D-Bus handle
 * @param[in] objectPath - D-Bus object path
 * @return true if progress is complete else false
 */
bool isDumpProgressCompleted(sdbusplus::bus::bus& bus,
                             const std::string& objectPath);
/**
 * @brief Read dump size from the interface map object
 * @param[in] bus - D-Bus handle
 * @param[in] objectPath - path of the D-Bus entry object
 * @return dump size value
 */
uint64_t getDumpSize(sdbusplus::bus::bus& bus, const std::string& objectPath);

/**
 * @brief Read D-Bus property to check if system is HMC managed
 * @detail Read the property from BIOSConfig.Manager interface, if attribute
 *         is not set it will be assumed system is non HMC managed system.
 *         Assumption is that if it is HMC managed the attribute will be set.
 * @param[in] bus D-Bus handle
 * @return true if HMC managed else false
 */
bool isSystemHMCManaged(sdbusplus::bus::bus& bus);

/**
 * @brief Read property value from the specified object and interface
 * @param[in] bus D-Bus handle
 * @param[in] service service which has implemented the interface
 * @param[in] object object having has implemented the interface
 * @param[in] intf interface having the property
 * @param[in] prop name of the property to read
 * @return property value
 */
template <typename T>
T readDBusProperty(sdbusplus::bus::bus& bus, const std::string& service,
                   const std::string& object, const std::string& intf,
                   const std::string& prop)
{
    T retVal{};
    try
    {
        auto properties =
            bus.new_method_call(service.c_str(), object.c_str(),
                                "org.freedesktop.DBus.Properties", "Get");
        properties.append(intf);
        properties.append(prop);
        auto result = bus.call(properties);
        result.read(retVal);
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Failed to get the property ({}) interface ({}) "
                        "object path ({}) error ({}) ",
                        prop.c_str(), intf.c_str(), object.c_str(), ex.what())
                .c_str());
        throw;
    }
    return retVal;
}

/**
 * @brief Read D-Bus property to check if host is in running state
 * @detail Read the Boot.Progress property to determine if host is running.
 * @param[in] bus D-Bus handle
 * @return true if host is running else false
 */
bool isHostRunning(sdbusplus::bus::bus& bus);

/**
 * @brief Read avaialble dumps implementing the dump type entry interface
 * @param[in] bus D-Bus handle
 * @param[in] entryIntf identifies type of the dump
 * @return D-Bus object paths
 */
const std::vector<std::string> getDumpEntryObjPaths(
    sdbusplus::bus::bus& bus, const std::string& entryIntf);
} // namespace openpower::dump
