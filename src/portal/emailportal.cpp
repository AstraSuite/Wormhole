#include "emailportal.hpp"
#include <QDesktopServices>
#include <QProcess>
#include <QUrl>
#include <QUrlQuery>

namespace wormhole::portal {

EmailPortal::EmailPortal(QObject* parent)
    : QDBusAbstractAdaptor(parent) {
}

void EmailPortal::ComposeEmail(const QDBusObjectPath& /*handle*/,
                               const QString& /*app_id*/,
                               const QString& /*parent_window*/,
                               const QVariantMap& options,
                               const QDBusMessage& message) {
    QString address = options.value(QStringLiteral("address")).toString();
    QStringList addresses = options.value(QStringLiteral("addresses")).toStringList();
    QStringList cc = options.value(QStringLiteral("cc")).toStringList();
    QStringList bcc = options.value(QStringLiteral("bcc")).toStringList();
    QString subject = options.value(QStringLiteral("subject")).toString();
    QString body = options.value(QStringLiteral("body")).toString();

    if (address.isEmpty() && !addresses.isEmpty()) {
        address = addresses.join(QLatin1Char(','));
    }

    QUrl mailUrl(QStringLiteral("mailto:") + address);
    QUrlQuery query;
    if (!cc.isEmpty()) query.addQueryItem(QStringLiteral("cc"), cc.join(QLatin1Char(',')));
    if (!bcc.isEmpty()) query.addQueryItem(QStringLiteral("bcc"), bcc.join(QLatin1Char(',')));
    if (!subject.isEmpty()) query.addQueryItem(QStringLiteral("subject"), subject);
    if (!body.isEmpty()) query.addQueryItem(QStringLiteral("body"), body);

    if (!query.isEmpty()) {
        mailUrl.setQuery(query);
    }

    bool ok = QDesktopServices::openUrl(mailUrl);
    if (!ok) {
        ok = QProcess::startDetached(QStringLiteral("xdg-email"), { mailUrl.toString() });
    }

    QDBusMessage reply = message.createReply();
    reply << static_cast<uint>(ok ? 0 : 1) << QVariantMap();
    QDBusConnection::sessionBus().send(reply);
}

} // namespace wormhole::portal
