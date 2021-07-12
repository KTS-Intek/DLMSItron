#include "dlmsitron.h"

#define PLG_4_PC         1 //Parametrization


#include "myucdevicetypes.h"
#include "definedpollcodes.h"



//------------------------------------------------------------------------------------------------------

QString DlmsItron::getMeterType()
{
    return QString("DLMSItron");
}

//------------------------------------------------------------------------------------------------------

QString DlmsItron::getMeterAddrsAndPsswdRegExp()
{
    return QString("%1%2").arg("^(1[6-9]{1,3}|[2-9][0-9]{1,2}|[1-9][0-9]{2,3}|1[0-5][0-9]{3}|16[0-2][0-9]{2}|163[0-7][0-9]|1638[0-3])$").arg("^([0-9A-Za-z]{8})$");
}

//------------------------------------------------------------------------------------------------------

quint16 DlmsItron::getPluginVersion()
{
    return PLG_VER_RELEASE;
}

//------------------------------------------------------------------------------------------------------

quint8 DlmsItron::getPasswdType()
{
    return UCM_PSWRD_TEXT;
}

//------------------------------------------------------------------------------------------------------

QString DlmsItron::getVersion()
{
    return QString("DLMSItron v0.0.1 %1").arg(QString(BUILDDATE));
}

//------------------------------------------------------------------------------------------------------

QByteArray DlmsItron::getDefPasswd()
{
    return dataprocessor.defPassword4meterVersion("");
}

//------------------------------------------------------------------------------------------------------

QString DlmsItron::getSupportedMeterList()
{
    return QString("ACE6000");
}

//------------------------------------------------------------------------------------------------------

quint8 DlmsItron::getMaxTariffNumber(const QString &version)
{
    Q_UNUSED(version); return 4;
}

//------------------------------------------------------------------------------------------------------

QStringList DlmsItron::getEnrgList4thisMeter(const quint8 &pollCode, const QString &version)
{
    return DlmsItronHelper::getSupportedEnrg(pollCode, version);
}

//------------------------------------------------------------------------------------------------------

quint8 DlmsItron::getMeterTypeTag()
{
    return UC_METER_ELECTRICITY;
}

//------------------------------------------------------------------------------------------------------

Mess2meterRezult DlmsItron::mess2meter(const Mess2meterArgs &pairAboutMeter)
{
    const QVariantHash hashConstData = pairAboutMeter.hashConstData;
    QVariantHash hashTmpData = pairAboutMeter.hashTmpData;
    QVariantHash hashMessage;

    const quint8 pollCode = hashConstData.value("pollCode").toUInt();
    if(!DlmsItronHelper::isPollCodeSupported(pollCode, hashTmpData))
        return Mess2meterRezult(hashMessage,hashTmpData);

    bool go2exit = false;
    dataprocessor.verboseMode = hashConstData.value("verboseMode").toBool();

    if(!hashTmpData.contains("DLMSLGZ_ISREADY")){ //for init
        dataprocessor.lastExchangeState.lastMeterIsShortDlms = hashConstData.value("vrsn").toString().contains("SN");//Short Names
        hashTmpData.insert("DLMSLGZ_ISREADY", true);
    }
    dataprocessor.lastExchangeState.lastHiLoHex.clear();

    quint16 step = hashTmpData.value("step", (quint16)0).toUInt();

    if(dataprocessor.verboseMode) qDebug() << "mess2Meter " << step << pollCode << dataprocessor.lastExchangeState.lastMeterIsShortDlms;

    /*
     * only for this plugin
     * hashTmpData key
     * corrDateTime     - QString   - korektsiya daty chasu
     * logined          - bool      - (false) if(true) logined good
     * sprtdVls         - bool      - (false) if(true) not supported values = '-'
     * DLMS_DTgood      - bool      - (false) if(true) dateTime not read
     *
     *
*/

    if(!hashTmpData.value("logined", false).toBool()){

        dataprocessor.preparyLogined(hashConstData, hashTmpData, hashMessage);


        go2exit = true;
    }

    if(!go2exit && hashTmpData.value("corrDateTime", false).toBool()){

        dataprocessor.preparyWriteDT(hashConstData, hashMessage);
        go2exit = true;
    }

    if(!go2exit){
        dataprocessor.message2meter(pollCode, hashConstData, hashTmpData, hashMessage, step);


    }

    if(hashMessage.isEmpty()){
        step = 0xFFFF;
    }else{
        dataprocessor.addDefaultReadingParams(hashMessage);
    }



    hashTmpData.insert("step", step);
    if(hashTmpData.value("step").toInt() >= 0xFFFE)
        hashMessage.clear();

    return Mess2meterRezult(hashMessage,hashTmpData);

}

//------------------------------------------------------------------------------------------------------

