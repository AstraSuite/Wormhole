#pragma once

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace wormhole::portal {

class PortalSession : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Session")

public:
    explicit PortalSession(const QString& path, const QString& appId, QObject* parent = nullptr);
    ~PortalSession() override;

    QString path() const { return m_path; }
    QString appId() const { return m_appId; }

public slots:
    Q_SCRIPTABLE void Close();

signals:
    void closed();

private:
    QString m_path;
    QString m_appId;
};

} // namespace wormhole::portal
