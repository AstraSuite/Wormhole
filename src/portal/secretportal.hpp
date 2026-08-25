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

class SecretPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Secret")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit SecretPortal(QObject* parent = nullptr);
    ~SecretPortal() override = default;

    uint version() const { return 1; }

public slots:
    Q_SCRIPTABLE void RetrieveSecret(const QDBusObjectPath& handle,
                                     const QDBusUnixFileDescriptor& fd,
                                     const QVariantMap& options,
                                     const QDBusMessage& message);

private:
    QByteArray getOrCreateMasterKey();
};

} // namespace wormhole::portal
