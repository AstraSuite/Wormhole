#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace wormhole::portal {

class EmailPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Email")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit EmailPortal(QObject* parent = nullptr);
    ~EmailPortal() override = default;

    uint version() const { return 1; }

public slots:
    Q_SCRIPTABLE void ComposeEmail(const QDBusObjectPath& handle,
                                  const QString& app_id,
                                  const QString& parent_window,
                                  const QVariantMap& options,
                                  const QDBusMessage& message);
};

} // namespace wormhole::portal
