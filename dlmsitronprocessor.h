#ifndef DLMSITRONPROCESSOR_H
#define DLMSITRONPROCESSOR_H


#include "dlmsprocessor.h"

#include "dlmsitronhelper.h"


class DlmsItronProcessor : public DlmsProcessor
{
    Q_OBJECT
public:
    explicit DlmsItronProcessor(QObject *parent = nullptr);

    bool isAShortNameMeter(const QString &version);


    QByteArray getItronAddressArrayHex();
    QByteArray getItronAddressArrayHiLo();

    QByteArray getTheFirstMessage(const QVariantHash &hashConstData);

    QByteArray getTheSecondMessage(const QVariantHash &hashConstData, QVariantHash &hashTmpData);


    QByteArray defPassword4meterVersion(const QString &version);

    QByteArray defPassword4meterVersion(const bool &shortDlms);

    void preparyLogined(const QVariantHash &hashConstData, QVariantHash &hashTmpData, QVariantHash &hashMessage);

    bool messageIsValidItron(const QByteArray &readArr, QVariantList &listMeterMesageVar, QList<QByteArray> &commandCodeH, quint8 &frameType, quint8 &errCode);


    QByteArray crcCalcFrameIItron(const ObisList &obisList, const AttributeList &attributeList) ;

    QByteArray crcCalcFrameIarrItron(const QByteArray &arrMessageXtend) ;



    void message2meter(const quint8 &pollCode, const QVariantHash &hashConstData, QVariantHash &hashTmpData, QVariantHash &hashMessage, quint16 &step);

    void decodeMeterData(const quint8 &pollCode, const QVariantList &meterMessageVar, const quint8 &errCode, quint16 &step,
                         const QVariantHash &hashConstData, QVariantHash &hashTmpData, const QList<QByteArray> &commandCodeH, int &warning_counter, int &error_counter);

    bool decodeMeterDtSnVrsn(const QVariantList &meterMessageVar, const QVariantHash &hashConstData, const QVariantHash &hashTmpData, QVariantHash &hash, quint16 &step, int &warning_counter, int &error_counter, int &goodObis);



//    DLMSShortNamesParams getShortNames2obis(const QVariantList &meterMessageVar, const QList<quint16> &lastShortNames2get, bool &lastWasEmpty);


    QByteArray preparyTotalEnrg(const QVariantHash &hashConstData, QVariantHash &hashTmpData);

    QVariantHash fullCurrent(const QVariantList &meterMessageVar, const quint8 &errCode, quint16 &step,
                           const QVariantHash &hashConstData, const QVariantHash &hashTmpData, const QList<QByteArray> &lastDump, int &warning_counter, int &error_counter);


    QVariantHash fullCurrentShortNames(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData);

    QVariantHash fullVoltageShortNames(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData);

    QVariantHash fullLoadProfileShortNames(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData);


    QVariantHash fullShortNamesExt(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData);



    void fillCurrentShortNamesEnergyKeys(DLMSShortNamesParams &params);
    void fillVoltageShortNamesEnergyKeys(DLMSShortNamesParams &params);
    void fillLoadProfileShortNamesEnergyKeys(DLMSShortNamesParams &params, const ObisList &DLMS_enrgObisList);


    void addTariff2obisList(ObisList &obislist, AttributeList &attrList, const int &lastTariff, const QString &enrgKey, const bool &lastIsShortDlms, bool &hasQuadrants);

    void addTariff2obisListSN(ObisList &obislist, AttributeList &attrList, const int &lastTariff, const QString &enrgKey, bool &hasQuadrants);


    void addVoltage2obisList(ObisList &obislist, AttributeList &attrList, const QString &enrgKeys, const bool &lastIsShortDlms, const QString &version);

    void addVoltage2obisListSN(ObisList &obislist, AttributeList &attrList,  const QString &enrgKeys, const QString &version);



    void addEnrgKeyTariff2obisListSN(ObisList &obislist, AttributeList &attrList, const quint64 &obis, const DLMSShortNames &shortNameSett);

    QByteArray preparyVoltage(const QVariantHash &hashConstData, QVariantHash &hashTmpData);

    QVariantHash fullVoltage(const QVariantList &meterMessageVar, const quint8 &errCode, quint16 &step,
                           const QVariantHash &hashConstData, const QVariantHash &hashTmpData, const QList<QByteArray> &lastDump, int &warning_counter, int &error_counter);


    void preparyWriteDT(QVariantHash &hashMessage);




//    QByteArray preparyLoadProfile(const QVariantHash &hashConstData, QVariantHash &hashTmpData);


//    QVariantHash fullLoadProfile(const QVariantList &meterMessageVar, quint16 &step,
//                           const QVariantHash &hashConstData, const QVariantHash &hashTmpData, int &warning_counter, int &error_counter);

//    QVariantHash fullLoadProfileEmptyVals(const QVariantHash &hashConstData, const QVariantHash &hashTmpData, const quint32 &pwrIntrvl);


//    void getObises4scallerUnitLoadProfile(ObisList &listCommand, AttributeList &listAttribute, const ObisList &enrg_obisList);


};

#endif // DLMSITRONPROCESSOR_H
