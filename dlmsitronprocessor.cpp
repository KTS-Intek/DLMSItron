#include "dlmsitronprocessor.h"


#include <QtMath>

#define PLG_4_PC         1 //Parametrization
#define MAX_PLG_PREC    9
#define DLMS_MAX_DIFF_SECS  MAX_DIFF_DT_SECS//1200



#include "matildalimits.h"
#include "moji_defy.h"



//----------------------------------------------------------------------------------

DlmsItronProcessor::DlmsItronProcessor(QObject *parent) : DlmsProcessor(parent)
{
    lastExchangeState.sourceAddressH = "03";//21 works, but for writting 41 must be used //this fucking meter doesn't want to authorize with another address

}

//----------------------------------------------------------------------------------

bool DlmsItronProcessor::isAShortNameMeter(const QString &version)
{
    return (version.isEmpty() || version.contains(" SN"));//be default SN

}

//----------------------------------------------------------------------------------

QByteArray DlmsItronProcessor::getItronAddressArrayHex()
{
    return QByteArray("02 23");
}

//----------------------------------------------------------------------------------

QByteArray DlmsItronProcessor::getItronAddressArrayHiLo()
{
    return QByteArray("00020023");

}

//----------------------------------------------------------------------------------


QByteArray DlmsItronProcessor::getTheFirstMessage(const QVariantHash &hashConstData)
{
//    7E
//    A0 08
//    02 23 - dest address
//    03 - source address 0x01
//    93  SNRM
//    3E 74
//    7E



//    7E
//    A0 23
//    03  - dest address
//    00 02 00 23  - source address
//    73
//    C0 48 -HCS
//    81 80 14  - format id / group id / group len
//            05 02 00 80 - parameter id/len/value  - maximum information field len transmit
//            06 02 00 80  - parameter id/len/value  - maximum information field len receive
//            07 04 00 00 00 01
//            08 04 00 00 00 01
//    CE 6A 7E



    return crcCalcExt(getItronAddressArrayHex(),  HDLC_FRAME_SNRM, lastExchangeState.messageCounterRRR, lastExchangeState.messageCounterSSS, "");

//    return crcCalc(hashConstData, HDLC_FRAME_SNRM, lastExchangeState.messageCounterRRR, lastExchangeState.messageCounterSSS, "");

}

//----------------------------------------------------------------------------------

QByteArray DlmsItronProcessor::getTheSecondMessage(const QVariantHash &hashConstData, QVariantHash &hashTmpData)
{
    Q_UNUSED(hashTmpData);
   const QByteArray arrMessageXtend =
                DlmsItronHelper::getAarq(hashConstData, lastExchangeState.lastMeterIsShortDlms,
                                                                                  defPassword4meterVersion(lastExchangeState.lastMeterIsShortDlms));
//                LandisGyrDLMSHelper::getAarqSmpl(lastExchangeState.lastMeterIsShortDlms);

   return crcCalcFrameIarrItron(arrMessageXtend);//  crcCalcExt(getItronAddressArrayHex(), HDLC_FRAME_I, lastExchangeState.messageCounterRRR, lastExchangeState.messageCounterSSS, arrMessageXtend);

//    return crcCalcFrameIarr(hashConstData, arrMessageXtend);
}

//----------------------------------------------------------------------------------

QByteArray DlmsItronProcessor::defPassword4meterVersion(const QString &version)
{
    return defPassword4meterVersion(isAShortNameMeter(version));
}

//----------------------------------------------------------------------------------

QByteArray DlmsItronProcessor::defPassword4meterVersion(const bool &shortDlms)
{
    Q_UNUSED(shortDlms);
    return QByteArray("ABCDEFGH");
}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::preparyLogined(const QVariantHash &hashConstData, QVariantHash &hashTmpData, QVariantHash &hashMessage)
{
    preparyLoginedHashTmpData(hashTmpData);



     hashMessage.insert( "message_0",
                         (hashTmpData.value("DLMS_SNRM_ready", false).toBool()) ?
                             getTheSecondMessage(hashConstData, hashTmpData) : //пароль
                             getTheFirstMessage(hashConstData)
                             ); //хто ти ???
}

//----------------------------------------------------------------------------------

bool DlmsItronProcessor::messageIsValidItron(const QByteArray &readArr, QVariantList &listMeterMesageVar, QList<QByteArray> &commandCodeH, quint8 &frameType, quint8 &errCode)
{
    lastExchangeState.lastHiLoHex = getItronAddressArrayHiLo();
return messageIsValid(readArr, listMeterMesageVar, commandCodeH,
                       lastExchangeState.lastHiLoHex, frameType, errCode,
                      lastExchangeState.sourceAddressH);
}

//----------------------------------------------------------------------------------

QByteArray DlmsItronProcessor::crcCalcFrameIItron(const ObisList &obisList, const AttributeList &attributeList)
{
// return crcCalcFrameIarr()
    return crcCalcFrameIarrItron(DlmsHelper::arrMessageXtend(lastExchangeState.lastObisList, obisList, attributeList, lastExchangeState.lastMeterIsShortDlms));
}

