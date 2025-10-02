#ifndef MACEVENTHANDLER_H
#define MACEVENTHANDLER_H

#include "eventhandlers/baseeventhandler.h"

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

class MacEventHandler : public BaseEventHandler
{
    Q_OBJECT

  public:
    explicit MacEventHandler(QObject *parent = nullptr);
    ~MacEventHandler() override;

    bool init() override;
    bool cleanup() override;

    void sendKeyboardEvent(JoyButtonSlot *slot, bool pressed) override;
    void sendMouseButtonEvent(JoyButtonSlot *slot, bool pressed) override;
    void sendMouseEvent(int xDis, int yDis) override;
    void sendMouseAbsEvent(int xDis, int yDis, int screen) override;
    void sendMouseSpringEvent(int xDis, int yDis, int width, int height) override;
    void sendTextEntryEvent(QString maintext) override;

    QString getName() override;
    QString getIdentifier() override;
    void printPostMessages() override;

  private:
    bool ensureEventSource();
    void promptForAccessibility();
    CGPoint currentCursorPosition() const;
    void postMouseEvent(CGEventType type, CGPoint location, CGMouseButton button);

    CGEventSourceRef eventSource;
    bool accessibilityTrusted;
};

#endif // MACEVENTHANDLER_H
