#ifndef MACVIRTUALCONTROLLERMANAGER_H
#define MACVIRTUALCONTROLLERMANAGER_H

#include <QtGlobal>

#ifdef Q_OS_MAC

#include <QObject>
#include <QHash>
#include <QSet>

#include <CoreGraphics/CoreGraphics.h>

class MacVirtualPad;

class Q_DECL_EXPORT MacVirtualControllerManager : public QObject
{
    Q_OBJECT

  public:
    explicit MacVirtualControllerManager(QObject *parent = nullptr);
    ~MacVirtualControllerManager() override;

    bool start();
    void stop();
    bool isActive() const;

  private:
    static CGEventRef eventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
    CGEventRef handleEvent(CGEventType type, CGEventRef event);
    void handleKey(CGKeyCode code, bool pressed);

    void updateLeftAxis();
    void updateHat();
    void syncPad();

    struct AxisState
    {
        bool negative = false;
        bool positive = false;
        qint16 value = 0;
    };

    MacVirtualPad *m_pad;
    CFMachPortRef m_eventTap;
    CFRunLoopSourceRef m_runLoopSource;
    bool m_active;

    AxisState m_leftX;
    AxisState m_leftY;
    qint16 m_leftTrigger;
    qint16 m_rightTrigger;

    bool m_hatUp;
    bool m_hatDown;
    bool m_hatLeft;
    bool m_hatRight;

    QHash<CGKeyCode, int> m_buttonMap;
    QHash<CGKeyCode, int> m_triggerMap;
};

#endif // Q_OS_MAC

#endif // MACVIRTUALCONTROLLERMANAGER_H