QByteArray DlmsItronProcessor::crcCalcFrameIItronTotalEnrg(const ObisList &obisList, const AttributeList &attributeList)
{
    QList<QByteArray> cidl;
    cidl.append("0003");

//    if(lastExchangeState.lastMeterIsShortDlms)

    return crcCalcFrameIarrItron(DlmsHelper::arrMessageXtendExt(lastExchangeState.lastObisList, obisList, cidl, attributeList));

}

//----------------------------------------------------------------------------------

QByteArray DlmsItronProcessor::crcCalcFrameIarrItron(const QByteArray &arrMessageXtend)
{
    return crcCalcExt(getItronAddressArrayHex(), HDLC_FRAME_I, lastExchangeState.messageCounterRRR, lastExchangeState.messageCounterSSS, arrMessageXtend);

}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::message2meter(const quint8 &pollCode, const QVariantHash &hashConstData, QVariantHash &hashTmpData, QVariantHash &hashMessage, quint16 &step)
{
    QByteArray writearr;

    switch (pollCode) {

//    case POLL_CODE_READ_METER_STATE     : writearr = preparyMeterState(hashConstData, hashTmpData, step)    ; break;

//    case POLL_CODE_READ_METER_LOGBOOK   : writearr = preparyMeterJournal(hashConstData, hashTmpData)  ; break;

    case POLL_CODE_READ_VOLTAGE         : writearr = preparyVoltage(hashConstData, hashTmpData)       ; break;

//    case POLL_CODE_READ_POWER           : writearr = preparyLoadProfile(hashConstData, hashTmpData)         ; break;

    case POLL_CODE_READ_TOTAL           : writearr = preparyTotalEnrg(hashConstData, hashTmpData)     ; break;

//    case POLL_CODE_READ_END_DAY         : writearr = preparyEoD(hashConstData, hashTmpData)           ; break;

//    case POLL_CODE_READ_END_MONTH       : writearr = preparyEoM(hashConstData, hashTmpData)           ; break;

    default:{ if(verboseMode) qDebug() << "DLMS pollCode not supported 1" << pollCode                       ; break;}

    }

    if(writearr.isEmpty()){
        step = 0xFFFF;
        hashMessage.clear();
    }else{


        hashMessage.insert("message_0", writearr);

    }
}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::decodeMeterData(const quint8 &pollCode, const QVariantList &meterMessageVar, const quint8 &errCode, quint16 &step, const QVariantHash &hashConstData, QVariantHash &hashTmpData, const QList<QByteArray> &commandCodeH, int &warning_counter, int &error_counter)
{
    QVariantHash result;

    switch(pollCode){

    case POLL_CODE_READ_METER_STATE:{
//        result = fullMeterState(meterMessageVar, errCode, step, hashConstData, hashTmpData, commandCodeH, warning_counter, error_counter) ;
        break;}

    case POLL_CODE_READ_METER_LOGBOOK:{
//        result = fullMeterJrnl(meterMessageVar, errCode, step, hashConstData, hashTmpData, commandCodeH, warning_counter, error_counter) ;
        break;}

    case POLL_CODE_READ_VOLTAGE:{
        result = fullVoltage(meterMessageVar, errCode, step, hashConstData, hashTmpData, commandCodeH, warning_counter, error_counter)  ;
        break; }

    case POLL_CODE_READ_POWER:{
//        result = fullLoadProfile(meterMessageVar, step, hashConstData, hashTmpData, warning_counter, error_counter) ;
        break;}

    case POLL_CODE_READ_TOTAL:{
        result = fullCurrent(meterMessageVar, errCode, step, hashConstData, hashTmpData, commandCodeH, warning_counter, error_counter) ;
        break;}

    case POLL_CODE_READ_END_DAY    :{
//        result = fullEoD(meterMessageVar, errCode, step, hashConstData, hashTmpData, commandCodeH, warning_counter, error_counter)  ;
        break;}

    case POLL_CODE_READ_END_MONTH  :{
//        result = fullEoM(meterMessageVar, errCode, step, hashConstData, hashTmpData, commandCodeH, warning_counter, error_counter) ;
        break;}
    }

    if(!result.isEmpty()){
        DlmsHelper::moveCache2hash(result, commandCodeH, hashTmpData, step, error_counter, verboseMode, lastExchangeState.lastErrS);

        if(lastExchangeState.lastMeterIsShortDlms && isItMarkedAsDoneSn2obis(result)){//it tells that short name map is received
            hashTmpData.remove("DLMS_lastSn4obisListLastIndexIsHere");//
            hashTmpData.remove("DLMS_lastSn4obisList");//stop reading short names
        }

    }
}

//----------------------------------------------------------------------------------

