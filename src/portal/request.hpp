#pragma once

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>

namespace wormhole::portal {

class PortalRequest : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Request")

public:
    explicit PortalRequest(const QString& path, QObject* parent = nullptr);
    ~PortalRequest() override;

public slots:
    Q_SCRIPTABLE void Close();

signals:
    void closeRequested();

private:
    QString m_path;
};

} // namespace wormhole::portal
