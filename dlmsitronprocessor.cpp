#include "dlmsitronprocessor.h"


#include <QtMath>

#define PLG_4_PC         1 //Parametrization
#define MAX_PLG_PREC    9
#define DLMS_MAX_DIFF_SECS  MAX_DIFF_DT_SECS//1200



#include "matildalimits.h"
#include "moji_defy.h"



DlmsItronProcessor::DlmsItronProcessor(QObject *parent) : DlmsProcessor(parent)
{
    lastExchangeState.sourceAddressH = "03";//21 works, but for writting 41 must be used //this fucking meter doesn't want to authorize with another address

}

bool DlmsItronProcessor::isAShortNameMeter(const QString &version)
{
    return (version.isEmpty() || version.contains(" SN"));//be default SN

}

QByteArray DlmsItronProcessor::getItronAddressArrayHex()
{
    return QByteArray("02 23");
}

QByteArray DlmsItronProcessor::getItronAddressArrayHiLo()
{
    return QByteArray("00020023");

}


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

QByteArray DlmsItronProcessor::getTheSecondMessage(const QVariantHash &hashConstData, QVariantHash &hashTmpData)
{
    Q_UNUSED(hashTmpData);
   const QByteArray arrMessageXtend =
                DlmsItronHelper::getAarq(hashConstData, lastExchangeState.lastMeterIsShortDlms,
                                                                                  defPassword4meterVersion(lastExchangeState.lastMeterIsShortDlms));
//                LandisGyrDLMSHelper::getAarqSmpl(lastExchangeState.lastMeterIsShortDlms);

   return crcCalcExt(getItronAddressArrayHex(), HDLC_FRAME_I, lastExchangeState.messageCounterRRR, lastExchangeState.messageCounterSSS, arrMessageXtend);

//    return crcCalcFrameIarr(hashConstData, arrMessageXtend);
}

QByteArray DlmsItronProcessor::defPassword4meterVersion(const QString &version)
{
    return defPassword4meterVersion(isAShortNameMeter(version));
}

QByteArray DlmsItronProcessor::defPassword4meterVersion(const bool &shortDlms)
{
    Q_UNUSED(shortDlms);
    return QByteArray("ABCDEFGH");
}

void DlmsItronProcessor::preparyLogined(const QVariantHash &hashConstData, QVariantHash &hashTmpData, QVariantHash &hashMessage)
{
    preparyLoginedHashTmpData(hashTmpData);



     hashMessage.insert( "message_0",
                         (hashTmpData.value("DLMS_SNRM_ready", false).toBool()) ?
                             getTheSecondMessage(hashConstData, hashTmpData) : //пароль
                             getTheFirstMessage(hashConstData)
                             ); //хто ти ???
}

bool DlmsItronProcessor::messageIsValidItron(const QByteArray &readArr, QVariantList &listMeterMesageVar, QList<QByteArray> &commandCodeH, quint8 &frameType, quint8 &errCode)
{
    lastExchangeState.lastHiLoHex = getItronAddressArrayHiLo();
return messageIsValid(readArr, listMeterMesageVar, commandCodeH,
                       lastExchangeState.lastHiLoHex, frameType, errCode,
                       lastExchangeState.sourceAddressH);
}

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

