#include "macrextras.h"

#ifdef Q_OS_MAC

#include <QByteArray>
#include <QDebug>
#include <QString>

#import <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CGWindow.h>
#include <libproc.h>

namespace {
QString cfStringToQString(CFStringRef str)
{
    if (!str)
        return {};

    CFIndex length = CFStringGetLength(str);
    if (length == 0)
        return QString();

    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    QByteArray buffer(static_cast<int>(maxSize), Qt::Uninitialized);
    if (CFStringGetCString(str, buffer.data(), maxSize, kCFStringEncodingUTF8))
        return QString::fromUtf8(buffer.constData());

    return QString();
}
} // namespace

QString MacExtras::applicationPathForPid(pid_t pid)
{
    char pathBuffer[PROC_PIDPATHINFO_MAXSIZE] = {0};
    const int result = proc_pidpath(pid, pathBuffer, sizeof(pathBuffer));

    if (result > 0)
        return QString::fromUtf8(pathBuffer);

    return QString();
}

QString MacExtras::bundleIdentifierForPid(pid_t pid)
{
    QString identifier;
    NSRunningApplication *app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];

    if (app.bundleIdentifier != nil)
        identifier = QString::fromUtf8(app.bundleIdentifier.UTF8String);

    return identifier;
}

MacExtras::FrontmostApplicationInfo MacExtras::frontmostApplication()
{
    FrontmostApplicationInfo info;

    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
    if (!windowList)
        return info;

    const CFIndex count = CFArrayGetCount(windowList);
    for (CFIndex index = 0; index < count; ++index)
    {
        CFDictionaryRef window = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, index));
        if (!window)
            continue;

        CFNumberRef layerNum = static_cast<CFNumberRef>(CFDictionaryGetValue(window, kCGWindowLayer));
        int layer = 0;
        if (layerNum)
            CFNumberGetValue(layerNum, kCFNumberIntType, &layer);

        if (layer != 0)
            continue;

        CFNumberRef windowNumber = static_cast<CFNumberRef>(CFDictionaryGetValue(window, kCGWindowNumber));
        if (windowNumber)
            CFNumberGetValue(windowNumber, kCFNumberLongLongType, &info.windowNumber);

        CFNumberRef ownerPid = static_cast<CFNumberRef>(CFDictionaryGetValue(window, kCGWindowOwnerPID));
        if (ownerPid)
            CFNumberGetValue(ownerPid, kCFNumberSInt32Type, &info.pid);

        CFStringRef ownerName = static_cast<CFStringRef>(CFDictionaryGetValue(window, kCGWindowOwnerName));
        info.ownerName = cfStringToQString(ownerName);

        CFStringRef windowTitle = static_cast<CFStringRef>(CFDictionaryGetValue(window, kCGWindowName));
        info.windowTitle = cfStringToQString(windowTitle);

        if (info.pid > 0)
        {
            info.executablePath = applicationPathForPid(info.pid);
            info.bundleIdentifier = bundleIdentifierForPid(info.pid);
        }

        if (info.isValid())
            break;
    }

    CFRelease(windowList);
    return info;
}

#endif // Q_OS_MAC
