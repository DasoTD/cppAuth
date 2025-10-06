// Minimal shim for laserpants/dotenv used only for local build convenience.
// This is a no-op implementation. Replace with real library for production.
#pragma once

namespace dotenv {
    inline void init() {}
}
