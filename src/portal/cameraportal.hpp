#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include "../screencast/pipewirestream.hpp"

namespace wormhole::portal {

class CameraPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Camera")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit CameraPortal(QObject* parent = nullptr);
    ~CameraPortal() override = default;

    uint version() const { return 1; }

public slots:
    Q_SCRIPTABLE void AccessCamera(const QDBusObjectPath& handle,
                                  const QString& app_id,
                                  const QString& parent_window,
                                  const QVariantMap& options,
                                  const QDBusMessage& message);

    Q_SCRIPTABLE void OpenPipeWireRemote(const QDBusObjectPath& handle,
                                        const QString& app_id,
                                        const QVariantMap& options,
                                        const QDBusMessage& message);
};

} // namespace wormhole::portal
