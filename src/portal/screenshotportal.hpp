#pragma once

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QVariantMap>
#include "request.hpp"

namespace wormhole::portal {

class ScreenshotPortal : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Screenshot")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit ScreenshotPortal(QObject* parent = nullptr);
    ~ScreenshotPortal() override = default;

    uint version() const { return 2; }

public slots:
    Q_SCRIPTABLE void Screenshot(const QDBusObjectPath& handle,
                                const QString& app_id,
                                const QString& parent_window,
                                const QVariantMap& options,
                                const QDBusMessage& message);

    Q_SCRIPTABLE void PickColor(const QDBusObjectPath& handle,
                               const QString& app_id,
                               const QString& parent_window,
                               const QVariantMap& options,
                               const QDBusMessage& message);

private:
    struct PendingRequest {
        QDBusMessage message;
        QProcess* process = nullptr;
        PortalRequest* requestObject = nullptr;
        bool isPickColor = false;
    };

    QMap<QString, PendingRequest> m_requests;
};

} // namespace wormhole::portal
