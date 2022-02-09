#include "config.h"

#include "dump_dbus_watch.hpp"

#include "dump_dbus_util.hpp"
#include "dump_send_pldm_cmd.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

namespace openpower::dump
{
using ::openpower::dump::utility::DBusInteracesMap;
using ::openpower::dump::utility::DBusPropertiesMap;
using ::openpower::dump::utility::ManagedObjectType;
using ::phosphor::logging::level;
using ::phosphor::logging::log;
using ::sdbusplus::bus::match::rules::sender;

DumpDBusWatch::DumpDBusWatch(sdbusplus::bus::bus& bus,
                             const std::string& entryIntf, DumpType dumpType) :
    _bus(bus),
    _entryIntf(entryIntf), _dumpType(dumpType),
    _intfAddWatch(std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::interfacesAdded() + sender(dumpService),
        [this](auto& msg) { this->interfaceAdded(msg); })),
    _intfRemWatch(std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::interfacesRemoved() + sender(dumpService),
        [this](auto& msg) { this->interfaceRemoved(msg); }))
{
}

void DumpDBusWatch::interfaceAdded(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path objPath;
    DBusInteracesMap interfaces;
    msg.read(objPath, interfaces);
    log<level::INFO>(
        fmt::format("interfaceAdded path ({})", objPath.str).c_str());
    auto iter = interfaces.find(_entryIntf);
    if (iter == interfaces.end())
    {
        // ignore not specific to the dump type being watched
        return;
    }
    uint32_t id = std::stoul(objPath.filename());
    _entryPropWatchList.emplace(
        objPath, std::make_unique<sdbusplus::bus::match_t>(
                     _bus,
                     sdbusplus::bus::match::rules::propertiesChanged(
                         objPath, progressIntf),
                     [this, objPath, id](auto& msg) {
                         this->propertiesChanged(objPath, id, msg);
                     }));
}

void DumpDBusWatch::interfaceRemoved(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path objPath;
    DBusInteracesMap interfaces;
    msg.read(objPath, interfaces);
    log<level::INFO>(
        fmt::format("interfaceRemoved path ({})", objPath.str).c_str());
    auto iter = interfaces.find(_entryIntf);
    if (iter == interfaces.end())
    {
        // ignore not specific to the dump type being watched
        return;
    }
    _entryPropWatchList.erase(objPath);
}

void DumpDBusWatch::propertiesChanged(const object_path& objPath, uint32_t id,
                                      sdbusplus::message::message& msg)
{
    std::string interface;
    DBusPropertiesMap propMap;
    msg.read(interface, propMap);
    log<level::INFO>(fmt::format("propertiesChanged object path ({}) id ({})",
                                 objPath.str, id)
                         .c_str());

    bool fcomplete = isDumpProgressCompleted(propMap);
    if (!fcomplete)
    {
        log<level::DEBUG>(
            fmt::format("propertiesChanged object path ({}) status is not "
                        "complete",
                        objPath.str)
                .c_str());
        return;
    }

    uint64_t size = getDumpSize(_bus, objPath.str);
    openpower::dump::pldm::sendNewDumpCmd(id, _dumpType, size);

    _entryPropWatchList.erase(objPath);
}

void DumpDBusWatch::addInProgressDumpsToWatch(const ManagedObjectType& objects)
{
    for (auto& object : objects)
    {
        object_path objectPath = object.first;
        auto iter = object.second.find(_entryIntf);
        if (iter == object.second.end())
        {
            continue;
        }
        bool fcomplete = isDumpProgressCompleted(object.second);
        if (!fcomplete)
        {
            uint32_t id = std::stoul(object.first.filename());
            log<level::INFO>(
                fmt::format("addInProgressDumpsToWatch object path ({})",
                            objectPath.str)
                    .c_str());
            _entryPropWatchList.emplace(
                objectPath, std::make_unique<sdbusplus::bus::match_t>(
                                _bus,
                                sdbusplus::bus::match::rules::propertiesChanged(
                                    objectPath, progressIntf),
                                [this, objectPath, id](auto& msg) {
                                    this->propertiesChanged(objectPath, id,
                                                            msg);
                                }));
        }
    }
}

} // namespace openpower::dump
