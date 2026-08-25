#include "printportal.hpp"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QUuid>
#include <unistd.h>

namespace wormhole::portal {

PrintPortal::PrintPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void PrintPortal::Print(const QDBusObjectPath& /*handle*/,
                        const QString& /*app_id*/,
                        const QString& /*parent_window*/,
                        const QString& /*title*/,
                        const QDBusUnixFileDescriptor& fd,
                        const QVariantMap& /*options*/,
                        const QDBusMessage& message) {
    if (!fd.isValid()) {
        QDBusMessage reply = message.createReply();
        reply << static_cast<uint>(1) << QVariantMap();
        QDBusConnection::sessionBus().send(reply);
        return;
    }

    int rawFd = fd.fileDescriptor();
    int dupFd = dup(rawFd);
    if (dupFd < 0) {
        QDBusMessage reply = message.createReply();
        reply << static_cast<uint>(1) << QVariantMap();
        QDBusConnection::sessionBus().send(reply);
        return;
    }

    QString spoolDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/wormhole/print");
    QDir().mkpath(spoolDir);
    QString spoolFile = spoolDir + QStringLiteral("/print_%1.pdf").arg(QDateTime::currentMSecsSinceEpoch());

    QFile src;
    if (src.open(dupFd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
        QFile dst(spoolFile);
        if (dst.open(QIODevice::WriteOnly)) {
            dst.write(src.readAll());
            dst.close();
            // Dispatch to lp or gtklp
            QProcess::startDetached(QStringLiteral("lp"), { spoolFile });
        }
    } else {
        close(dupFd);
    }

    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

void PrintPortal::PreparePrint(const QDBusObjectPath& /*handle*/,
                              const QString& /*app_id*/,
                              const QString& /*parent_window*/,
                              const QString& /*title*/,
                              const QVariantMap& settings,
                              const QVariantMap& page_setup,
                              const QVariantMap& /*options*/,
                              const QDBusMessage& message) {
    QVariantMap results;
    results.insert(QStringLiteral("token"), static_cast<uint>(QUuid::createUuid().data1));
    results.insert(QStringLiteral("settings"), settings);
    results.insert(QStringLiteral("page-setup"), page_setup);

    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(0) << results;
    QDBusConnection::sessionBus().send(reply);
}

} // namespace wormhole::portal
