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

class PrintPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Print")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit PrintPortal(QObject* parent = nullptr);
    ~PrintPortal() override = default;

    uint version() const { return 2; }

public slots:
    Q_SCRIPTABLE void Print(const QDBusObjectPath& handle,
                            const QString& app_id,
                            const QString& parent_window,
                            const QString& title,
                            const QDBusUnixFileDescriptor& fd,
                            const QVariantMap& options,
                            const QDBusMessage& message);

    Q_SCRIPTABLE void PreparePrint(const QDBusObjectPath& handle,
                                  const QString& app_id,
                                  const QString& parent_window,
                                  const QString& title,
                                  const QVariantMap& settings,
                                  const QVariantMap& page_setup,
                                  const QVariantMap& options,
                                  const QDBusMessage& message);
};

} // namespace wormhole::portal
