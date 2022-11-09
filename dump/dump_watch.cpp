#include "config.h"

#include "dump_watch.hpp"

#include "dbus_util.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

namespace openpower::dump
{
using ::openpower::dump::utility::DBusInteracesList;
using ::openpower::dump::utility::DBusInteracesMap;
using ::openpower::dump::utility::DBusPropertiesMap;
using ::phosphor::logging::level;
using ::phosphor::logging::log;
using ::sdbusplus::bus::match::rules::sender;

DumpWatch::DumpWatch(sdbusplus::bus::bus& bus, HostOffloaderQueue& dumpQueue,
                     const std::string& entryObjPath, DumpType dumpType) :
    _bus(bus),
    _dumpQueue(dumpQueue), _dumpType(dumpType)
{
    _intfAddWatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::interfacesAdded() +
            sdbusplus::bus::match::rules::argNpath(0, entryObjPath),
        [this](auto& msg) { this->interfaceAdded(msg); });

    _intfRemWatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::interfacesRemoved() +
            sdbusplus::bus::match::rules::argNpath(0, entryObjPath),
        [this](auto& msg) { this->interfaceRemoved(msg); });
}

void DumpWatch::interfaceAdded(sdbusplus::message::message& msg)
{
    try
    {
        sdbusplus::message::object_path objPath;
        DBusInteracesMap interfaces;
        msg.read(objPath, interfaces);
        log<level::INFO>(
            fmt::format("Watch interfaceAdded path ({})", objPath.str).c_str());

        // check if dump generation is already completed
        bool isComplete = false;
        auto iface = interfaces.find(progressIntf);
        if (iface != interfaces.end())
        {
            auto prop = iface->second.find("Status");
            if (prop != iface->second.end())
            {
                auto status = std::get_if<std::string>(&prop->second);
                if (status != nullptr)
                {
                    if (*status == progressComplete)
                    {
                        isComplete = true;
                    }
                }
            }
        }
        if (isComplete)
        {
            // queue the dump for offloading
            _dumpQueue.enqueue(objPath, _dumpType);
        }
        else
        {
            _entryPropWatchList.emplace(
                objPath, std::make_unique<sdbusplus::bus::match_t>(
                             _bus,
                             sdbusplus::bus::match::rules::propertiesChanged(
                                 objPath.str, progressIntf),
                             [this, objPath](auto& msg) {
                                 this->propertiesChanged(objPath, msg);
                             }));
        }
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Watch exception in interfaceAdded ({})", ex.what())
                .c_str());
        throw;
    }
}

void DumpWatch::interfaceRemoved(sdbusplus::message::message& msg)
{
    try
    {
        sdbusplus::message::object_path objPath;
        DBusInteracesList interfaces;
        msg.read(objPath, interfaces);
        log<level::INFO>(
            fmt::format("Watch interfaceRemoved path ({})", objPath.str)
                .c_str());

        _dumpQueue.dequeue(objPath);
        _entryPropWatchList.erase(objPath);
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Watch exception in interfaceRemoved ({})", ex.what())
                .c_str());
        throw;
    }
}

void DumpWatch::propertiesChanged(const object_path& objPath,
                                  sdbusplus::message::message& msg)
{
    try
    {
        std::string interface;
        DBusPropertiesMap propMap;
        msg.read(interface, propMap);
        log<level::INFO>(
            fmt::format("Watch propertiesChanged object path ({})", objPath.str)
                .c_str());

        bool fcomplete = isDumpProgressCompleted(propMap);
        if (!fcomplete)
        {
            log<level::DEBUG>(
                fmt::format("Watch propertiesChanged object path ({}) "
                            "status is not completed",
                            objPath.str)
                    .c_str());
            return;
        }

        // queue the dump for offloading
        _dumpQueue.enqueue(objPath, _dumpType);

        _entryPropWatchList.erase(objPath);
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Watch exception in propertiesChanged ({})", ex.what())
                .c_str());
        throw;
    }
}

void DumpWatch::addInProgressDumpsToWatch(std::vector<std::string> paths)
{
    try
    {
        for (auto& path : paths)
        {
            object_path objPath = path;
            _entryPropWatchList.emplace(
                objPath, std::make_unique<sdbusplus::bus::match_t>(
                             _bus,
                             sdbusplus::bus::match::rules::propertiesChanged(
                                 objPath.str, progressIntf),
                             [this, objPath](auto& msg) {
                                 this->propertiesChanged(objPath, msg);
                             }));
        }
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Watch exception in addInProgressDumpsToWatch ({})",
                        ex.what())
                .c_str());
        throw;
    }
}

} // namespace openpower::dump
