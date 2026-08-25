#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include "session.hpp"

namespace wormhole::portal {

class InputCapturePortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.InputCapture")
    Q_PROPERTY(uint version READ version CONSTANT)
    Q_PROPERTY(uint SupportedCapabilities READ SupportedCapabilities CONSTANT)

public:
    enum Capability {
        Keyboard = 1,
        Pointer = 2,
        Touchscreen = 4
    };
    Q_ENUM(Capability)

    explicit InputCapturePortal(QObject* parent = nullptr);
    ~InputCapturePortal() override = default;

    uint version() const { return 1; }
    uint SupportedCapabilities() const { return Keyboard | Pointer; }

public slots:
    Q_SCRIPTABLE uint CreateSession(const QDBusObjectPath& handle,
                                    const QDBusObjectPath& session_handle,
                                    const QString& app_id,
                                    const QVariantMap& options,
                                    QVariantMap& results);

    Q_SCRIPTABLE uint GetZones(const QDBusObjectPath& session_handle,
                               const QVariantMap& options,
                               QVariantMap& results);

    Q_SCRIPTABLE uint SetPointerBarriers(const QDBusObjectPath& session_handle,
                                        const QVariantMap& options,
                                        const QVariantList& barriers,
                                        uint zone_set,
                                        QVariantMap& results);

    Q_SCRIPTABLE uint Enable(const QDBusObjectPath& session_handle,
                             const QVariantMap& options,
                             QVariantMap& results);

    Q_SCRIPTABLE uint Disable(const QDBusObjectPath& session_handle,
                              const QVariantMap& options,
                              QVariantMap& results);

    Q_SCRIPTABLE uint Release(const QDBusObjectPath& session_handle,
                              const QVariantMap& options,
                              QVariantMap& results);

signals:
    Q_SCRIPTABLE void Disabled(const QDBusObjectPath& session_handle,
                               const QVariantMap& options);

    Q_SCRIPTABLE void Activated(const QDBusObjectPath& session_handle,
                                const QVariantMap& options);

    Q_SCRIPTABLE void Deactivated(const QDBusObjectPath& session_handle,
                                  const QVariantMap& options);

    Q_SCRIPTABLE void ZonesChanged(const QDBusObjectPath& session_handle,
                                   const QVariantMap& options);

private:
    struct SessionData {
        QString appId;
        uint capabilities = Keyboard | Pointer;
        bool enabled = false;
    };

    QMap<QString, SessionData> m_sessions;
};

} // namespace wormhole::portal
