#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace wormhole::portal {

class UsbPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Usb")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit UsbPortal(QObject* parent = nullptr);
    ~UsbPortal() override = default;

    uint version() const { return 1; }

public slots:
    Q_SCRIPTABLE void ClaimDevice(const QDBusObjectPath& handle,
                                  const QString& app_id,
                                  const QString& parent_window,
                                  const QVariantMap& options,
                                  const QDBusMessage& message);

    Q_SCRIPTABLE void OpenDevice(const QDBusObjectPath& handle,
                                 const QString& app_id,
                                 const QString& parent_window,
                                 const QVariantMap& options,
                                 const QDBusMessage& message);

    Q_SCRIPTABLE void ReleaseDevice(const QDBusObjectPath& handle,
                                   const QString& app_id,
                                   const QVariantMap& options,
                                   const QDBusMessage& message);
};

} // namespace wormhole::portal