QVariantHash DlmsItron::decodeMeterData(const DecodeMeterMess &threeHash)
{
    const QVariantHash hashConstData = threeHash.hashConstData;
    const QVariantHash hashRead = threeHash.hashRead;
    QVariantHash hashTmpData = threeHash.hashTmpData;


    if(dataprocessor.verboseMode){
        foreach (QString key, hashRead.keys()) {
            qDebug() << "DLMS read "  << key << hashRead.value(key).toByteArray().toHex().toUpper();
        }
    }


    hashTmpData.insert("messFail", true);
    quint8 pollCode     = hashConstData.value("pollCode").toUInt();
    quint16 step        = hashTmpData.value("step", (quint16)0).toUInt();
    int error_counter   = qMax(0, hashTmpData.value("error_counter", 0).toInt());
    int warning_counter = qMax(0, hashTmpData.value("warning_counter", 0).toInt());



    QVariantList meterMessageVar;
    QList<QByteArray> commandCodeH;
    quint8 frameType, errCode;
    bool messValid = false;



    if(!dataprocessor.messageIsValidItron(hashRead.value("readArr_0").toByteArray(), meterMessageVar, commandCodeH,
                                      frameType, errCode)){
        if(dataprocessor.verboseMode) qDebug() << "! messageIsValid( hashRead.val" <<  frameType << meterMessageVar << commandCodeH << errCode;
        hashTmpData.insert(MeterPluginHelper::errWarnKey(error_counter, true),
                           MeterPluginHelper::prettyMess(tr("incomming data is not valid"),
                                                         PrettyValues::prettyHexDump( hashRead.value("readArr_0").toByteArray().toHex(), "", 0),
                                                         dataprocessor.lastExchangeState.lastErrS.lastErrorStr,
                                                         dataprocessor.lastExchangeState.lastErrS.lastWarning, true));

        hashTmpData.insert("logined", false);
        dataprocessor.lastExchangeState.lastMeterH.clear();
        pollCode = 0;
        step = 0;
    }else{
        if(dataprocessor.verboseMode) qDebug() << QByteArray::number(frameType, 16).toUpper() << commandCodeH << meterMessageVar << errCode << frameType;

        messValid = true;
        const QBitArray sss = ConvertAtype::uint8ToBitArray(frameType);
        quint8 s = 0x0;
        for(int i = 5; i < 8; i++){
            if(sss.at(i)){

                switch(i){
                case 7: s += 4; break;
                case 6: s += 2; break;
                case 5: s++; break;
                }

            }
        }

        if(dataprocessor.verboseMode)
            qDebug() << "messisvalid messageCounterSSS= " << frameType << s << dataprocessor.lastExchangeState.messageCounterSSS;
        dataprocessor.lastExchangeState.messageCounterSSS = s;


    }


    if(dataprocessor.exitCozNAuth(errCode, hashTmpData)){
        hashTmpData.insert("error_counter", error_counter);
        hashTmpData.insert("warning_counter", warning_counter);
        return hashTmpData;
    }


    if(!hashTmpData.value("logined", false).toBool()){
        dataprocessor.fullLogined(meterMessageVar, frameType, hashTmpData);
        pollCode = 0;
    }

    if(pollCode > 0 && errCode == ERR_BAD_OBIS){
        pollCode = 0;
    }


    if(pollCode > 0 && hashTmpData.value("corrDateTime", false).toBool()){


        if(errCode == ERR_WRITE_DONE ||
                (dataprocessor.lastExchangeState.lastMeterIsShortDlms && errCode == ERR_ALL_GOOD && !meterMessageVar.isEmpty() && meterMessageVar.first().toString().isEmpty())){
            hashTmpData.insert("corrDateTime", false);
            if(dataprocessor.verboseMode) qDebug() << "DLMS date good" ;

            MeterPluginHelper::insertEvnt2hash(hashTmpData, ZBR_EVENT_DATETIME_CORR_DONE,
                                               tr("Meter new date %1 UTC%2%3")
                                               .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                               .arg( (QDateTime::currentDateTime().offsetFromUtc() >= 0) ? "+" : "" )
                                               .arg(QString::number(QDateTime::currentDateTime().offsetFromUtc())));
        }else{

            MeterPluginHelper::insertEvnt2hash(hashTmpData, ZBR_EVENT_DATETIME_NOT_CORR, tr("Correct date: fail") );

            hashTmpData.insert(MeterPluginHelper::errWarnKey(error_counter, true),
                               MeterPluginHelper::prettyMess(tr("Correct date: fail"), "",
                                                             dataprocessor.lastExchangeState.lastErrS.lastErrorStr,
                                                             dataprocessor.lastExchangeState.lastErrS.lastWarning, true));

            if(dataprocessor.verboseMode) qDebug() << "can't correct date time " << errCode;
        }


        //        hashTmpData.remove("DLMS_dt_sn_vrsn_ready");
        pollCode = 0;
    }


    if(pollCode > 0)
        dataprocessor.decodeMeterData(pollCode, meterMessageVar, errCode, step, hashConstData, hashTmpData, commandCodeH, warning_counter, error_counter);

    if(error_counter > 0 && dataprocessor.verboseMode){
        //        hashTmpData.insert("step", (quint16)0);

        int i = hashTmpData.value("DLMS_lastErr").toInt();
        for( ; i < error_counter; i++)
            qDebug() << "DLMS error " << hashTmpData.value(QString("Error_%1").arg(i));

        hashTmpData.insert("DLMS_lastErr", i);

    }

    if(messValid && hashTmpData.value("logined", false).toBool()){
        dataprocessor.lastExchangeState.messageCounterRRR++;
        if(dataprocessor.lastExchangeState.messageCounterRRR > 7)
            dataprocessor.lastExchangeState.messageCounterRRR = 0;
    }


    //sometimes meter stop answering, so to prevent time wasting, detect it and stop
    if(hashTmpData.value("DLMS_FRAME_UA_COUNTER", 0).toInt() > 20){
        hashTmpData.insert(MeterPluginHelper::errWarnKey(error_counter, true),
                           MeterPluginHelper::prettyMess(tr("no access to meter %1").arg(hashTmpData.value("DLMS_FRAME_UA_COUNTER", 0).toInt()), "",
                                                         dataprocessor.lastExchangeState.lastErrS.lastErrorStr,
                                                         dataprocessor.lastExchangeState.lastErrS.lastWarning, true));


        step = 0xFFFF;
    }

    hashTmpData.insert("step", step);
    hashTmpData.insert("error_counter", error_counter);
    hashTmpData.insert("warning_counter", warning_counter);
    if(dataprocessor.lastExchangeState.lastMeterH.isEmpty())
        dataprocessor.lastExchangeState.lastObisList.clear();

    if(step >= 0xFFFE)
        qDebug() << "finita la comedia " << step;
    return hashTmpData;
}

