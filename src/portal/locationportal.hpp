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

class LocationPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Location")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit LocationPortal(QObject* parent = nullptr);
    ~LocationPortal() override = default;

    uint version() const { return 1; }

public slots:
    Q_SCRIPTABLE uint CreateSession(const QDBusObjectPath& handle,
                                    const QDBusObjectPath& session_handle,
                                    const QString& app_id,
                                    const QVariantMap& options,
                                    QVariantMap& results);

    Q_SCRIPTABLE uint SelectAccuracy(const QDBusObjectPath& handle,
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

signals:
    Q_SCRIPTABLE void LocationUpdated(const QDBusObjectPath& session_handle,
                                      const QVariantMap& location);

private:
    struct SessionData {
        QString appId;
        uint accuracy = 0;
    };

    QMap<QString, SessionData> m_sessions;
};

} // namespace wormhole::portal
