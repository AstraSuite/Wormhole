#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include "session.hpp"

namespace wormhole::portal {

class RemoteDesktopPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.RemoteDesktop")
    Q_PROPERTY(uint version READ version CONSTANT)
    Q_PROPERTY(uint AvailableDeviceTypes READ AvailableDeviceTypes CONSTANT)

public:
    enum DeviceType {
        Keyboard = 1,
        Pointer = 2,
        Touchscreen = 4
    };
    Q_ENUM(DeviceType)

    explicit RemoteDesktopPortal(QObject* parent = nullptr);
    ~RemoteDesktopPortal() override = default;

    uint version() const { return 2; }
    uint AvailableDeviceTypes() const { return Keyboard | Pointer | Touchscreen; }

public slots:
    Q_SCRIPTABLE uint CreateSession(const QDBusObjectPath& handle,
                                    const QDBusObjectPath& session_handle,
                                    const QString& app_id,
                                    const QVariantMap& options,
                                    QVariantMap& results);

    Q_SCRIPTABLE uint SelectDevices(const QDBusObjectPath& handle,
                                    const QDBusObjectPath& session_handle,
                                    const QString& app_id,
                                    const QVariantMap& options,
                                    QVariantMap& results);

    Q_SCRIPTABLE void Start(const QDBusObjectPath& handle,
                            const QDBusObjectPath& session_handle,
                            const QString& app_id,
                            const QString& parent_window,
                            const QVariantMap& options,
                            const QDBusMessage& message);

    Q_SCRIPTABLE void NotifyPointerMotion(const QDBusObjectPath& session_handle,
                                          const QVariantMap& options,
                                          double dx,
                                          double dy);

    Q_SCRIPTABLE void NotifyPointerMotionAbsolute(const QDBusObjectPath& session_handle,
                                                  const QVariantMap& options,
                                                  uint stream,
                                                  double x,
                                                  double y);

    Q_SCRIPTABLE void NotifyPointerButton(const QDBusObjectPath& session_handle,
                                          const QVariantMap& options,
                                          int button,
                                          uint state);

    Q_SCRIPTABLE void NotifyPointerAxis(const QDBusObjectPath& session_handle,
                                        const QVariantMap& options,
                                        double dx,
                                        double dy);

    Q_SCRIPTABLE void NotifyPointerAxisDiscrete(const QDBusObjectPath& session_handle,
                                                const QVariantMap& options,
                                                uint axis,
                                                int steps);

    Q_SCRIPTABLE void NotifyKeyboardKeycode(const QDBusObjectPath& session_handle,
                                           const QVariantMap& options,
                                           int keycode,
                                           uint state);

    Q_SCRIPTABLE void NotifyKeyboardKeysym(const QDBusObjectPath& session_handle,
                                          const QVariantMap& options,
                                          int keysym,
                                          uint state);

    Q_SCRIPTABLE void NotifyTouchDown(const QDBusObjectPath& session_handle,
                                      const QVariantMap& options,
                                      uint stream,
                                      uint slot,
                                      double x,
                                      double y);

    Q_SCRIPTABLE void NotifyTouchMotion(const QDBusObjectPath& session_handle,
                                        const QVariantMap& options,
                                        uint stream,
                                        uint slot,
                                        double x,
                                        double y);

    Q_SCRIPTABLE void NotifyTouchUp(const QDBusObjectPath& session_handle,
                                    const QVariantMap& options,
                                    uint slot);

private:
    struct SessionData {
        QString appId;
        uint devices = Keyboard | Pointer;
    };

    QMap<QString, SessionData> m_sessions;
};

} // namespace wormhole::portal