bool DlmsItronProcessor::decodeMeterDtSnVrsn(const QVariantList &meterMessageVar, const QVariantHash &hashConstData, const QVariantHash &hashTmpData, QVariantHash &hash, quint16 &step, int &warning_counter, int &error_counter, int &goodObis)
{
    const int meterCount = meterMessageVar.size();
    const int obisCount = lastExchangeState.lastObisList.size();
    if(obisCount != meterCount)
        return false;

    goodObis = 0;


    const int itronStep = hashTmpData.value("ACE_itronStep", 0).toInt() ;

    int maxVal4good = 1;

    int dtReadyClass = itronStep + 1;


    /* maxVal4good = 3;
     * clock_status  DST isEnabled
     * dt str
     *
     * sn
     * vrsn
     *
     * maxVal4good = 7;
     * dst isActivated
     * dst deviation
     * dst end
     * dst begin
     *
     * dt str
     * sn
     * vrsn
     */


    for(int i = 0; i < meterCount && i < obisCount && i < maxVal4good; i++){

        switch(lastExchangeState.lastObisList.at(i)){

        case CMD_GSET_DATETIME:{

            switch(dtReadyClass){
            case 6:{
                //DST_Profile
                hash.insert("DST_Profile", meterMessageVar.at(i).toBool()); goodObis++;
                break;}
            case 5:{
                //deviation
                hash.insert("DLMS_deviation", meterMessageVar.at(i).toInt()); goodObis++; //minutes
                break;}
            case 4:{
                //NormalTime  //Місяць,День тижня,Година,Переведення на г годин
                hash.insert("NormalTime", DlmsItronHelper::arrh2dataTimeDstSett(meterMessageVar.at(i).toByteArray().toHex(),
                                                                               hash.value("DLMS_deviation").toInt(), hash.contains("DLMS_deviation"))); goodObis++;
                break;}
            case 3:{
                //SummerTime
                hash.insert("SummerTime", DlmsItronHelper::arrh2dataTimeDstSett(meterMessageVar.at(i).toByteArray().toHex(),
                                                                               hash.value("DLMS_deviation").toInt(), hash.contains("DLMS_deviation"))); goodObis++;
                break;}
            case 2:{
                const QDateTime dt = DlmsItronHelper::arrh2dataTime(meterMessageVar.at(i).toByteArray().toHex());//
                        //(meterMessageVar.at(i).toByteArray().toHex(), verboseMode, true, QDateTime(), true);//
                if(dt.isValid()){//const bool asLocalTime = true, const QDateTime dtpwrCorr25hour = QDateTime(), const bool ignoredst = false
                    goodObis++;
                    hash.insert("lastMeterDateTime", dt);

                    const bool lockWriteDt = (hashConstData.value("memo").toString().startsWith("DNWT ") || hashConstData.value("dta", false).toBool());
                    const QDateTime currDt = QDateTime::currentDateTime();
                    if(lockWriteDt){
                        //lock write
                        hash.insert("DLMS_timeDiffSec", dt.secsTo(currDt));
                        hash.insert("DLMS_DateTime", dt);

                        if(lastExchangeState.lastMeterIsShortDlms){//Fucking SN DLMS Date time object!!!!
                            const QDateTime dtLocalised = QDateTime::fromString(dt.toString("yyyyMMddhhmmss"), "yyyyMMddhhmmss");
                            if(dtLocalised.isValid()){
                                hash.insert("DLMS_timeDiffSec", dtLocalised.secsTo(currDt));
                                hash.insert("DLMS_DateTime", dtLocalised);
                            }
                        }






                        hash.insert("DLMS_currentDateTime", currDt);
                        if(qAbs(hash.value("DLMS_timeDiffSec").toInt()) > DLMS_MAX_DIFF_SECS){
                            goodObis--;
                            hash.insert("step", 0xFFFF);
                            hash.insert(MeterPluginHelper::errWarnKey(error_counter, true),
                                               MeterPluginHelper::prettyMess(tr("Time correction: disabled, diff.: %1 [s], exiting")
                                                                                           .arg( dt.secsTo(currDt) ),
                                                                             PrettyValues::prettyHexDump(meterMessageVar.at(i).toByteArray(), QByteArray(), 0),
                                                                             lastExchangeState.lastErrS.lastWarning, lastExchangeState.lastErrS.lastErrorStr, true));

                            step = 0xFFFF;
                            break;
                        }
                        hash.insert(MeterPluginHelper::errWarnKey(error_counter, true),
                                    MeterPluginHelper::prettyMess(tr("Time correction: disabled, diff.: %1 [s], ignore")
                                                                  .arg( dt.secsTo(currDt) ),
                                                                  PrettyValues::prettyHexDump(meterMessageVar.at(i).toByteArray(), QByteArray(), 0),
                                                                  lastExchangeState.lastErrS.lastWarning, lastExchangeState.lastErrS.lastErrorStr, false));



                    }



                    if(!lockWriteDt && qAbs(dt.secsTo(currDt)) > hashConstData.value("corDTintrvl", SETT_DT_DIFF_MAX_SECS).toInt()){ //need to correct dateTime  90 sec;
                        hash.insert("corrDateTime", true);
                        hash.insert(MeterPluginHelper::nextMatildaEvntName(hashTmpData), MeterPluginHelper::addEvnt2hash(ZBR_EVENT_DATETIME_NEED2CORR, QDateTime::currentDateTimeUtc(),
                                                                                          tr("Meter date %1 UTC%2%3")
                                                                                          .arg(dt.toString("yyyy-MM-dd hh:mm:ss"))
                                                                                          .arg( (dt.offsetFromUtc() >= 0) ? "+" : "" )
                                                                                          .arg(QString::number(dt.offsetFromUtc()))) );

                        hash.insert(MeterPluginHelper::errWarnKey(warning_counter, false),
                                           MeterPluginHelper::prettyMess(tr("Need to correct time, diff.: %1 [s]").arg( dt.secsTo(currDt) ),
                                                                         PrettyValues::prettyHexDump(meterMessageVar.at(i).toByteArray(), QByteArray(), 0),
                                                                         lastExchangeState.lastErrS.lastWarning, lastExchangeState.lastErrS.lastErrorStr, false));

                        if(!hashConstData.value("corDTallow", false).toBool()){
                            hash.insert(MeterPluginHelper::errWarnKey(error_counter, true),
                                               MeterPluginHelper::prettyMess(tr("Time correction: disabled, diff.: %1 [s], exiting")
                                                                                           .arg( dt.secsTo(currDt) ),
                                                                             PrettyValues::prettyHexDump(meterMessageVar.at(i).toByteArray(), QByteArray(), 0),
                                                                             lastExchangeState.lastErrS.lastWarning, lastExchangeState.lastErrS.lastErrorStr, true));

                            step = 0xFFFF;

                        }
                        if(verboseMode) qDebug() << "DLMS need to correct dateTime" << dt << currDt;
                        break;
                    }
                    hash.insert("DLMS_currentDateTime", currDt);


                    hash.insert("DLMS_DateTime", dt);
                    hash.insert("lastMeterDateTime", dt);
                    updateThisMeterCacheLastDt(hashConstData);

                }


                break;}
            }

            dtReadyClass--;

            break;}

        case CMD_GSET_METER_NUMBER:{
            QByteArray a = meterMessageVar.at(i).toByteArray();//EGM 000209244
            bool ok, ok2;
            a.mid(8,6).toULongLong(&ok);
            a.mid(8).toULongLong(&ok2);
//            ttyUSB0     > 12:04:42.078 7E A0 25 03 00 02 00 23 52 86 20 E6 E7 00 C4 01 ~.%....#R.
//                                       81 00 09 10 41 43 45 36 36 31 4D 41 38 34 35 37 ....ACE661MA8457
//                                       33 35 34 38 E7 6A 7E                            3548.j~
            if((a.length() >= 13 && ok && ok2) || (lastExchangeState.lastMeterIsShortDlms && !a.isEmpty())){
                goodObis++;

                QString vrsn;

                if(!lastExchangeState.lastMeterIsShortDlms){
                    vrsn = a.left(8);

                    a = a.mid(8);
                    if(a.left(1) == "0")
                        a = a.mid(1);
                    if(a.left(1) == "0")
                        a = a.mid(1);
                }
                hash.insert("SN", a);
                hash.insert("vrsn", vrsn);

            }
            break;}

        }
    }

    return  (meterCount > maxVal4good) ? (goodObis == maxVal4good) : (goodObis == meterCount);
}


