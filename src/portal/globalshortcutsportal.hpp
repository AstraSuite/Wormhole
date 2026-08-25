#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include "session.hpp"

namespace wormhole::portal {

struct GlobalShortcutItem {
    QString id;
    QVariantMap options;
};

inline QDBusArgument& operator<<(QDBusArgument& argument, const GlobalShortcutItem& item) {
    argument.beginStructure();
    argument << item.id << item.options;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument& operator>>(const QDBusArgument& argument, GlobalShortcutItem& item) {
    argument.beginStructure();
    argument >> item.id >> item.options;
    argument.endStructure();
    return argument;
}

using GlobalShortcutList = QList<GlobalShortcutItem>;

class GlobalShortcutsPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.GlobalShortcuts")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit GlobalShortcutsPortal(QObject* parent = nullptr);
    ~GlobalShortcutsPortal() override = default;

    uint version() const { return 1; }

public slots:
    Q_SCRIPTABLE uint CreateSession(const QDBusObjectPath& handle,
                                    const QDBusObjectPath& session_handle,
                                    const QString& app_id,
                                    const QVariantMap& options,
                                    QVariantMap& results);

    Q_SCRIPTABLE void BindShortcuts(const QDBusObjectPath& handle,
                                    const QDBusObjectPath& session_handle,
                                    const wormhole::portal::GlobalShortcutList& shortcuts,
                                    const QString& parent_window,
                                    const QVariantMap& options,
                                    const QDBusMessage& message);

    Q_SCRIPTABLE uint ListShortcuts(const QDBusObjectPath& handle,
                                   const QDBusObjectPath& session_handle,
                                   QVariantMap& results);

signals:
    Q_SCRIPTABLE void Activated(const QDBusObjectPath& session_handle,
                                const QString& shortcut_id,
                                qulonglong timestamp,
                                const QVariantMap& options);

    Q_SCRIPTABLE void Deactivated(const QDBusObjectPath& session_handle,
                                  const QString& shortcut_id,
                                  qulonglong timestamp,
                                  const QVariantMap& options);

    Q_SCRIPTABLE void ShortcutsChanged(const QDBusObjectPath& session_handle,
                                       const wormhole::portal::GlobalShortcutList& shortcuts);

private:
    struct SessionData {
        QString appId;
        QList<GlobalShortcutItem> shortcuts;
    };

    QMap<QString, SessionData> m_sessions;
};

} // namespace wormhole::portal

Q_DECLARE_METATYPE(wormhole::portal::GlobalShortcutItem)
Q_DECLARE_METATYPE(wormhole::portal::GlobalShortcutList)
