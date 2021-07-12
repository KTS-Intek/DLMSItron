#ifndef DLMSITRONHELPER_H
#define DLMSITRONHELPER_H

#include "dlmshelper.h"

class DlmsItronHelper : public DlmsHelper
{

public:
    static QStringList getSupportedEnrg(const quint8 &code, const QString &version);

    static bool getVersion(const QVariantHash &hashConstData, const QVariantHash &hashTmpData);

    static bool getVersionCache(const QVariantHash &hashConstData, QVariantHash &hashTmpData);


    static bool isPollCodeSupported(const quint16 &pollCode, QVariantHash &hashTmpData);

    static bool isPollCodeSupported(const quint16 &pollCode);

    static int calculateEnrgIndx(qint16 currEnrg, const quint8 &pollCode, const QStringList &listEnrg, QString version, const bool &verboseMode);



    static QByteArray getAarq(const QVariantHash &hashConstData, const bool &lastMeterIsShortDlms, const QByteArray &defPassword);

//    static QByteArray getAarqSmpl(const bool &lastMeterIsShortDlms);

//    static QByteArray getValidPassword(const QVariantHash &hashConstData, const bool &retInHex, const QByteArray &defPasswd);

    static void addObis4readDtSnVrsnDst(ObisList &obislist, AttributeList &attrList, const bool &addDst, const bool &getVersion, const bool &lastIsShortDlms);

};

#endif // DLMSITRONHELPER_H
