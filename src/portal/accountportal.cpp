#include "accountportal.hpp"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>

namespace wormhole::portal {

AccountPortal::AccountPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void AccountPortal::GetUserInformation(const QDBusObjectPath& /*handle*/,
                                      const QString& /*app_id*/,
                                      const QString& /*parent_window*/,
                                      const QVariantMap& /*options*/,
                                      const QDBusMessage& message) {
    QString username = qEnvironmentVariable("USER", QStringLiteral("user"));
    QString realName = username;

    // Check /etc/passwd or getenv for name
    QString facePath = QDir::homePath() + QStringLiteral("/.face");
    if (!QFile::exists(facePath)) {
        facePath = QDir::homePath() + QStringLiteral("/.face.icon");
    }

    QVariantMap results;
    results.insert(QStringLiteral("id"), username);
    results.insert(QStringLiteral("name"), realName);
    if (QFile::exists(facePath)) {
        results.insert(QStringLiteral("image"), QStringLiteral("file://") + facePath);
    }

    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << results;
    QDBusConnection::sessionBus().send(reply);
}

} // namespace wormhole::portal