//------------------------------------------------------------------------------------------------------

QVariantHash DlmsItron::helloMeter(const QVariantHash &hashMeterConstData)
{
    QVariantHash hash;
    /*
    * bool data7EPt = hashMessage.value("data7EPt", false).toBool();
    * QByteArray endSymb = hashMessage.value("endSymb", QByteArray("\r\n")).toByteArray();
    * QByteArray currNI = hashMeterConstData.value("NI").toByteArray();
    * hashMessage.value("message")
*/

    hash.insert("data7EPt", false);
    hash.insert("quickCRC", true);
    hash.insert("endSymb", QByteArray::fromHex("7E"));
    hash.insert("message", dataprocessor.crcCalc(hashMeterConstData, HDLC_FRAME_SNRM, 0,0, ""));
    //7E 52 FF FF FE FF DC 46 7E
    return hash;
}

//------------------------------------------------------------------------------------------------------

QString DlmsItron::meterTypeFromMessage(const QByteArray &readArr)
{
    QList<QByteArray> commandCodeH;
    QVariantList meterMessageVar;

    quint8 frameType, errCode;
    if(! dataprocessor.messageIsValid( readArr, meterMessageVar, commandCodeH, "", frameType, errCode)){
        if(dataprocessor.verboseMode) qDebug() << "! messageIsValid( hashRead.val" <<  frameType << meterMessageVar << commandCodeH << errCode;
        return "";
    }else{
        if(dataprocessor.verboseMode) qDebug() << QByteArray::number(frameType, 16).toUpper() << commandCodeH <<  meterMessageVar << errCode;
    }

    if(frameType == HDLC_FRAME_UA){
        return getMeterType();
    }else{
        if(dataprocessor.verboseMode){
            qDebug() << commandCodeH << meterMessageVar << frameType << (frameType == HDLC_FRAME_UA);

        }
    }

    return "";
}

//------------------------------------------------------------------------------------------------------

