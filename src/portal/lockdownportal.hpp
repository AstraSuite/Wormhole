#pragma once

#include <QDBusAbstractAdaptor>
#include <QObject>

namespace wormhole::portal {

class LockdownPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Lockdown")
    Q_PROPERTY(uint version READ version CONSTANT)
    Q_PROPERTY(bool disable_printing READ disablePrinting CONSTANT)
    Q_PROPERTY(bool disable_save_to_disk READ disableSaveToDisk CONSTANT)
    Q_PROPERTY(bool disable_user_installation READ disableUserInstallation CONSTANT)
    Q_PROPERTY(bool disable_application_handlers READ disableApplicationHandlers CONSTANT)

public:
    explicit LockdownPortal(QObject* parent = nullptr);
    ~LockdownPortal() override = default;

    uint version() const { return 1; }
    bool disablePrinting() const { return false; }
    bool disableSaveToDisk() const { return false; }
    bool disableUserInstallation() const { return false; }
    bool disableApplicationHandlers() const { return false; }
};

} // namespace wormhole::portal