//----------------------------------------------------------------------------------


QByteArray DlmsItronProcessor::preparyTotalEnrg(const QVariantHash &hashConstData, QVariantHash &hashTmpData)
{
    ObisList listCommand;
    AttributeList listAttribute;

    if(!hashTmpData.value("DLMS_dt_sn_vrsn_ready", false).toBool()){
        quint8 itronStep = hashTmpData.value("ACE_itronStep", 0).toUInt();

        DlmsItronHelper::addObis4readDtSnVrsnDst(itronStep, listCommand, listAttribute, false, lastExchangeState.lastMeterIsShortDlms); //0x20
        hashTmpData.insert("ACE_itronStep", itronStep);

        if(!listCommand.isEmpty())
            return crcCalcFrameIItron(listCommand, listAttribute);

         hashTmpData.insert("DLMS_dt_sn_vrsn_ready", true);

    }

    if(!hashTmpData.contains("ACE_obisCodesHash"))
        hashTmpData.insert("ACE_obisCodesHash",  DlmsItronHelper::getObisCodesTotal4thisMeter(hashConstData, hashTmpData.value("vrsn").toString()));

    const QVariantHash obisCodesHash = hashTmpData.value("ACE_obisCodesHash").toHash();
    const QVariantHash obisCodesScallersHash = hashTmpData.value("ACE_obisCodesScallersHash").toHash();

    if(!obisCodesHash.isEmpty()){
        auto lk = obisCodesHash.keys();
        std::sort(lk.begin(), lk.end());


        const QString enrgKeyFull = lk.first();


        DlmsItronHelper::addTariffAttribute2obisAndAttributeList(listCommand, listAttribute,
                                                                 obisCodesHash.value(enrgKeyFull).toULongLong(),
                                                                 (!obisCodesScallersHash.contains(enrgKeyFull)), lastExchangeState.lastMeterIsShortDlms);

         //1. receive scaller unit
         //2. receive value


        hashTmpData.insert("DLMS_enrgKeyFull", enrgKeyFull);
        return crcCalcFrameIItronTotalEnrg(listCommand, listAttribute);
    }

    return QByteArray();

}

