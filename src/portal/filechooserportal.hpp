#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "request.hpp"

namespace wormhole::portal {

class FileChooserPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.FileChooser")

public:
    explicit FileChooserPortal(QObject* parent = nullptr);
    ~FileChooserPortal() override = default;

public slots:
    Q_SCRIPTABLE void OpenFile(const QDBusObjectPath& handle,
                               const QString& app_id,
                               const QString& parent_window,
                               const QString& title,
                               const QVariantMap& options,
                               const QDBusMessage& message);

    Q_SCRIPTABLE void SaveFile(const QDBusObjectPath& handle,
                               const QString& app_id,
                               const QString& parent_window,
                               const QString& title,
                               const QVariantMap& options,
                               const QDBusMessage& message);

    Q_SCRIPTABLE void SaveFiles(const QDBusObjectPath& handle,
                                const QString& app_id,
                                const QString& parent_window,
                                const QString& title,
                                const QVariantMap& options,
                                const QDBusMessage& message);

    void finishRequest(const QString& handlePath, quint32 response, const QVariantMap& results);

signals:
    void openFileRequested(const QString& handlePath,
                           const QString& title,
                           const QStringList& filters,
                           const QString& filterLabel,
                           bool directoryOnly,
                           bool multiple,
                           const QString& initialDirectory);

    void saveFileRequested(const QString& handlePath,
                           const QString& title,
                           const QStringList& filters,
                           const QString& filterLabel,
                           const QString& suggestedName,
                           const QString& initialDirectory);

    void saveFilesRequested(const QString& handlePath,
                            const QString& title,
                            const QStringList& fileList,
                            const QString& initialDirectory);

private:
    void sendResponse(const QString& handlePath, quint32 response, const QVariantMap& results);
    void cleanupRequest(const QString& handlePath);

    static QString parseInitialDirectory(const QVariantMap& options);
    static void parseFilters(const QVariantMap& options, QStringList& outFilters, QString& outLabel);

    struct PendingRequest {
        QDBusMessage message;
        PortalRequest* requestObject = nullptr;
    };

    QMap<QString, PendingRequest> m_requests;
};

} // namespace wormhole::portal