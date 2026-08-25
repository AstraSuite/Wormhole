#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QObject>

namespace wormhole::portal {

class RealtimePortal : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Realtime")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit RealtimePortal(QObject* parent = nullptr);
    ~RealtimePortal() override = default;

    uint version() const { return 1; }

public slots:
    Q_SCRIPTABLE uint MakeThreadRealtimeWithPID(qulonglong process, qulonglong thread, uint priority);
    Q_SCRIPTABLE uint MakeThreadHighPriorityWithPID(qulonglong process, qulonglong thread, int priority);
};

} // namespace wormhole::portal