//----------------------------------------------------------------------------------

QVariantHash DlmsItronProcessor::fullCurrent(const QVariantList &meterMessageVar, const quint8 &errCode, quint16 &step, const QVariantHash &hashConstData, const QVariantHash &hashTmpData, const QList<QByteArray> &lastDump, int &warning_counter, int &error_counter)
{


    Q_UNUSED(errCode);
    Q_UNUSED(lastDump);

    QVariantHash hash;
    hash.insert("messFail", true);

    const int meterCount = meterMessageVar.size();
    const int obisCount = lastExchangeState.lastObisList.size();
    if(obisCount != meterCount)
        return hash;

    int goodObis = 0;

//    QString version = hashTmpData.value("vrsn").toString();
    if(!hashTmpData.value("DLMS_dt_sn_vrsn_ready", false).toBool()){
        return getDecodedMeterDtSnVrsn(meterMessageVar, hashConstData, hashTmpData, step, warning_counter, error_counter, goodObis);


    }


    if(meterMessageVar.isEmpty())
        return hash;

     QVariantHash obisCodesHash = hashTmpData.value("ACE_obisCodesHash").toHash();
     QVariantHash obisCodesScallersHash = hashTmpData.value("ACE_obisCodesScallersHash").toHash();


    if(!obisCodesHash.isEmpty()){

        QString enrgKeyFull = hashTmpData.value("DLMS_enrgKeyFull").toString();

        //1. receive scaller unit
        //2. receive value

        bool isValueValid = false;

        if(obisCodesScallersHash.contains(enrgKeyFull)){
         //this is a value

            qreal val = meterMessageVar.first().toReal(&isValueValid) ;
            const qreal s = obisCodesScallersHash.value(enrgKeyFull, 0.1).toDouble();

            if(isValueValid){
                val *= s;
                val *= 0.001 ;//Wh to kWh

                QString valStr = PrettyValues::prettyNumber(val, 4, MAX_PLG_PREC);
                if(verboseMode) qDebug() << "fullCurrent DLMS total value " << val << valStr << s ;

                hash.insert(enrgKeyFull, valStr);

                obisCodesHash.remove(enrgKeyFull);

                hash.insert("ACE_obisCodesHash", obisCodesHash);


            }else{
                if(verboseMode) qDebug() << "fullCurrent bad value " << meterMessageVar.first() << val << s << enrgKeyFull;
            }


        }else{
            //this  is a scaller unit

            const int s = meterMessageVar.first().toMap().value("0", "NaN").toInt(&isValueValid);//scaler
            if(isValueValid){
                const qreal v = qPow(10.0, (qreal)s);
                if(verboseMode) qDebug() << "fullCurrent obis2scaller " << QString::number(obisCodesHash.value(enrgKeyFull).toInt(), 16) << s << v << enrgKeyFull;
                obisCodesScallersHash.insert(enrgKeyFull, v);
                hash.insert("ACE_obisCodesScallersHash", obisCodesScallersHash);

            }else{
                const int obiserr = meterMessageVar.first().toInt(&isValueValid);

                if(isValueValid && (obiserr == ERR_DLMS_BAD_REQUEST || obiserr == ERR_DLMS_BAD_REQUEST_2)){//the meter doesn't support this obis code because of its settings

                    updateThisMeterCacheUnsupportedTotalEnergKey(hashConstData, enrgKeyFull);

                    markThisAsUnsupported(hash, obisCodesHash, obisCodesScallersHash, enrgKeyFull, hashTmpData);

                }else
                    if(verboseMode) qDebug() << "fullCurrent bad scaller_unit " << meterMessageVar.first() << s << isValueValid << enrgKeyFull;
            }
        }

        if(isValueValid){
            hash.insert("messFail", false);
            getenrgKeyFullCheckedTotal(hash, obisCodesHash, obisCodesScallersHash, hashConstData, hashTmpData);
        }
        hash.insert("DLMS_enrgKeyFull", "");


        if(obisCodesHash.isEmpty()){
            //there is nothing to receive
            step = 0xFFFF;

        }
//        return hash;

    }



    return hash;


}

//----------------------------------------------------------------------------------

QVariantHash DlmsItronProcessor::fullCurrentShortNames(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData)
{
    return QVariantHash();

}

//----------------------------------------------------------------------------------

QVariantHash DlmsItronProcessor::fullVoltageShortNames(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData)
{
    return QVariantHash();

}

//----------------------------------------------------------------------------------

QVariantHash DlmsItronProcessor::fullLoadProfileShortNames(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData)
{
    return QVariantHash();

}

//----------------------------------------------------------------------------------

QVariantHash DlmsItronProcessor::fullShortNamesExt(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData)
{
    return QVariantHash();

}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::fillCurrentShortNamesEnergyKeys(DLMSShortNamesParams &params)
{

}

