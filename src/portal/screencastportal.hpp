#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QList>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QVariantMap>
#include "request.hpp"
#include "restorestore.hpp"
#include "session.hpp"
#include "../screencast/pipewirestream.hpp"
#include "../screencast/waylandcapture.hpp"

namespace wormhole::portal {

struct ScreenCastPosition {
    int32_t x = 0;
    int32_t y = 0;
};

inline QDBusArgument& operator<<(QDBusArgument& argument, const ScreenCastPosition& pos) {
    argument.beginStructure();
    argument << pos.x << pos.y;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument& operator>>(const QDBusArgument& argument, ScreenCastPosition& pos) {
    argument.beginStructure();
    argument >> pos.x >> pos.y;
    argument.endStructure();
    return argument;
}

struct ScreenCastSize {
    int32_t width = 0;
    int32_t height = 0;
};

inline QDBusArgument& operator<<(QDBusArgument& argument, const ScreenCastSize& size) {
    argument.beginStructure();
    argument << size.width << size.height;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument& operator>>(const QDBusArgument& argument, ScreenCastSize& size) {
    argument.beginStructure();
    argument >> size.width >> size.height;
    argument.endStructure();
    return argument;
}

struct ScreenCastStream {
    uint32_t nodeId = 0;
    QVariantMap properties;
};

inline QDBusArgument& operator<<(QDBusArgument& argument, const ScreenCastStream& stream) {
    argument.beginStructure();
    argument << stream.nodeId << stream.properties;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument& operator>>(const QDBusArgument& argument, ScreenCastStream& stream) {
    argument.beginStructure();
    argument >> stream.nodeId >> stream.properties;
    argument.endStructure();
    return argument;
}

using ScreenCastStreamList = QList<ScreenCastStream>;

class ScreenCastPortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.ScreenCast")
    Q_PROPERTY(uint version READ version CONSTANT)
    Q_PROPERTY(uint AvailableSourceTypes READ AvailableSourceTypes CONSTANT)
    Q_PROPERTY(uint AvailableCursorModes READ AvailableCursorModes CONSTANT)

public:
    enum SourceType {
        Monitor = 1,
        Window = 2,
        Virtual = 4
    };
    Q_ENUM(SourceType)

    enum CursorMode {
        Hidden = 1,
        Embedded = 2,
        Metadata = 4
    };
    Q_ENUM(CursorMode)

    enum PersistMode {
        NoPersist = 0,
        PersistWhileRunning = 1,
        PersistUntilRevoked = 2
    };
    Q_ENUM(PersistMode)

    explicit ScreenCastPortal(QObject* parent = nullptr);
    ~ScreenCastPortal() override = default;

    uint version() const { return 5; }
    uint AvailableSourceTypes() const { return Monitor | Window | Virtual; }
    uint AvailableCursorModes() const { return Hidden | Embedded; }

public slots:
    Q_SCRIPTABLE uint CreateSession(const QDBusObjectPath& handle,
                                    const QDBusObjectPath& session_handle,
                                    const QString& app_id,
                                    const QVariantMap& options,
                                    QVariantMap& results);

    Q_SCRIPTABLE uint SelectSources(const QDBusObjectPath& handle,
                                    const QDBusObjectPath& session_handle,
                                    const QString& app_id,
                                    const QVariantMap& options,
                                    QVariantMap& results);

    Q_SCRIPTABLE void Start(const QDBusObjectPath& handle,
                            const QDBusObjectPath& session_handle,
                            const QString& app_id,
                            const QString& parent_window,
                            const QVariantMap& options,
                            const QDBusMessage& message);

private:
    struct SelectedSource {
        bool isWindow = false;
        QString outputName;
        QString windowAppId;
        QString windowTitle;
        QString windowAddress;
        int x = 0;
        int y = 0;
        int fps = 60;
        bool paintCursor = false;
    };

    struct SessionData {
        QString appId;
        uint types = Monitor | Window;
        uint cursorMode = Hidden;
        uint persistMode = NoPersist;
        QString restoreToken;
        uint32_t pipewireNodeId = 0;
        screencast::WaylandCapture* capture = nullptr;
    };

    struct PendingStartRequest {
        QDBusMessage message;
        QDBusObjectPath sessionHandle;
        QString appId;
        QProcess* process = nullptr;
        PortalRequest* requestObject = nullptr;
    };

    void closeSession(const QString& sessionPath);

    bool startStream(const QString& sessionPath, const SelectedSource& source, QVariantMap& results);
    static void sendReply(const QDBusMessage& message, uint response, const QVariantMap& results);

    void launchScreenChooser(const QDBusObjectPath& handle,
                             const QDBusObjectPath& sessionHandle,
                             const QString& appId,
                             const QString& parentWindow,
                             const QDBusMessage& message);

    QMap<QString, SessionData> m_sessions;
    QMap<QString, PendingStartRequest> m_startRequests;
};

} // namespace wormhole::portal

Q_DECLARE_METATYPE(wormhole::portal::ScreenCastPosition)
Q_DECLARE_METATYPE(wormhole::portal::ScreenCastSize)
Q_DECLARE_METATYPE(wormhole::portal::ScreenCastStream)
Q_DECLARE_METATYPE(wormhole::portal::ScreenCastStreamList)
