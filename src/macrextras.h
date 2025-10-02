#ifndef MACEXTRAS_H
#define MACEXTRAS_H

#include <QtGlobal>
#include <QString>

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

class MacExtras
{
  public:
    struct FrontmostApplicationInfo
    {
        qint64 windowNumber = 0;
        QString bundleIdentifier;
        QString executablePath;
        QString windowTitle;
        QString ownerName;
        pid_t pid = 0;

        bool isValid() const { return pid > 0; }
    };

    static FrontmostApplicationInfo frontmostApplication();
    static QString applicationPathForPid(pid_t pid);
    static QString bundleIdentifierForPid(pid_t pid);
};

#endif // MACEXTRAS_H