void
//----------------------------------------------------------------------------------
 DlmsItronProcessor::fillVoltageShortNamesEnergyKeys(DLMSShortNamesParams &params)
{

}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::fillLoadProfileShortNamesEnergyKeys(DLMSShortNamesParams &params, const ObisList &DLMS_enrgObisList)
{

}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::addTariff2obisList(ObisList &obislist, AttributeList &attrList, const int &lastTariff, const QString &enrgKey, const bool &lastIsShortDlms, bool &hasQuadrants)
{

}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::addTariff2obisListSN(ObisList &obislist, AttributeList &attrList, const int &lastTariff, const QString &enrgKey, bool &hasQuadrants)
{

}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::addVoltage2obisList(ObisList &obislist, AttributeList &attrList, const QString &enrgKeys, const bool &lastIsShortDlms, const QString &version)
{

}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::addVoltage2obisListSN(ObisList &obislist, AttributeList &attrList, const QString &enrgKeys, const QString &version)
{

}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::addEnrgKeyTariff2obisListSN(ObisList &obislist, AttributeList &attrList, const quint64 &obis, const DLMSShortNames &shortNameSett)
{

}

//----------------------------------------------------------------------------------

QByteArray DlmsItronProcessor::preparyVoltage(const QVariantHash &hashConstData, QVariantHash &hashTmpData)
{
    ObisList listCommand;
    AttributeList listAttribute;

    if(!hashTmpData.value("DLMS_dt_sn_vrsn_ready", false).toBool()){
        quint8 itronStep = hashTmpData.value("ACE_itronStep", 0).toUInt();

        DlmsItronHelper::addObis4readDtSnVrsnDst(itronStep, listCommand, listAttribute, false, lastExchangeState.lastMeterIsShortDlms); //0x20
        hashTmpData.insert("ACE_itronStep", itronStep);

        if(!listCommand.isEmpty())
            return crcCalcFrameIItron(listCommand, listAttribute);

         hashTmpData.insert("DLMS_dt_sn_vrsn_ready", true);

    }

    if(!hashTmpData.contains("ACE_obisCodesHash"))
        hashTmpData.insert("ACE_obisCodesHash",  DlmsItronHelper::getObisCodesVoltage4thisMeter(hashConstData, hashTmpData.value("vrsn").toString()));

    const QVariantHash obisCodesHash = hashTmpData.value("ACE_obisCodesHash").toHash();
    const QVariantHash obisCodesScallersHash = hashTmpData.value("ACE_obisCodesScallersHash").toHash();

    if(!obisCodesHash.isEmpty()){
        auto lk = obisCodesHash.keys();
        std::sort(lk.begin(), lk.end());


        const QString enrgKeyFull = lk.first();


        DlmsItronHelper::addTariffAttribute2obisAndAttributeList(listCommand, listAttribute,
                                                                 obisCodesHash.value(enrgKeyFull).toULongLong(),
                                                                 (!obisCodesScallersHash.contains(enrgKeyFull)), lastExchangeState.lastMeterIsShortDlms);

         //1. receive scaller unit
         //2. receive value


        hashTmpData.insert("DLMS_enrgKeyFull", enrgKeyFull);
        return crcCalcFrameIItronTotalEnrg(listCommand, listAttribute);
    }

    return QByteArray();
}

//----------------------------------------------------------------------------------