bool DlmsItronProcessor::decodeMeterDtSnVrsn(const QVariantList &meterMessageVar, const QVariantHash &hashConstData, const QVariantHash &hashTmpData, QVariantHash &hash, quint16 &step, int &warning_counter, int &error_counter, int &goodObis)
{
    const int meterCount = meterMessageVar.size();
    const int obisCount = lastExchangeState.lastObisList.size();
    if(obisCount != meterCount)
        return false;

    goodObis = 0;

    int maxVal4good = 2;
    for(int i = 0, dtObis = 0; i < meterCount; i++){
        if(lastExchangeState.lastObisList.at(i) == CMD_GSET_DATETIME){
            dtObis++;
            if(dtObis > 1){//read dt sett
                maxVal4good = 6;
                break;
            }
        }
    }
    int dtReadyClass = maxVal4good;

    if(lastExchangeState.lastObisList.contains(CMD_GET_MODIFICATION))
        maxVal4good += 2;


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


    QString vrsn, modification;
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
                }


                break;}
            }

            dtReadyClass--;

            break;}

        case CMD_GSET_METER_NUMBER:{
            QByteArray a = meterMessageVar.at(i).toByteArray();//EGM 000209244
            bool ok, ok2;
            a.mid(3,6).toULongLong(&ok);
            a.mid(6).toULongLong(&ok2);
            if((a.length() == 13 && ok && ok2) || (lastExchangeState.lastMeterIsShortDlms && !a.isEmpty())){
                goodObis++;

                if(!lastExchangeState.lastMeterIsShortDlms){
                    a = a.mid(3);
                    if(a.left(1) == "0")
                        a = a.mid(1);
                    if(a.left(1) == "0")
                        a = a.mid(1);
                }
                hash.insert("SN", a);
            }
            break;}

        case CMD_GET_FIRMWARE_VERSION:{
            const QString a = meterMessageVar.at(i).toString();
            const QString asimple = a.simplified().trimmed();
            if(asimple == a || (lastExchangeState.lastMeterIsShortDlms && !asimple.isEmpty())){//G1B F38 - A+, A-; G1B F18 - |A|
                goodObis++;
                vrsn = asimple;
            }
            break;}
        case CMD_GET_MODIFICATION:{
            QString a = meterMessageVar.at(i).toString();
            const QString asimple = a.simplified().trimmed();
            if(asimple == a){//220.F38.P2.C300.V1.R1L5 - A+, A-; 220.F18.P2.C300.V1.R1L5 - |A|
                const QStringList l = asimple.split(".");
                a.clear();
                for(int j = 0, jMax = l.size(); j < jMax; j++){
                    if(a.isEmpty()){
                        if(l.at(j).startsWith("F"))
                            a = l.at(j);
                    }else{
                        if(l.at(j).startsWith("R1")){
                            a.append("." + l.at(j));
                            break;
                        }
                    }
                }
                modification = a;
                goodObis++;} //save as   Fxx or Fxx.R1
            break;}
        }
    }
    if(!vrsn.isEmpty() && !modification.isEmpty())
        hash.insert("vrsn", QString("%1 %2%3").arg(vrsn).arg(modification).arg( lastExchangeState.lastMeterIsShortDlms ? " SN" : ""));

    return  (meterCount > maxVal4good) ? (goodObis == maxVal4good) : (goodObis == meterCount);
}



QByteArray DlmsItronProcessor::preparyTotalEnrg(const QVariantHash &hashConstData, QVariantHash &hashTmpData)
{

    return QByteArray();

}

QVariantHash DlmsItronProcessor::fullCurrent(const QVariantList &meterMessageVar, const quint8 &errCode, quint16 &step, const QVariantHash &hashConstData, const QVariantHash &hashTmpData, const QList<QByteArray> &lastDump, int &warning_counter, int &error_counter)
{

    return QVariantHash();
}

QVariantHash DlmsItronProcessor::fullCurrentShortNames(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData)
{
    return QVariantHash();

}

QVariantHash DlmsItronProcessor::fullVoltageShortNames(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData)
{
    return QVariantHash();

}

QVariantHash DlmsItronProcessor::fullLoadProfileShortNames(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData)
{
    return QVariantHash();

}

QVariantHash DlmsItronProcessor::fullShortNamesExt(const QVariantList &meterMessageVar, const QVariantHash &hashTmpData)
{
    return QVariantHash();

}

void DlmsItronProcessor::fillCurrentShortNamesEnergyKeys(DLMSShortNamesParams &params)
{

}

void DlmsItronProcessor::fillVoltageShortNamesEnergyKeys(DLMSShortNamesParams &params)
{

}

void DlmsItronProcessor::fillLoadProfileShortNamesEnergyKeys(DLMSShortNamesParams &params, const ObisList &DLMS_enrgObisList)
{

}

void DlmsItronProcessor::addTariff2obisList(ObisList &obislist, AttributeList &attrList, const int &lastTariff, const QString &enrgKey, const bool &lastIsShortDlms, bool &hasQuadrants)
{

}

void DlmsItronProcessor::addTariff2obisListSN(ObisList &obislist, AttributeList &attrList, const int &lastTariff, const QString &enrgKey, bool &hasQuadrants)
{

}

void DlmsItronProcessor::addVoltage2obisList(ObisList &obislist, AttributeList &attrList, const QString &enrgKeys, const bool &lastIsShortDlms, const QString &version)
{

}

void DlmsItronProcessor::addVoltage2obisListSN(ObisList &obislist, AttributeList &attrList, const QString &enrgKeys, const QString &version)
{

}

void DlmsItronProcessor::addEnrgKeyTariff2obisListSN(ObisList &obislist, AttributeList &attrList, const quint64 &obis, const DLMSShortNames &shortNameSett)
{

}

QByteArray DlmsItronProcessor::preparyVoltage(const QVariantHash &hashConstData, QVariantHash &hashTmpData)
{
    return QByteArray();
}

QVariantHash DlmsItronProcessor::fullVoltage(const QVariantList &meterMessageVar, const quint8 &errCode, quint16 &step, const QVariantHash &hashConstData, const QVariantHash &hashTmpData, const QList<QByteArray> &lastDump, int &warning_counter, int &error_counter)
{
    return QVariantHash();

}

void DlmsItronProcessor::preparyWriteDT(const QVariantHash &hashConstData, QVariantHash &hashMessage)
{

}

