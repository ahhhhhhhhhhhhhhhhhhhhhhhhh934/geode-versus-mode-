#pragma once

#include <Geode/Geode.hpp>
#include <globed/soft-link/API.hpp>

using namespace geode::prelude;

// Thin, honest wrappers around Globed's public Soft Link API. Every call is
// guarded so an unavailable or outdated Globed never crashes Versus, and
// each helper documents exactly what it does and does not guarantee.
namespace Versus {
    inline geode::Mod* getGlobedMod() {
        return geode::Loader::get()->getInstalledMod("dankmeme.globed2");
    }

    inline bool globedInstalled() {
        return getGlobedMod() != nullptr;
    }

    inline bool globedEnabled() {
        auto* mod = getGlobedMod();
        return mod && mod->isOrWillBeEnabled();
    }

    inline bool globedLoaded() {
        auto* mod = getGlobedMod();
        return mod && mod->isOrWillBeEnabled() &&
            geode::Loader::get()->isModLoaded("dankmeme.globed2");
    }

    // Probe only the public Soft Link API. Keep the exact dependency version
    // visible in diagnostics so an unavailable table is not misdiagnosed as
    // a network disconnect.
    inline bool globedApiAvailable() {
        if (!globedLoaded()) return false;
        const bool available = globed::api::available();
        static bool loggedUnavailable = false;
        if (!available && !loggedUnavailable) {
            loggedUnavailable = true;
            auto* mod = getGlobedMod();
            auto version = mod ? mod->getVersion().toVString() : std::string("not installed");
            log::warn("Versus: Globed {} is loaded, but getRootTable() did not return an API table. This is an API-export/loader issue, not a server connection state.", version);
        }
        if (available) loggedUnavailable = false;
        return available;
    }

    inline bool globedConnected() {
        return globedApiAvailable() && globed::api::net::isConnected();
    }

    inline bool globedInRoom() {
        return globedApiAvailable() && globed::api::room::isInRoom();
    }

    inline uint32_t globedRoomId() {
        return globedApiAvailable() && globedInRoom()
            ? globed::api::room::getId() : 0;
    }

    inline bool globedReady() {
        return globedLoaded() && globedApiAvailable() &&
            globedConnected();
    }

    inline char const* globedConnectionStateName() {
        if (!globedLoaded()) return "GLOBED NOT LOADED";
        switch (globed::api::net::getConnectionState()) {
            case globed::ConnectionState::Disconnected: return "DISCONNECTED";
            case globed::ConnectionState::DnsResolving: return "DNS RESOLVING";
            case globed::ConnectionState::Pinging: return "PINGING";
            case globed::ConnectionState::Connecting: return "CONNECTING";
            case globed::ConnectionState::Authenticating: return "AUTHENTICATING";
            case globed::ConnectionState::Connected: return "CONNECTED";
            case globed::ConnectionState::Closing: return "CLOSING";
            case globed::ConnectionState::Reconnecting: return "RECONNECTING";
            default: return "UNKNOWN";
        }
    }

    // One-line status used by every page's network banner.
    inline char const* globedStatusLine() {
        return !globedInstalled() ? "GLOBED NOT INSTALLED" :
            (!globedEnabled() ? "GLOBED DISABLED" :
            (!globedLoaded() ? "GLOBED STARTING" :
            globedConnectionStateName()));
    }
}