QVariantHash DlmsItron::isItYour(const QByteArray &arr)
{
    QList<quint8> mas;
    quint8 masSize;

    for(int i = 0, iMax = arr.length(); i < iMax; i++)
        mas.append((quint8)arr.at(i));

    while(!mas.isEmpty()){
        if(mas.first() != 0x7E){
            mas.removeFirst();
        }else
            break;
    }

    while(!mas.isEmpty()){
        if(mas.last() != 0x7E)
            mas.removeFirst();
        else
            break;
    }

    masSize = (quint8)mas.size();
    if(masSize < 12){
        if(dataprocessor.verboseMode) qDebug() << "masSize=" << masSize << mas << arr;
        return QVariantHash();
    }

    if(mas.first() != 0x7E || mas.last() != 0x7E){
        if(dataprocessor.verboseMode) qDebug() << "if(mas.first() != 0x7E || mas.last() != 0x7E){" << masSize << mas << arr;
        return QVariantHash();
    }

    mas.removeFirst();
    mas.removeLast();
    masSize -= 2;


    if(mas.at(1) != masSize || (mas.first() != 0xA0 && mas.first() != 0xA8) ){
        if(dataprocessor.verboseMode) qDebug() << "if(mas.at(1) != masSize || mas.first() != 0xA0){" << mas.at(1) << mas.first() << masSize;
        return QVariantHash();
    }



    QByteArray messCrc("");
    messCrc.prepend((quint8)mas.takeLast());
    messCrc.prepend((quint8)mas.takeLast());
    masSize -= 2;

    //    if(true){
    //        quint8 byte7d = 0x7D;
    //        quint8 byte5e = 0x5E, byte5d = 0x5D;
    //        for(int i = 0, iMax = mas.size(); i < iMax; i++){
    //            if(mas.at(i) == byte7d && (i + 1) < iMax){
    //                if(mas.at(i+1) == byte5e || mas.at(i+1) == byte5d){
    //                    mas.replace(i, mas.at(i+1) == byte5e ? 0x7E : 0x7D);
    //                    mas.removeAt(i + 1);
    //                    iMax--;
    //                    masSize--;
    //                }
    //            }
    //        }
    //    }

    QByteArray frameArr("");
    for(int i = 0; i < masSize; i++)
        frameArr.append((quint8)mas.at(i));

    //    qDebug() << "frameArr=" << frameArr.toHex().toUpper();


    QByteArray crc16ccit = QByteArray::number( qChecksum(frameArr.constData(), frameArr.length()) , 16).rightJustified(4, '0');
    crc16ccit = QByteArray::fromHex(crc16ccit.right(2) + crc16ccit.left(2));

    if(crc16ccit != messCrc){
        if(dataprocessor.verboseMode) qDebug() << "crc16ccit != messCrc " << crc16ccit.toHex() << messCrc.toHex();
        return QVariantHash();
    }

    if(mas.size() > 8){
        QByteArray crc16h = QByteArray::number( qChecksum(frameArr.left(8).constData(), 8) , 16).rightJustified(4,'0');
        crc16h = QByteArray::fromHex(crc16h.right(2) + crc16h.left(2));

        QByteArray messCrc("");
        messCrc.prepend((quint8)mas.at(9));
        messCrc.prepend((quint8)mas.at(8));


        if(crc16h != messCrc){
            if(dataprocessor.verboseMode) qDebug() << "crc16h != messCrc " << crc16h.toHex() << messCrc.toHex();
            return QVariantHash();
        }

    }


    //    QByteArray arr = QByteArray::fromHex("A0 1C FE FC 7A 61 21 10 80 4D E6 E6 00 C0 01 81 00 08 00 00 01 00 00 FF 02 00 65 D7");
    //    QByteArray arr = QByteArray::fromHex("A0 1C 80 00 7c b3 21 10 80 4D E6 E6 00 C0 01 81 00 08 00 00 01 00 00 FF 02 00 65 D7");
    //    QList<quint8> mas;
    //    for(int i = 0, iMax = arr.length(); i < iMax; i++)
    //        mas.append((quint8)arr.at(i));

    quint16 destLow , destHigh;

    frameArr.clear();
    frameArr.append((quint8)mas.at(2));
    frameArr.append((quint8)mas.at(3));
    bool ok;
    destHigh = frameArr.toHex().toUShort(&ok, 16);

    if(destHigh != 0x2 || !ok){
        if(dataprocessor.verboseMode) qDebug() << "if(destHigh != 0x01 || !ok) " << destHigh << ok << frameArr.toHex() << mas.at(2) << mas.at(3);
        return QVariantHash();
    }

    frameArr.clear();
    frameArr.append((quint8)mas.at(4));
    frameArr.append((quint8)mas.at(5));
    destLow = frameArr.toHex().toUShort(&ok, 16);
    QByteArray modemNI;
    destLow = destLow >> 1;

    if(destLow > 0x7FFF || !ok){
        if(dataprocessor.verboseMode) qDebug() << "if(destLow < 0x3FFF || !ok) " << destLow << ok << frameArr.toHex() << mas.at(4) << mas.at(5);
        return QVariantHash();
    }


    //    destLow &= ~(1 << 8);

    frameArr.clear();
    frameArr = QByteArray::number(destLow, 16).rightJustified(4, '0');
    quint8 byteR = frameArr.left(2).toUShort(0,16);
    quint8 byteL = frameArr.right(2).toUShort(0,16);
    byteL |=  ((byteR % 2) != 0) << 7 ;
    byteR = byteR >> 1;


    modemNI.append(QByteArray::number(QByteArray(QByteArray::number(byteR, 16).rightJustified(2, '0') + QByteArray::number(byteL, 16).rightJustified(2, '0')).toUInt(&ok,16)));




    if(ok){
        if(modemNI.toUpper() == "16383")
            modemNI = "\r\n";
        else{
            quint32 modemNIingr = modemNI.toUInt();
            if(modemNIingr > 1000){
                modemNIingr -= 1000;
                modemNI = QByteArray::number(modemNIingr);
            }
            modemNI.append("\r\n");
        }

        QVariantHash hash;
        hash.insert("nodeID", modemNI);
        hash.insert("endSymb", QByteArray::fromHex("7E"));
        hash.insert("quickCRC", true);
        hash.insert("readLen", 9);

        return hash;
        //7DtEpt

    }

    if(dataprocessor.verboseMode) qDebug() << "not dlms ";


    return QVariantHash();
}

//------------------------------------------------------------------------------------------------------

QVariantHash DlmsItron::isItYourRead(const QByteArray &arr)
{
    QList<QByteArray> commandCodeH;
    QVariantList meterMessageVar;

    quint8 frameType, errCode;
    if(dataprocessor.messageIsValid(arr, meterMessageVar, commandCodeH, "", frameType, errCode)){
        if(dataprocessor.verboseMode) qDebug() << "1719 this is " << getMeterType() ;

        if(errCode == ERR_CHECK_AUTHORIZE && (!arr.contains(QByteArray::fromHex("7E")) || arr.length() < 10 )){
            return QVariantHash();

        }

        QVariantHash hash;
        hash.insert("Tak", true);
        return hash;
    }
    return QVariantHash();
}

//------------------------------------------------------------------------------------------------------

QByteArray DlmsItron::niChanged(const QByteArray &arr)
{
    Q_UNUSED(arr);
    return QByteArray();
}

//------------------------------------------------------------------------------------------------------

QVariantHash DlmsItron::meterSn2NI(const QString &meterSn)
{
    /*
     * hard - без варіантів (жорстко), від старого до нового ф-ту
     * altr - альтернатива для стандартного варіанту
     * <keys> : <QStringList>
     */

    QVariantHash h;
    bool ok;
    quint64 sn = meterSn.toULongLong(&ok);
    QStringList listNI;

    if(meterSn.length() < 9 && !meterSn.isEmpty()){
        //        listNI.append(DlmsItronHelper::calcMeterAddr(sn, 1000));
        listNI.append(DlmsHelper::calcMeterAddr(sn, 0));
        //        listNI.removeDuplicates();

    }
    if(!listNI.isEmpty())
        h.insert("hard", listNI);

    return h;
}

