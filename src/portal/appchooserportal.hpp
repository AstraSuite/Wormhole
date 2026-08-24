#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "request.hpp"

namespace wormhole::portal {

class AppChooserPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.AppChooser")

public:
    explicit AppChooserPortal(QObject* parent = nullptr);
    ~AppChooserPortal() override = default;

public slots:
    Q_SCRIPTABLE void ChooseApplication(const QDBusObjectPath& handle,
                                        const QString& app_id,
                                        const QString& parent_window,
                                        const QStringList& choices,
                                        const QVariantMap& options,
                                        const QDBusMessage& message);

    Q_SCRIPTABLE void UpdateChoices(const QDBusObjectPath& handle,
                                    const QStringList& choices);

private:
    struct PendingRequest {
        QDBusMessage message;
        QProcess* process = nullptr;
        PortalRequest* requestObject = nullptr;
    };

    QMap<QString, PendingRequest> m_requests;
};

} // namespace wormhole::portal
