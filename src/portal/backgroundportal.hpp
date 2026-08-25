#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace wormhole::portal {

class BackgroundPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Background")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit BackgroundPortal(QObject* parent = nullptr);
    ~BackgroundPortal() override = default;

    uint version() const { return 1; }

public slots:
    Q_SCRIPTABLE void GetAppState(const QDBusMessage& message);

    Q_SCRIPTABLE void NotifyBackground(const QDBusObjectPath& handle,
                                       const QString& app_id,
                                       const QString& name,
                                       const QDBusMessage& message);

    Q_SCRIPTABLE void SetStatus(const QDBusObjectPath& handle,
                                const QString& app_id,
                                const QVariantMap& options,
                                const QDBusMessage& message);
};

} // namespace wormhole::portal