//------------------------------------------------------------------------------------------------------

Mess2meterRezult DlmsItron::messParamPamPam(const Mess2meterArgs &pairAboutMeter)
{

#ifndef PLG_4_PC
    Q_UNUSED(pairAboutMeter);
    return Mess2meterRezult();
#else

    const QVariantHash hashConstData = pairAboutMeter.hashConstData;
    QVariantHash hashTmpData = pairAboutMeter.hashTmpData;
    QVariantHash hashMessage;
    quint8 pollCode = hashConstData.value("pollCode").toUInt();
    quint16 step = hashTmpData.value("step", (quint16)0).toUInt();
    dataprocessor.verboseMode = hashConstData.value("verboseMode").toBool();



    if(pollCode == POLL_CODE_WRITE_METER_ADDR){
        if(dataprocessor.verboseMode) qDebug() << "DLMS write meter addr  notsup";
        hashTmpData.insert("notsup", true);
        hashTmpData.insert("step", 0xFFFF);
        return Mess2meterRezult(hashMessage,hashTmpData);
    }


    const QString version = hashTmpData.value("vrsn").toString().isEmpty() ?
                hashConstData.value("vrsn").toString() :
                hashTmpData.value("vrsn").toString();

    if(pollCode == POLL_CODE_RELAY_OPERATIONS){ // && !DlmsItronHelper::isRelaySupported(version)){
        MeterPluginHelper::addRelayStatusAll(hashTmpData, RELAY_STATE_NOT_SUP, RELAY_STATE_NOT_SUP);
        hashTmpData.insert("notsup", true);
        hashTmpData.insert("step", 0xFFFF);
        return Mess2meterRezult(hashMessage,hashTmpData);
    }

    if(!hashTmpData.contains("DLMSLGZ_ISREADY")){ //for init, replace all DLMSGAMA_ISREADY here
        dataprocessor.lastExchangeState.lastMeterIsShortDlms = hashConstData.value("vrsn").toString().contains("SN");//Short Names
        hashTmpData.insert("DLMSLGZ_ISREADY", true);

    }



    if(!hashTmpData.value("logined", false).toBool()){
        dataprocessor.preparyLogined(hashConstData, hashTmpData, hashMessage);
        pollCode = 0;
    }

    const bool getVersion = DlmsItronHelper::getVersionCache(hashConstData, hashTmpData);


    ObisList listCommand;
    AttributeList listAttribute;
    bool useHdlcFrameI = true;



    switch(pollCode){

    case POLL_CODE_READ_DATE_TIME_DST:{

//        DlmsItronHelper::addObis4readDtSnVrsnDst(listCommand, listAttribute, true, getVersion ? "" : version, dataprocessor.lastExchangeState.lastMeterIsShortDlms); //0x20

        break;}

    case POLL_CODE_WRITE_DATE_TIME:{

        if(!hashTmpData.value("DLMS_dt_sn_vrsn_ready", false).toBool()){
            //first
//            DlmsItronHelper::addObis4readDtSnVrsnDst(listCommand, listAttribute, true, getVersion ? "" : version, dataprocessor.lastExchangeState.lastMeterIsShortDlms); //0x20
        }else{


            if(hashTmpData.value("DLMS_DateReady", false).toBool()){
                //third
//                DlmsItronHelper::addObis4readDtSnVrsnDst(listCommand, listAttribute, true, getVersion ? "" : version, dataprocessor.lastExchangeState.lastMeterIsShortDlms); //0x20

            }else{
                //second
                useHdlcFrameI = false;
                dataprocessor.preparyWriteDT(hashConstData, hashMessage);

            }
        }

        break;}

//    case POLL_CODE_RELAY_OPERATIONS:{

//        //it is impossible to check this methods yet, so it is disabled
//        if(DlmsItronHelper::getVersionCache(hashConstData, hashTmpData)){
//            //            DlmsItronHelper::addObis4readDtSnVrsnDst(listCommand, listAttribute, true, getVersion, dataprocessor.lastExchangeState.lastMeterIsShortDlms);
//            hashTmpData.insert("DlmsGamaReadVersion", true);

//        }else{


//            if(!version.contains("R1") && !dataprocessor.lastExchangeState.lastMeterIsShortDlms){
//                if(dataprocessor.verboseMode) qDebug() << "DLMS relay_all notsup";

//                MeterPluginHelper::addRelayStatusAll(hashTmpData, RELAY_STATE_NOT_SUP, RELAY_STATE_NOT_SUP);

//                hashTmpData.insert("notsup", true);
//                hashTmpData.insert("step", 0xFFFF);
//                return Mess2meterRezult(hashMessage,hashTmpData);
//            }


//            for(int i = 0; i < 3; i++)
//                listCommand.append(CMD_GSET_RELAY);

//            switch(hashConstData.value("relay_all").toInt()){

//            case RELAY_LOAD_ON:{
//                if(!hashTmpData.value("DlmsGama_RelayCmd", false).toBool()){
//                    //                    hashMessage.insert("message_0", dataprocessor.relaySwitch(hashConstData, true));
//                    break;}
//            }//CMD_RELE_ON

//            case RELAY_LOAD_OFF:{
//                if(!hashTmpData.value("DlmsGama_RelayCmd", false).toBool()){
//                    //                    hashMessage.insert("message_0", dataprocessor.relaySwitch(hashConstData, false));
//                    break;}
//            }//CMD_RELE_OFF

//            default:{
//                if(dataprocessor.lastExchangeState.lastMeterIsShortDlms){
//                    listCommand.clear();
//                    listCommand.append(CMD_GSET_RELAY);
//                    listAttribute.append(CMD_GSET_RELAY_SN);
//                }else
//                    listAttribute << 2 << 3 << 4;
//                break;}//RELAY_READ read the state
//            }

//            if(listAttribute.isEmpty()){
//                listCommand.clear();
//                useHdlcFrameI = false;
//            }
//        }

//        break;}

    default:{
        useHdlcFrameI = false;
        if(pollCode != 0){
            if(dataprocessor.verboseMode) qDebug() << "DLMS pollcode notsup " << pollCode;
            hashTmpData.insert("notsup", true);
            hashTmpData.insert("step", 0xFFFF);
            hashMessage.clear();
            return Mess2meterRezult(hashMessage,hashTmpData);
        }
        break;}
    }

    if(useHdlcFrameI && !listCommand.isEmpty()){
        hashMessage.insert("message_0", dataprocessor.crcCalcFrameI(hashConstData, listCommand, listAttribute));

    }

    if(hashMessage.isEmpty()){
        step = 0xFFFF;
    }else{

        dataprocessor.addDefaultReadingParams(hashMessage);
    }

    hashTmpData.insert("step", step);
    if(hashTmpData.value("step").toInt() >= 0xFFFE)
        hashMessage.clear();

    return Mess2meterRezult(hashMessage,hashTmpData);

#endif
}

