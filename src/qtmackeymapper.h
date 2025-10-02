#ifndef QTMACKEYMAPPER_H
#define QTMACKEYMAPPER_H

#include "qtkeymapperbase.h"

class QtMacKeyMapper : public QtKeyMapperBase
{
    Q_OBJECT

  public:
    explicit QtMacKeyMapper(QObject *parent = nullptr);

  protected:
    void populateMappingHashes() override;
    void populateCharKeyInformation() override;
};

#endif // QTMACKEYMAPPER_H
