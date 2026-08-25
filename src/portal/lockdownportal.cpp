#include "lockdownportal.hpp"

namespace wormhole::portal {

LockdownPortal::LockdownPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

} // namespace wormhole::portal
