#ifndef MACVIRTUALPAD_H
#define MACVIRTUALPAD_H

#include <QtGlobal>

#ifdef Q_OS_MAC

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hidsystem/IOHIDUserDevice.h>
#include <IOKit/hid/IOHIDLib.h>
#include <mach/mach_time.h>

#include <QDebug>
#include <cstring>

namespace MacVirtualPadPrivate
{
constexpr quint16 kVendorId = 0x3438;
constexpr quint16 kProductId = 0x00A0;
constexpr quint16 kVersion = 0x0001;

const uint8_t kGamepadReportDescriptor[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x05,       // Usage (Game Pad)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID 1
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x35, 0x00,       //   Physical Minimum (0)
    0x45, 0x01,       //   Physical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x10,       //   Report Count (16)
    0x05, 0x09,       //   Usage Page (Button)
    0x19, 0x01,       //   Usage Minimum (Button 1)
    0x29, 0x10,       //   Usage Maximum (Button 16)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x05, 0x01,       //   Usage Page (Generic Desktop)
    0x25, 0x07,       //   Logical Maximum (7)
    0x46, 0x3B, 0x01, //   Physical Maximum (315°)
    0x75, 0x04,       //   Report Size (4)
    0x95, 0x01,       //   Report Count (1)
    0x65, 0x14,       //   Unit (Degrees)
    0x09, 0x39,       //   Usage (Hat switch)
    0x81, 0x42,       //   Input (Data,Var,Abs,Null)
    0x65, 0x00,       //   Unit (None)
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x04,       //   Report Size (4)
    0x81, 0x03,       //   Input (Const,Var,Abs)
    0x16, 0x00, 0x80, //   Logical Minimum (-32768)
    0x26, 0xFF, 0x7F, //   Logical Maximum (32767)
    0x36, 0x00, 0x80, //   Physical Minimum (-32768)
    0x46, 0xFF, 0x7F, //   Physical Maximum (32767)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x02,       //   Report Count (2)
    0x09, 0x30,       //   Usage (X)
    0x09, 0x31,       //   Usage (Y)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x33,       //   Usage (Rx)
    0x09, 0x34,       //   Usage (Ry)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x03, //   Logical Maximum (1023)
    0x35, 0x00,       //   Physical Minimum (0)
    0x46, 0xFF, 0x03, //   Physical Maximum (1023)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x02,       //   Report Count (2)
    0x09, 0x32,       //   Usage (Z)
    0x09, 0x35,       //   Usage (Rz)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0xC0              // End Collection
};
} // namespace MacVirtualPadPrivate

class Q_DECL_EXPORT MacVirtualPad
{
  public:
    MacVirtualPad()
        : m_device(nullptr)
        , m_dirty(false)
    {
        resetReport();
    }

    ~MacVirtualPad()
    {
        close();
    }

    bool open()
    {
        if (m_device)
            return true;

        CFMutableDictionaryRef properties = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                                      &kCFTypeDictionaryKeyCallBacks,
                                                                      &kCFTypeDictionaryValueCallBacks);
        if (!properties)
            return false;

        CFNumberRef vendor = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &MacVirtualPadPrivate::kVendorId);
        CFNumberRef product = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &MacVirtualPadPrivate::kProductId);
        CFNumberRef version = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &MacVirtualPadPrivate::kVersion);

        CFDictionarySetValue(properties, CFSTR(kIOHIDVendorIDKey), vendor);
        CFDictionarySetValue(properties, CFSTR(kIOHIDProductIDKey), product);
        CFDictionarySetValue(properties, CFSTR(kIOHIDVersionNumberKey), version);

        CFStringRef manufacturer = CFSTR("AntiMicroX");
        CFStringRef productName = CFSTR("AntiMicroX Virtual Gamepad");
        CFDictionarySetValue(properties, CFSTR(kIOHIDManufacturerKey), manufacturer);
        CFDictionarySetValue(properties, CFSTR(kIOHIDProductKey), productName);

        CFDataRef descriptor = CFDataCreate(kCFAllocatorDefault, MacVirtualPadPrivate::kGamepadReportDescriptor,
                                            sizeof(MacVirtualPadPrivate::kGamepadReportDescriptor));

        if (descriptor)
        {
            CFDictionarySetValue(properties, CFSTR(kIOHIDReportDescriptorKey), descriptor);
            CFRelease(descriptor);
        }

        m_device = IOHIDUserDeviceCreateWithProperties(kCFAllocatorDefault, properties, 0);

        CFRelease(vendor);
        CFRelease(product);
        CFRelease(version);
        CFRelease(properties);

        if (!m_device)
        {
            qWarning() << "Unable to create virtual HID gamepad.";
            return false;
        }

        resetReport();
        IOHIDUserDeviceActivate(m_device);
        sync();
        return true;
    }

    void close()
    {
        if (m_device)
        {
            IOHIDUserDeviceHandleReportWithTimeStamp(m_device, mach_absolute_time(),
                                                     reinterpret_cast<uint8_t *>(&m_report), sizeof(m_report));
            CFRelease(m_device);
            m_device = nullptr;
        }
        resetReport();
    }

    void setButton(int index, bool pressed)
    {
        if (index < 0 || index >= 16)
            return;

        quint16 mask = quint16(1u << index);
        quint16 original = m_report.buttons;

        if (pressed)
            m_report.buttons |= mask;
        else
            m_report.buttons &= ~mask;

        if (m_report.buttons != original)
            m_dirty = true;
    }

    void setHat(int direction)
    {
        quint8 value = 0x0F;
        if (direction >= 0 && direction <= 7)
            value = static_cast<quint8>(direction);

        if (m_report.hat != value)
        {
            m_report.hat = value;
            m_dirty = true;
        }
    }

    void setAxis(int index, qint16 value)
    {
        if (index < 0 || index >= 4)
            return;

        if (m_report.axes[index] != value)
        {
            m_report.axes[index] = value;
            m_dirty = true;
        }
    }

    void setTrigger(int index, qint16 value)
    {
        if (index < 0 || index >= 2)
            return;

        if (m_report.triggers[index] != value)
        {
            m_report.triggers[index] = value;
            m_dirty = true;
        }
    }

    void sync()
    {
        if (!ensureDevice())
            return;

        if (!m_dirty)
            return;

        IOReturn result = IOHIDUserDeviceHandleReportWithTimeStamp(m_device, mach_absolute_time(),
                                                                   reinterpret_cast<uint8_t *>(&m_report),
                                                                   sizeof(m_report));
        if (result != kIOReturnSuccess)
        {
            qWarning() << "IOHIDUserDeviceHandleReport failed" << result;
        }

        m_dirty = false;
    }

  private:
    void resetReport()
    {
        memset(&m_report, 0, sizeof(m_report));
        m_report.hat = 0x0F;
        m_dirty = true;
    }

    bool ensureDevice()
    {
        if (m_device)
            return true;
        return open();
    }

    struct __attribute__((__packed__)) GamepadReport
    {
        quint16 buttons;
        quint8 hat;
        quint8 padding;
        qint16 axes[4];
        qint16 triggers[2];
    };

    IOHIDUserDeviceRef m_device;
    GamepadReport m_report;
    bool m_dirty;
};

#endif // Q_OS_MAC

#endif // MACVIRTUALPAD_H
