#include "eventhandlers/maceventhandler.h"

#ifdef Q_OS_MAC

#include "antkeymapper.h"
#include "joybuttonslot.h"

#include <QDebug>
#include <QGuiApplication>
#include <QScreen>

#include <CoreFoundation/CoreFoundation.h>

MacEventHandler::MacEventHandler(QObject *parent)
    : BaseEventHandler(parent)
    , eventSource(nullptr)
    , accessibilityTrusted(false)
{
}

MacEventHandler::~MacEventHandler() { cleanup(); }

bool MacEventHandler::ensureEventSource()
{
    if (eventSource)
        return true;

    eventSource = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!eventSource)
    {
        lastErrorString = QStringLiteral("Unable to create Quartz event source.");
        return false;
    }

    CGEventSourceSetLocalEventsSuppressionInterval(eventSource, 0.0);
    return true;
}

void MacEventHandler::promptForAccessibility()
{
    if (accessibilityTrusted)
        return;

    const void *keys[] = {kAXTrustedCheckOptionPrompt};
    const void *values[] = {kCFBooleanTrue};
    CFDictionaryRef options =
        CFDictionaryCreate(nullptr, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

#if defined(MAC_OS_X_VERSION_10_9) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_9
    if (options)
    {
        accessibilityTrusted = AXIsProcessTrustedWithOptions(options);
        CFRelease(options);
    }
    else
    {
        accessibilityTrusted = AXIsProcessTrusted();
    }
#else
    Q_UNUSED(options);
    accessibilityTrusted = AXIsProcessTrusted();
#endif

    if (!accessibilityTrusted)
    {
        qWarning() << "Accessibility permission has not been granted. Keyboard and mouse synthesis may not work.";
    }
}

bool MacEventHandler::init()
{
    accessibilityTrusted = AXIsProcessTrusted();
    if (!accessibilityTrusted)
        promptForAccessibility();

    return ensureEventSource();
}

bool MacEventHandler::cleanup()
{
    if (eventSource)
    {
        CFRelease(eventSource);
        eventSource = nullptr;
    }

    return true;
}

CGPoint MacEventHandler::currentCursorPosition() const
{
    CGEventRef event = CGEventCreate(nullptr);
    CGPoint point = CGEventGetLocation(event);
    if (event)
        CFRelease(event);
    return point;
}

void MacEventHandler::postMouseEvent(CGEventType type, CGPoint location, CGMouseButton button)
{
    if (!ensureEventSource())
        return;

    CGEventRef mouseEvent = CGEventCreateMouseEvent(eventSource, type, location, button);
    if (!mouseEvent)
        return;

    CGEventPost(kCGHIDEventTap, mouseEvent);
    CFRelease(mouseEvent);
}

void MacEventHandler::sendKeyboardEvent(JoyButtonSlot *slot, bool pressed)
{
    if (!ensureEventSource())
        return;

    CGKeyCode keyCode = static_cast<CGKeyCode>(slot->getSlotCode());
    CGEventRef keyEvent = CGEventCreateKeyboardEvent(eventSource, keyCode, pressed);
    if (!keyEvent)
        return;

    CGEventPost(kCGHIDEventTap, keyEvent);
    CFRelease(keyEvent);
}

void MacEventHandler::sendMouseButtonEvent(JoyButtonSlot *slot, bool pressed)
{
    if (!ensureEventSource())
        return;

    const int code = slot->getSlotCode();

    if (code >= 4 && code <= 7)
    {
        if (!pressed)
            return;

        int64_t vertical = 0;
        int64_t horizontal = 0;
        switch (code)
        {
        case 4:
            vertical = 1;
            break;
        case 5:
            vertical = -1;
            break;
        case 6:
            horizontal = -1;
            break;
        case 7:
            horizontal = 1;
            break;
        default:
            break;
        }

        CGEventRef scroll = nullptr;
        if (horizontal != 0 && vertical != 0)
            scroll = CGEventCreateScrollWheelEvent(eventSource, kCGScrollEventUnitLine, 2, vertical, horizontal);
        else if (vertical != 0)
            scroll = CGEventCreateScrollWheelEvent(eventSource, kCGScrollEventUnitLine, 1, vertical);
        else if (horizontal != 0)
            scroll = CGEventCreateScrollWheelEvent(eventSource, kCGScrollEventUnitLine, 2, 0, horizontal);

        if (scroll)
        {
            CGEventPost(kCGHIDEventTap, scroll);
            CFRelease(scroll);
        }
        return;
    }

    CGPoint cursor = currentCursorPosition();
    CGMouseButton button = kCGMouseButtonLeft;
    CGEventType type = pressed ? kCGEventLeftMouseDown : kCGEventLeftMouseUp;

    switch (code)
    {
    case 1:
        button = kCGMouseButtonLeft;
        type = pressed ? kCGEventLeftMouseDown : kCGEventLeftMouseUp;
        break;
    case 2:
        button = kCGMouseButtonCenter;
        type = pressed ? kCGEventOtherMouseDown : kCGEventOtherMouseUp;
        break;
    case 3:
        button = kCGMouseButtonRight;
        type = pressed ? kCGEventRightMouseDown : kCGEventRightMouseUp;
        break;
    default:
        button = kCGMouseButtonCenter;
        type = pressed ? kCGEventOtherMouseDown : kCGEventOtherMouseUp;
        break;
    }

    postMouseEvent(type, cursor, button);
}

void MacEventHandler::sendMouseEvent(int xDis, int yDis)
{
    if (!ensureEventSource())
        return;

    CGPoint cursor = currentCursorPosition();
    cursor.x += xDis;
    cursor.y += yDis;

    postMouseEvent(kCGEventMouseMoved, cursor, kCGMouseButtonLeft);
}

void MacEventHandler::sendMouseAbsEvent(int xDis, int yDis, int screen)
{
    Q_UNUSED(screen);
    if (!ensureEventSource())
        return;

    CGPoint point = CGPointMake(static_cast<CGFloat>(xDis), static_cast<CGFloat>(yDis));
    postMouseEvent(kCGEventMouseMoved, point, kCGMouseButtonLeft);
}

void MacEventHandler::sendMouseSpringEvent(int xDis, int yDis, int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
    sendMouseAbsEvent(xDis, yDis, 0);
}

void MacEventHandler::sendTextEntryEvent(QString maintext)
{
    if (!ensureEventSource())
        return;

    for (const QChar &character : maintext)
    {
        UniChar ch = character.unicode();
        CGEventRef keyDown = CGEventCreateKeyboardEvent(eventSource, 0, true);
        if (keyDown)
        {
            CGEventKeyboardSetUnicodeString(keyDown, 1, &ch);
            CGEventPost(kCGHIDEventTap, keyDown);
            CFRelease(keyDown);
        }

        CGEventRef keyUp = CGEventCreateKeyboardEvent(eventSource, 0, false);
        if (keyUp)
        {
            CGEventKeyboardSetUnicodeString(keyUp, 1, &ch);
            CGEventPost(kCGHIDEventTap, keyUp);
            CFRelease(keyUp);
        }
    }
}

QString MacEventHandler::getName() { return QStringLiteral("Quartz Event Services"); }

QString MacEventHandler::getIdentifier() { return QStringLiteral("macos"); }

void MacEventHandler::printPostMessages()
{
    if (!accessibilityTrusted)
    {
        qInfo() << "AntiMicroX requires Accessibility access to send keyboard and mouse events.";
        qInfo() << "Enable it under System Settings > Privacy & Security > Accessibility.";
    }
}

#endif // Q_OS_MAC
