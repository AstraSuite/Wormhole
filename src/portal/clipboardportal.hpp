#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "session.hpp"

namespace wormhole::portal {

class ClipboardPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Clipboard")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit ClipboardPortal(QObject* parent = nullptr);
    ~ClipboardPortal() override = default;

    uint version() const { return 1; }

public slots:
    Q_SCRIPTABLE uint RequestClipboard(const QDBusObjectPath& handle,
                                       const QDBusObjectPath& session_handle,
                                       const QString& app_id,
                                       const QVariantMap& options,
                                       QVariantMap& results);

    Q_SCRIPTABLE uint SetSelection(const QDBusObjectPath& session_handle,
                                   const QVariantMap& options,
                                   const QVariantMap& mime_types,
                                   QVariantMap& results);

signals:
    Q_SCRIPTABLE void SelectionOwnerChanged(const QDBusObjectPath& session_handle,
                                           const QVariantMap& options,
                                           const QStringList& mime_types);

    Q_SCRIPTABLE void SelectionTransfer(const QDBusObjectPath& session_handle,
                                        const QString& mime_type,
                                        const QDBusUnixFileDescriptor& fd);
};

} // namespace wormhole::portal