//------------------------------------------------------------------------------------------------------

QVariantHash DlmsItron::decodeParamPamPam(const DecodeMeterMess &threeHash)
{

#ifdef PLG_4_PC
    const QVariantHash hashConstData = threeHash.hashConstData;
    const QVariantHash hashRead = threeHash.hashRead;

    QVariantHash hashTmpData = threeHash.hashTmpData;
    if(dataprocessor.verboseMode){
        foreach (QString key, hashRead.keys()) {
            qDebug() << "DLMS read "  << key << hashRead.value(key).toByteArray().toHex().toUpper();
        }
    }


    hashTmpData.insert("messFail", true);
    quint8 pollCode = hashConstData.value("pollCode").toUInt();
    quint16 step = hashTmpData.value("step", (quint16)0).toUInt();

    int error_counter = qMax(0, hashTmpData.value("error_counter", 0).toInt());
    int warning_counter = qMax(0, hashTmpData.value("warning_counter", 0).toInt());

    QVariantList meterMessageVar;
    QList<QByteArray> commandCodeH;
    quint8 frameType, errCode;
    bool messIsValid = false;


    if(!dataprocessor.messageIsValidItron( hashRead.value("readArr_0").toByteArray(), meterMessageVar, commandCodeH,
                                      frameType, errCode)){
        if(dataprocessor.verboseMode) qDebug() << "! messageIsValid( hashRead.val" <<  frameType << meterMessageVar << commandCodeH << errCode;
        hashTmpData.insert(MeterPluginHelper::errWarnKey(error_counter, true),
                           MeterPluginHelper::prettyMess(tr("incomming data is not valid"),
                                                         PrettyValues::prettyHexDump( hashRead.value("readArr_0").toByteArray().toHex(), "", 0),
                                                         dataprocessor.lastExchangeState.lastErrS.lastErrorStr,
                                                         dataprocessor.lastExchangeState.lastErrS.lastWarning, true));

        hashTmpData.insert("logined", false);
        dataprocessor.lastExchangeState.lastMeterH.clear();

        pollCode = 0;
        step = 0;
    }else{
        if(dataprocessor.verboseMode) qDebug() << QByteArray::number(frameType, 16).toUpper() << commandCodeH << meterMessageVar << errCode << frameType;
        messIsValid = true;
        const QBitArray sss = ConvertAtype::uint8ToBitArray(frameType);
        quint8 s = 0x0;
        for(int i = 5; i < 8; i++){
            if(sss.at(i)){

                switch(i){
                case 7: s += 4; break;
                case 6: s += 2; break;
                case 5: s++; break;
                }

            }
        }

        qDebug() << "messisvalid = " << frameType << s << dataprocessor.lastExchangeState.messageCounterSSS;
        dataprocessor.lastExchangeState.messageCounterSSS = s;

    }



    if(dataprocessor.exitCozNAuth(errCode, hashTmpData)){
        hashTmpData.insert("error_counter", error_counter);
        hashTmpData.insert("warning_counter", warning_counter);
        return hashTmpData;
    }

    if(!hashTmpData.value("logined", false).toBool()){
        dataprocessor.fullLogined(meterMessageVar, frameType, hashTmpData);
        pollCode = 0;
    }



    if(pollCode > 0 && errCode == ERR_BAD_OBIS){
        pollCode = 0;
    }


    switch(pollCode){

    case POLL_CODE_READ_DATE_TIME_DST:{

        QVariantHash hash;
        int goodObis = 0;

        if(dataprocessor.decodeMeterDtSnVrsn(meterMessageVar, hashConstData, hashTmpData, hash, step, warning_counter, error_counter, goodObis)){
            QList<QString> lk = hash.keys();
            for(int i = 0, iMax = lk.size(); i < iMax; i++)
                hashTmpData.insert(lk.at(i), hash.value(lk.at(i)));
            hashTmpData.insert("messFail", false);
            step = 0xFFFF;
        }

        break;}

    case POLL_CODE_WRITE_DATE_TIME:{

        if(!hashTmpData.value("DLMS_dt_sn_vrsn_ready", false).toBool()){
            QVariantHash hash;
            int goodObis = 0;
            if(dataprocessor.decodeMeterDtSnVrsn(meterMessageVar, hashConstData, hashTmpData, hash, step, warning_counter, error_counter, goodObis)){

                const QList<QString> lk = hash.keys();
                for(int i = 0, iMax = lk.size(); i < iMax; i++)
                    hashTmpData.insert(lk.at(i), hash.value(lk.at(i)));
                hashTmpData.insert("messFail", false);
                hashTmpData.insert("DLMS_dt_sn_vrsn_ready", true);

                //                the next step is to write dt
            }



        }else{

            if(hashTmpData.value("DLMS_DateReady", false).toBool()){
                QVariantHash hash;
                int goodObis = 0;
                if(dataprocessor.decodeMeterDtSnVrsn(meterMessageVar, hashConstData, hashTmpData, hash, step, warning_counter, error_counter, goodObis)){



                    const QList<QString> lk = hash.keys();
                    for(int i = 0, iMax = lk.size(); i < iMax; i++)
                        hashTmpData.insert(lk.at(i), hash.value(lk.at(i)));
                    hashTmpData.insert("messFail", false);
                    step = 0xFFFF;
                }
            }else{


                if(dataprocessor.lastExchangeState.lastMeterIsShortDlms && errCode == ERR_ALL_GOOD && !meterMessageVar.isEmpty() && meterMessageVar.first().toString().isEmpty())
                    hashTmpData.insert("DLMS_DateReady", true);
                else
                    hashTmpData.insert("DLMS_DateReady", (errCode == ERR_WRITE_DONE));


                if(hashTmpData.value("DLMS_DateReady").toBool())
                    hashTmpData.insert("messFail", false);

            }
        }

        break;}

    case POLL_CODE_RELAY_OPERATIONS:{


        if(hashTmpData.value("DlmsGamaReadVersion", false).toBool()){
            QVariantHash hash;
            int goodObis = 0;
            if(dataprocessor.decodeMeterDtSnVrsn(meterMessageVar, hashConstData, hashTmpData, hash, step, warning_counter, error_counter, goodObis)){

                const QList<QString> lk = hash.keys();
                for(int i = 0, iMax = lk.size(); i < iMax; i++)
                    hashTmpData.insert(lk.at(i), hash.value(lk.at(i)));

                hashTmpData.insert("messFail", false);
                hashTmpData.insert("DlmsGamaReadVersion", false);
            }
            break;
        }

        const int meterCount = meterMessageVar.size();
        const int obisCount = dataprocessor.lastExchangeState.lastObisList.size();

        if(dataprocessor.lastExchangeState.lastMeterIsShortDlms){
            if(!meterMessageVar.isEmpty()){
                hashTmpData.insert("messFail", !(errCode == ERR_ALL_GOOD));


                const int relaymode = hashTmpData.value("DlmsGama_RelayCmd").toBool() ?
                            RELAY_READ : hashConstData.value("relay_all").toInt();
                //read relay after writing
                switch(relaymode){
                case RELAY_LOAD_ON:{
                    if(errCode == ERR_ALL_GOOD)
                        hashTmpData.insert("DlmsGama_RelayCmd", true);
                    break;}

                case RELAY_LOAD_OFF:{
                    if(errCode == ERR_ALL_GOOD)
                        hashTmpData.insert("DlmsGama_RelayCmd", true);
                    break; }

                default:{
                    const QString stts = meterMessageVar.first().toString().rightJustified(2, '0');

                    MeterPluginHelper::addRelayStatusAll(hashTmpData,
                                                         (stts.right(1).toInt() != 0) ? RELAY_STATE_LOAD_ON : RELAY_STATE_LOAD_OFF,
                                                         (stts.left(1).toInt() != 0) ? RELAY_STATE_LOAD_ON : RELAY_STATE_LOAD_OFF);
                    hashTmpData.insert("relay_all_power_on", (stts.right(1).toInt() != 0));

                    step = 0xFFFF;
                    break; }
                }
            }
            break;
        }

        if(obisCount == 1){
            if(errCode == ERR_WRITE_DONE || errCode == ERR_ALL_GOOD){
                hashTmpData.insert("DlmsGama_RelayCmd", true);

                if(dataprocessor.verboseMode) qDebug() << "DLMS relay good" ;
                hashTmpData.insert("messFail", false);

            }else{
                hashTmpData.insert(MeterPluginHelper::errWarnKey(error_counter, true),
                                   MeterPluginHelper::prettyMess(tr("Relay: fail"), "",
                                                                 dataprocessor.lastExchangeState.lastErrS.lastErrorStr,
                                                                 dataprocessor.lastExchangeState.lastErrS.lastWarning, true));

                if(dataprocessor.verboseMode) qDebug() << "can't write relay " << errCode;
            }
        }else{
            int goodObis = 0;
            for(int i = 0; i < meterCount && i < obisCount ; i++){

                //                switch(operation){
                //                case RELAY_LOAD_ON  : command = CMD_RELE_ON    ; break;
                //                case RELAY_LOAD_OFF : command = CMD_RELE_OFF   ; break;

                //            //    case RELAY2_LOAD_ON  : break; //MTX meters've only one relay
                //            //    case RELAY2_LOAD_OFF : break;

                //                    case RELAY_READ      : break; ///READS ALL REALAYS
                //                default             : command = CMD_CURENT_STATUS_METER; break;
                //                }
                switch(i){
                case 0:{
                    MeterPluginHelper::addRelayStatusAll(hashTmpData, meterMessageVar.at(i).toBool() ? RELAY_STATE_LOAD_ON : RELAY_STATE_LOAD_OFF, RELAY_STATE_NOT_SUP);
                    hashTmpData.insert("relay_all_power_on", meterMessageVar.at(i).toBool());
                    goodObis++;
                    break;}
                case 1:{
                    QString state;

                    quint8 mainstts = RELAY_STATE_UNKN;
                    //                    switch((int)meterMessageVar.at(i).toByteArray().at(0)){
                    //                    case DLMS_RELAY_STATE_LOAD_OFF   : state = tr("Load OFF")       ; mainstts = RELAY_STATE_LOAD_OFF; break;
                    //                    case DLMS_RELAY_STATE_LOAD_ON    : state = tr("Load ON")        ; mainstts = RELAY_STATE_LOAD_ON;break;
                    //                    case DLMS_RELAY_STATE_WAIT4BUTTON: state = tr("Wait For Button"); mainstts = RELAY_STATE_OFF_PRSBTTN; hashTmpData.insert("Wait4button", true); break;
                    //                    default: state = tr("Unknown"); break;
                    //                    }
                    MeterPluginHelper::addRelayStatusAll(hashTmpData, mainstts, RELAY_STATE_NOT_SUP);


                    hashTmpData.insert("DlmsGamaRelayState", meterMessageVar.at(i));
                    hashTmpData.insert(MeterPluginHelper::errWarnKey(warning_counter, true),
                                       MeterPluginHelper::prettyMess(tr("Relay State: %1").arg( state ), "",
                                                                     dataprocessor.lastExchangeState.lastErrS.lastErrorStr,
                                                                     dataprocessor.lastExchangeState.lastErrS.lastWarning, false));
                    goodObis++;
                    break;}

                case 2:{
                    QString mode;
                    //                    switch((int)meterMessageVar.at(i).toByteArray().at(0)){
                    //                    case DLMS_RELAY_MODE_REMOTE_BUTTON: mode = tr("Remote With Button")         ; break;
                    //                    case DLMS_RELAY_MODE_REMOTE       : mode = tr("Remote")                     ; break;
                    //                    case DLMS_RELAY_MODE_LOCAL        : mode = tr("Local")                      ; break;
                    //                    default: mode = tr("Unknown"); break;
                    //                    }

                    hashTmpData.insert("DlmsGamaRelayMode", meterMessageVar.at(i));
                    hashTmpData.insert(MeterPluginHelper::errWarnKey(warning_counter, true),
                                       MeterPluginHelper::prettyMess(tr("Relay Mode: %1").arg(mode), "",
                                                                     dataprocessor.lastExchangeState.lastErrS.lastErrorStr,
                                                                     dataprocessor.lastExchangeState.lastErrS.lastWarning, false));
                    goodObis++;
                    break;}
                }

            }


            if(goodObis == 3){
                hashTmpData.insert("messFail", false);
                step = 0xFFFF;
            }
        }
        break;}

    default: {
        if(pollCode != 0)
            step = 0xFFFF;
        break;}

    }

    if(dataprocessor.verboseMode){
        foreach (QString key, hashRead.keys()) {
            qDebug() << "DLMS read "  << key << hashRead.value(key).toByteArray().toHex().toUpper();
        }
    }

    if(messIsValid && hashTmpData.value("logined", false).toBool()){
        dataprocessor.lastExchangeState.messageCounterRRR++;
        if(dataprocessor.lastExchangeState.messageCounterRRR > 7)
            dataprocessor.lastExchangeState.messageCounterRRR = 0;
    }

    hashTmpData.insert("step", step);

    //    if(dataprocessor.verboseMode) qDebug() << "2573" << itIsData << stepUp << step << hashTmpData.value("step").toUInt() << error_counter<< commandCodeList << lastNodeID << meterMessHash;
    hashTmpData.insert("error_counter", error_counter);
    hashTmpData.insert("warning_counter", warning_counter);

    if(dataprocessor.lastExchangeState.lastMeterH.isEmpty())
        dataprocessor.lastExchangeState.lastObisList.clear();

    return hashTmpData;
#else
    Q_UNUSED(threeHash);
    return QVariantHash();
#endif
}

//------------------------------------------------------------------------------------------------------

QVariantHash DlmsItron::how2logout(const QVariantHash &hashConstData, const QVariantHash &hashTmpData)
{
    Q_UNUSED(hashConstData);
    Q_UNUSED(hashTmpData);
    return QVariantHash();
}

//------------------------------------------------------------------------------------------------------

QVariantHash DlmsItron::getDoNotSleep(const quint8 &minutes)
{
    Q_UNUSED(minutes);
    return QVariantHash();
}

//------------------------------------------------------------------------------------------------------

QVariantHash DlmsItron::getGoToSleep(const quint8 &seconds)
{
    Q_UNUSED(seconds);
    return QVariantHash();
}

//------------------------------------------------------------------------------------------------------

QVariantList DlmsItron::getDefaultVirtualMetersSchema()
{
    return QVariantList();
}

//------------------------------------------------------------------------------------------------------
