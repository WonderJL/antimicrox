#include "macvirtualcontrollermanager.h"

#ifdef Q_OS_MAC

#include "macvirtualpad.h"

#include <QDebug>

namespace
{
constexpr qint16 kAxisMax = 32767;
constexpr qint16 kAxisMin = -32767;
constexpr qint16 kTriggerMax = 1023;
}

MacVirtualControllerManager::MacVirtualControllerManager(QObject *parent)
    : QObject(parent)
    , m_pad(new MacVirtualPad())
    , m_eventTap(nullptr)
    , m_runLoopSource(nullptr)
    , m_active(false)
    , m_leftTrigger(0)
    , m_rightTrigger(0)
    , m_hatUp(false)
    , m_hatDown(false)
    , m_hatLeft(false)
    , m_hatRight(false)
{
    // ABXY
    m_buttonMap.insert(38, 0); // J -> A
    m_buttonMap.insert(40, 1); // K -> B
    m_buttonMap.insert(32, 2); // U -> X
    m_buttonMap.insert(34, 3); // I -> Y

    // Shoulder buttons
    m_buttonMap.insert(12, 4); // Q -> LB
    m_buttonMap.insert(14, 5); // E -> RB

    // Start / Select
    m_buttonMap.insert(36, 6); // Return -> Start
    m_buttonMap.insert(49, 7); // Space -> Select

    // Stick click
    m_buttonMap.insert(45, 8); // N -> L3
    m_buttonMap.insert(46, 9); // M -> R3

    // Extra buttons (Map to back/menu)
    m_buttonMap.insert(35, 10); // P -> Button 11
    m_buttonMap.insert(37, 11); // L -> Button 12

    // Triggers (digital -> analog)
    m_triggerMap.insert(6, 0);  // Z -> LT
    m_triggerMap.insert(7, 1);  // X -> RT
}

MacVirtualControllerManager::~MacVirtualControllerManager()
{
    stop();
    delete m_pad;
    m_pad = nullptr;
}

bool MacVirtualControllerManager::start()
{
    if (m_active)
        return true;

    if (!m_pad->open())
        return false;

    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp);
    m_eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly, mask,
                                  &MacVirtualControllerManager::eventCallback, this);

    if (!m_eventTap)
    {
        qWarning() << "Failed to create keyboard event tap for virtual controller.";
        return false;
    }

    m_runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), m_runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(m_eventTap, true);

    m_active = true;
    m_pad->sync();
    return true;
}

void MacVirtualControllerManager::stop()
{
    if (m_eventTap)
    {
        CGEventTapEnable(m_eventTap, false);
        if (m_runLoopSource)
        {
            CFRunLoopRemoveSource(CFRunLoopGetCurrent(), m_runLoopSource, kCFRunLoopCommonModes);
            CFRelease(m_runLoopSource);
            m_runLoopSource = nullptr;
        }
        CFRelease(m_eventTap);
        m_eventTap = nullptr;
    }

    if (m_pad)
        m_pad->close();

    m_active = false;
}

bool MacVirtualControllerManager::isActive() const
{
    return m_active;
}

CGEventRef MacVirtualControllerManager::eventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event,
                                                      void *refcon)
{
    auto *manager = static_cast<MacVirtualControllerManager *>(refcon);
    if (!manager)
        return event;

    return manager->handleEvent(type, event);
}

CGEventRef MacVirtualControllerManager::handleEvent(CGEventType type, CGEventRef event)
{
    if (type == kCGEventKeyDown || type == kCGEventKeyUp)
    {
        bool pressed = (type == kCGEventKeyDown);
        CGKeyCode code = static_cast<CGKeyCode>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
        handleKey(code, pressed);
    }

    return event;
}

void MacVirtualControllerManager::handleKey(CGKeyCode code, bool pressed)
{
    bool changed = false;

    if (m_buttonMap.contains(code))
    {
        int button = m_buttonMap.value(code);
        m_pad->setButton(button, pressed);
        changed = true;
    }
    else if (m_triggerMap.contains(code))
    {
        int triggerIndex = m_triggerMap.value(code);
        qint16 value = pressed ? kTriggerMax : 0;
        if (triggerIndex == 0)
        {
            if (m_leftTrigger != value)
            {
                m_leftTrigger = value;
                m_pad->setTrigger(0, value);
                changed = true;
            }
        }
        else
        {
            if (m_rightTrigger != value)
            {
                m_rightTrigger = value;
                m_pad->setTrigger(1, value);
                changed = true;
            }
        }
    }
    else
    {
        switch (code)
        {
        case 0: // A
            m_leftX.negative = pressed;
            updateLeftAxis();
            changed = true;
            break;
        case 2: // D
            m_leftX.positive = pressed;
            updateLeftAxis();
            changed = true;
            break;
        case 13: // W
            m_leftY.negative = pressed;
            updateLeftAxis();
            changed = true;
            break;
        case 1: // S
            m_leftY.positive = pressed;
            updateLeftAxis();
            changed = true;
            break;
        case 126: // Up arrow
            m_hatUp = pressed;
            updateHat();
            changed = true;
            break;
        case 125: // Down arrow
            m_hatDown = pressed;
            updateHat();
            changed = true;
            break;
        case 123: // Left arrow
            m_hatLeft = pressed;
            updateHat();
            changed = true;
            break;
        case 124: // Right arrow
            m_hatRight = pressed;
            updateHat();
            changed = true;
            break;
        default:
            break;
        }
    }

    if (changed)
        syncPad();
}

void MacVirtualControllerManager::updateLeftAxis()
{
    qint16 x = 0;
    if (m_leftX.negative && !m_leftX.positive)
        x = kAxisMin;
    else if (m_leftX.positive && !m_leftX.negative)
        x = kAxisMax;

    qint16 y = 0;
    if (m_leftY.negative && !m_leftY.positive)
        y = kAxisMin;
    else if (m_leftY.positive && !m_leftY.negative)
        y = kAxisMax;

    m_pad->setAxis(0, x);
    m_pad->setAxis(1, y);
}

void MacVirtualControllerManager::updateHat()
{
    int value = -1;
    if (m_hatUp && m_hatRight)
        value = 1;
    else if (m_hatRight && m_hatDown)
        value = 3;
    else if (m_hatDown && m_hatLeft)
        value = 5;
    else if (m_hatLeft && m_hatUp)
        value = 7;
    else if (m_hatUp)
        value = 0;
    else if (m_hatRight)
        value = 2;
    else if (m_hatDown)
        value = 4;
    else if (m_hatLeft)
        value = 6;

    m_pad->setHat(value);
}

void MacVirtualControllerManager::syncPad()
{
    m_pad->sync();
}

#endif // Q_OS_MAC