QVariantHash DlmsItronProcessor::fullVoltage(const QVariantList &meterMessageVar, const quint8 &errCode, quint16 &step, const QVariantHash &hashConstData, const QVariantHash &hashTmpData, const QList<QByteArray> &lastDump, int &warning_counter, int &error_counter)
{
    Q_UNUSED(errCode);
    Q_UNUSED(lastDump);

    QVariantHash hash;
    hash.insert("messFail", true);

    const int meterCount = meterMessageVar.size();
    const int obisCount = lastExchangeState.lastObisList.size();
    if(obisCount != meterCount)
        return hash;

    int goodObis = 0;

//    QString version = hashTmpData.value("vrsn").toString();
    if(!hashTmpData.value("DLMS_dt_sn_vrsn_ready", false).toBool()){
        return getDecodedMeterDtSnVrsn(meterMessageVar, hashConstData, hashTmpData, step, warning_counter, error_counter, goodObis);


    }


    if(meterMessageVar.isEmpty())
        return hash;

     QVariantHash obisCodesHash = hashTmpData.value("ACE_obisCodesHash").toHash();
     QVariantHash obisCodesScallersHash = hashTmpData.value("ACE_obisCodesScallersHash").toHash();
     QVariantHash obis2precision = hashTmpData.value("ACE_obis2precision").toHash();


    if(!obisCodesHash.isEmpty()){

        QString enrgKeyFull = hashTmpData.value("DLMS_enrgKeyFull").toString();

        //1. receive scaller unit
        //2. receive value

        bool isValueValid = false;
        const quint64 obisCode = obisCodesHash.value(enrgKeyFull).toULongLong();

        if(obisCodesScallersHash.contains(enrgKeyFull)){
         //this is a value

            qreal val = meterMessageVar.first().toReal(&isValueValid) ;
            const qreal s = obisCodesScallersHash.value(enrgKeyFull, 0.1).toDouble();

            if(isValueValid){

                const qreal scaler = DlmsItronHelper::getScaller4obisCode(obisCode);
                int plgPrec = obis2precision.value(QString::number(obisCode)).toInt();


                val *= s;
                val *= scaler;// 0.001 ;//Wh to kWh


                if(scaler != 1.0)
                    plgPrec += 3;

                if(plgPrec < 0)
                    plgPrec = 0;


                QString valStr = PrettyValues::prettyNumber(val, plgPrec, MAX_PLG_PREC);
                if(verboseMode) qDebug() << "fullCurrent DLMS total value " << val << valStr << s << enrgKeyFull;


                if(!valStr.isEmpty() && hashTmpData.contains(QString("DLMS_%1").arg(enrgKeyFull))){
                    bool ok;
                    const qreal otherv = hashTmpData.value(QString("DLMS_%1").arg(enrgKeyFull)).toDouble(&ok);
                    if(ok){
                        val += otherv;
                        valStr = PrettyValues::prettyNumber(val, plgPrec, MAX_PLG_PREC);
                    }

                }

                hash.insert(enrgKeyFull, valStr);

                obisCodesHash.remove(enrgKeyFull);

                hash.insert("ACE_obisCodesHash", obisCodesHash);


            }else{
                if(verboseMode) qDebug() << "fullCurrent bad value " << meterMessageVar.first() << val << s << enrgKeyFull;
            }


        }else{
            //this  is a scaller unit

            const int s = meterMessageVar.first().toMap().value("0", "NaN").toInt(&isValueValid);//scaler
            if(isValueValid){
                const qreal v = qPow(10.0, (qreal)s);
                if(verboseMode) qDebug() << "fullCurrent obis2scaller " << QString::number(obisCodesHash.value(enrgKeyFull).toInt(), 16) << s << v << enrgKeyFull;
                obisCodesScallersHash.insert(enrgKeyFull, v);
                hash.insert("ACE_obisCodesScallersHash", obisCodesScallersHash);


                obis2precision.insert(QString::number(obisCode), s * (-1));
                hash.insert("ACE_obis2precision", obis2precision);


            }else{
                const int obiserr = meterMessageVar.first().toInt(&isValueValid);

                if(isValueValid && (obiserr == ERR_DLMS_BAD_REQUEST || obiserr == ERR_DLMS_BAD_REQUEST_2)){//the meter doesn't support this obis code because of its settings

                    updateThisMeterCacheUnsupportedVoltageKey(hashConstData, enrgKeyFull);

                    markThisAsUnsupported(hash, obisCodesHash, obisCodesScallersHash, enrgKeyFull, hashTmpData);
                    obis2precision.insert(QString::number(obisCode), 0);
                    hash.insert("ACE_obis2precision", obis2precision);

                }else
                    if(verboseMode) qDebug() << "fullCurrent bad scaller_unit " << meterMessageVar.first() << s << isValueValid << enrgKeyFull;
            }
        }

        if(isValueValid){
            hash.insert("messFail", false);
            getenrgKeyFullCheckedVoltage(hash, obisCodesHash, obisCodesScallersHash, hashConstData, hashTmpData);
        }
        hash.insert("DLMS_enrgKeyFull", "");


        if(obisCodesHash.isEmpty()){
            //there is nothing to receive
            step = 0xFFFF;

        }
//        return hash;

    }



    return hash;

}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::preparyWriteDT(QVariantHash &hashMessage)
{
//    192.168.88.41  > 20:18:17.506 7E A0 28 02 23 03 DC 5C AE E6 E6 00 C1 01 C1 00 ~.(.#..\
//                                   08 00 00 01 00 00 FF 02 00 09 0C 07 E5 07 07 FF
//                                   14 12 11 FF FF 4C 80 01 67 7E
    hashMessage.insert("message_0",
                       crcCalcFrameIarrItron("E6 E6 00" + DlmsItronHelper::addObis4writeDt(lastExchangeState.lastObisList, lastExchangeState.lastMeterIsShortDlms)));
}
//----------------------------------------------------------------------------------

QVariantHash DlmsItronProcessor::getDecodedMeterDtSnVrsn(const QVariantList &meterMessageVar, const QVariantHash &hashConstData, const QVariantHash &hashTmpData, quint16 &step, int &warning_counter, int &error_counter, int &goodObis)
{
    QVariantHash hash;
    hash.insert("messFail", true);


    if(!decodeMeterDtSnVrsn(meterMessageVar, hashConstData, hashTmpData, hash, step, warning_counter, error_counter, goodObis))
        return hash;


    const QString meterni = hashConstData.value("NI").toString();



    const quint8 itronStep = hashTmpData.value("ACE_itronStep", 0).toUInt() + 1;
    hash.insert("ACE_itronStep", itronStep);
    hash.insert("messFail", false);


    if(itronStep == 1 && acecache.contains(meterni) && acecache.value(meterni).lastDt.isValid()){//read dt once a day

        const auto secsdiff = qAbs(QDateTime::currentDateTimeUtc().secsTo(acecache.value(meterni).lastDt));

        if(secsdiff < 86400){ //check it onece a day
            hash.insert("ACE_itronStep", 0xFF);

        }

    }


    return hash;
}
//----------------------------------------------------------------------------------

