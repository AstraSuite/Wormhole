#include "secretportal.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <unistd.h>

namespace wormhole::portal {

SecretPortal::SecretPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

QByteArray SecretPortal::getOrCreateMasterKey() {
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/wormhole");
    QDir().mkpath(dirPath);
    QString keyPath = dirPath + QStringLiteral("/secret-master.key");

    QFile file(keyPath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray key = file.readAll();
        if (key.size() >= 32) {
            return key;
        }
        file.close();
    }

    QByteArray newKey(64, '\0');
    QRandomGenerator::securelySeeded().fillRange(reinterpret_cast<quint32*>(newKey.data()), newKey.size() / sizeof(quint32));

    if (file.open(QIODevice::WriteOnly)) {
        file.write(newKey);
        file.close();
        QFile::setPermissions(keyPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }
    return newKey;
}

void SecretPortal::RetrieveSecret(const QDBusObjectPath& /*handle*/,
                                 const QDBusUnixFileDescriptor& fd,
                                 const QVariantMap& options,
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

    QString token = options.value(QStringLiteral("token")).toString();
    QByteArray masterKey = getOrCreateMasterKey();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(masterKey);
    hash.addData(token.toUtf8());
    QByteArray derivedSecret = hash.result();

    QFile outFile;
    if (outFile.open(dupFd, QIODevice::WriteOnly, QFileDevice::AutoCloseHandle)) {
        outFile.write(derivedSecret);
        outFile.flush();
        outFile.close();

        QDBusMessage reply = message.createReply();
        reply << static_cast<uint>(0) << QVariantMap();
        QDBusConnection::sessionBus().send(reply);
    } else {
        close(dupFd);
        QDBusMessage reply = message.createReply();
        reply << static_cast<uint>(1) << QVariantMap();
        QDBusConnection::sessionBus().send(reply);
    }
}

} // namespace wormhole::portal
