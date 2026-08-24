#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "../config/colours.hpp"

namespace wormhole::portal {

class SettingsPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Settings")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit SettingsPortal(QObject* parent = nullptr);
    ~SettingsPortal() override = default;

    uint version() const { return 1; }

public slots:
    Q_SCRIPTABLE void Read(const QString& namesp,
                          const QString& key,
                          const QDBusMessage& message);

    Q_SCRIPTABLE void ReadAll(const QStringList& namespaces,
                             const QDBusMessage& message);

signals:
    Q_SCRIPTABLE void SettingChanged(const QString& namesp, const QString& key, const QDBusVariant& value);

private:
    QVariant readKey(const QString& namesp, const QString& key);
};

} // namespace wormhole::portal