void DlmsItronProcessor::updateThisMeterCacheLastDt(const QVariantHash &hashConstData)
{
    const QString meterni = hashConstData.value("NI").toString();
    auto onemeter = acecache.value(meterni);

    onemeter.lastDt = QDateTime::currentDateTimeUtc();

    acecache.insert(meterni, onemeter);
}
//----------------------------------------------------------------------------------

void DlmsItronProcessor::updateThisMeterCacheUnsupportedTotalEnergKey(const QVariantHash &hashConstData, const QString &enrgKeyFull)
{
    const QString meterni = hashConstData.value("NI").toString();
    auto onemeter = acecache.value(meterni);

    if(onemeter.lkUnsupportedTotalEnrg.contains(enrgKeyFull))
        return;
    onemeter.lkUnsupportedTotalEnrg.append(enrgKeyFull);

    acecache.insert(meterni, onemeter);
}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::updateThisMeterCacheUnsupportedVoltageKey(const QVariantHash &hashConstData, const QString &enrgKeyFull)
{
    const QString meterni = hashConstData.value("NI").toString();
    auto onemeter = acecache.value(meterni);

    if(onemeter.lkUnsupportedVoltage.contains(enrgKeyFull))
        return;
    onemeter.lkUnsupportedVoltage.append(enrgKeyFull);

    acecache.insert(meterni, onemeter);
}
//----------------------------------------------------------------------------------

void DlmsItronProcessor::markThisAsUnsupported(QVariantHash &hash, QVariantHash &obisCodesHash, QVariantHash &obisCodesScallersHash, const QString &enrgKeyFull, const QVariantHash &hashTmpData)
{
    obisCodesScallersHash.insert(enrgKeyFull, 1.0);
    hash.insert("ACE_obisCodesScallersHash", obisCodesScallersHash);

    hash.insert(enrgKeyFull, "!");

    if(enrgKeyFull.startsWith("T1_") && hashTmpData.contains(QString("T0_%1").arg(enrgKeyFull.right(2))))//T0_  <Ax/Rx>
        hash.insert(enrgKeyFull, hashTmpData.value(QString("T0_%1").arg(enrgKeyFull.right(2))));


    obisCodesHash.remove(enrgKeyFull);
    hash.insert("ACE_obisCodesHash", obisCodesHash);
}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::getenrgKeyFullCheckedTotal(QVariantHash &hash, QVariantHash &obisCodesHash, QVariantHash &obisCodesScallersHash, const QVariantHash &hashConstData, const QVariantHash &hashTmpData)
{

    if(hashTmpData.value("ACE_enrgKeyFullChecked", false).toBool())
        return;

    const QString meterni = hashConstData.value("NI").toString();
    auto onemeter = acecache.value(meterni);

    if(onemeter.lkUnsupportedTotalEnrg.isEmpty())
        return;


    auto lk = obisCodesHash.keys();
    if(lk.isEmpty())
        return;

    std::sort(lk.begin(), lk.end());


    if(!lk.first().startsWith("T0")){

        //check it once, after all necessary T0 are received
        hash.insert("ACE_enrgKeyFullChecked", true);


        for(int i = 0, imax = lk.size(); i < imax; i++){
         const QString enrgKeyFull = lk.at(i);
            if(onemeter.lkUnsupportedTotalEnrg.contains(enrgKeyFull)){
                obisCodesHash.remove(enrgKeyFull);
                markThisAsUnsupported(hash, obisCodesHash, obisCodesScallersHash, enrgKeyFull, hashTmpData);

            }
        }
    }

}

//----------------------------------------------------------------------------------

void DlmsItronProcessor::getenrgKeyFullCheckedVoltage(QVariantHash &hash, QVariantHash &obisCodesHash, QVariantHash &obisCodesScallersHash, const QVariantHash &hashConstData, const QVariantHash &hashTmpData)
{
    if(hashTmpData.value("ACE_enrgKeyFullChecked", false).toBool())
        return;

    const QString meterni = hashConstData.value("NI").toString();
    auto onemeter = acecache.value(meterni);

    if(onemeter.lkUnsupportedVoltage.isEmpty())
        return;


    auto lk = obisCodesHash.keys();
    if(lk.isEmpty())
        return;

    std::sort(lk.begin(), lk.end());

    //check it once, after all necessary T0 are received
    hash.insert("ACE_enrgKeyFullChecked", true);


    for(int i = 0, imax = lk.size(); i < imax; i++){
        const QString enrgKeyFull = lk.at(i);
        if(onemeter.lkUnsupportedVoltage.contains(enrgKeyFull)){
            obisCodesHash.remove(enrgKeyFull);
            markThisAsUnsupported(hash, obisCodesHash, obisCodesScallersHash, enrgKeyFull, hashTmpData);

        }
    }

}


//----------------------------------------------------------------------------------
