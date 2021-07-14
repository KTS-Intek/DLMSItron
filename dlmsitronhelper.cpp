#include "dlmsitronhelper.h"


//---------------------------------------------------------------------------------------------------------

QStringList DlmsItronHelper::getSupportedEnrg(const quint8 &code, const QString &version)
{

    if(!isPollCodeSupported(code))
        return QStringList();


    if(code == POLL_CODE_READ_VOLTAGE){
        return QString("UA,UB,UC,IA,IB,IC,PA,PB,PC,QA,QB,QC,cos_fA,cos_fB,cos_fC,F").split(',');
    }


    const QString defEnrg = version.isEmpty() ? "A+,A-,R+,R-" : "";

    QString v;
    if(!version.isEmpty()){
        v = "A+,A-,R+,R-";
    }


    return v.isEmpty() ? defEnrg.split(",") : v.split(",");

}

//---------------------------------------------------------------------------------------------------------

bool DlmsItronHelper::getVersion(const QVariantHash &hashConstData, const QVariantHash &hashTmpData)
{
    return !(hashConstData.value("vrsn").toString().contains("E") || hashTmpData.value("vrsn").toString().contains("E"));

}

//---------------------------------------------------------------------------------------------------------

bool DlmsItronHelper::getVersionCache(const QVariantHash &hashConstData, QVariantHash &hashTmpData)
{
    if(hashConstData.value("vrsn").toString().contains("E") && !hashTmpData.value("vrsn").toString().contains("E"))
        hashTmpData.insert("vrsn", hashConstData.value("vrsn").toString());
    else if(!hashTmpData.value("vrsn").toString().contains("E"))
        hashTmpData.remove("vrsn");
    return !(hashConstData.value("vrsn").toString().contains("E") || hashTmpData.value("vrsn").toString().contains("E"));
}

//---------------------------------------------------------------------------------------------------------

bool DlmsItronHelper::isPollCodeSupported(const quint16 &pollCode, QVariantHash &hashTmpData)
{
    if(hashTmpData.value("plgChecked").toBool())
        return true;

    const bool isSupp = isPollCodeSupported(pollCode);

    hashTmpData.insert("sprtdVls", isSupp);
    hashTmpData.insert("plgChecked", true);
    return isSupp;
}

//---------------------------------------------------------------------------------------------------------

bool DlmsItronHelper::isPollCodeSupported(const quint16 &pollCode)
{
    return (pollCode == POLL_CODE_READ_VOLTAGE || pollCode == POLL_CODE_READ_TOTAL);//
//            || pollCode == POLL_CODE_READ_POWER);

}

//---------------------------------------------------------------------------------------------------------

int DlmsItronHelper::calculateEnrgIndx(qint16 currEnrg, const quint8 &pollCode, const QStringList &listEnrg, QString version, const bool &verboseMode)
{
    if(version.isEmpty())
        version = " ";//G3B F48";
    const QStringList spprtdListEnrg = getSupportedEnrg(pollCode, version);

    // int energyIndex = (-1);//індекс енергії планіну до індексу опитування
    if(currEnrg < 0)
        currEnrg = 0;

    if(verboseMode) qDebug() << listEnrg << currEnrg << spprtdListEnrg ;
    for(qint16 iMax = listEnrg.size(); currEnrg < iMax; currEnrg++){

        if(spprtdListEnrg.contains(listEnrg.at(currEnrg))){
            if(verboseMode) qDebug() << "calculateEnrgIndx " << spprtdListEnrg.indexOf(listEnrg.at(currEnrg));
            return spprtdListEnrg.indexOf(listEnrg.at(currEnrg));
        }
    }
    if(verboseMode) qDebug() << "DLMS decode energyIndex power -1" << currEnrg;
    return (-2);
}

//---------------------------------------------------------------------------------------------------------

QByteArray DlmsItronHelper::getAarq(const QVariantHash &hashConstData, const bool &lastMeterIsShortDlms, const QByteArray &defPassword)
{

//    A0 53 - frame type and len
//    00 02
//    34 E5
//    03
//    10 - frame type I frame
//    A3 06 - HCS
//    E6 E6 00 - LLC bytes
//    60 42 - AARQ tag and len
//    A1 09 06 07 60 85 74 05 08 01 01 //Application context name tag, length and encoded value нах надо не зрозуміло, але таке саме як і в описі в неті
//    A6 0A 04 08 45 47 4D 30 32 34 34 32 // коротке мережеве назвисько ????
//    8A 02 07 80
//    8B 07 60 85 74 05 08 02 01
//    AC 0A 80 08 30 30 30 30 30 30 30 32

//    BE 10 - tag and length for AARQ user field
//        04 0E - encoding the choice for user-information (OCTET STRING, universal) and length
//        01 - tag for xDLMS-Initiate request
//        00 - usage field for dedicated-key component – not used
//        00 - usage field for the response allowed component – not used
//        00 - usage field of the proposed-quality-of-service component – not used
//        06 - proposed dlms version number 6
//        5F 1F -  tag for conformance block
//        04 - length of the conformance block
//        00 - encoding the number of unused bits in the bit string
//        00 7E 1F - conformance block
//        00 00 - client-max-receive-pdu-size

//    33 1B - FCS


//    відповідь авторизація
//    A0 3A  - frame type and len
//    03
//    00 02
//    34 E5
//    30 - frame type XX???
//    1A B7 - HCS
//    E6 E7 00 - LLC bytes
//    61 29 - AARE tag and length
//    A1 09 06 07 60 85 74 05 08 01 01
//    A2 03 02 01 00
//    A3 05 A1 03 02 01 00
//    BE 10       -   tag and length for AARE user-field
//        04 0E - encoding the choice for user-information (OCTET STRING, universal) and length
//        08 - tag for xDLMS-Initate.response
//        00 - usage field of the negotiated-quality-of-service component
//        06 - negotiated dlms version number 6
//        5F 1F - tag for conformance block
//        04 -  length of the conformance block
//        00 - encoding the number of unused bits in the bit string
//        00 1A 1D - negotiated conformance block
//        00 7D - server-max-receive-pdu-size
//        00 07 - VAA name (0x0007 for LN referencing)
//    07 BB - FCS

    const QByteArray p = getValidPassword(hashConstData, true, defPassword);//password 8 byte !always

    const QByteArray a1Message = lastMeterIsShortDlms ?
                "A1 09 06 07 60 85 74 05 08 01 02   " : "A1 09 06 07 60 85 74 05 08 01 01"; //A6 0A 04 08 45 47 4D 30 38 35 34 39
    const QByteArray conformBlock = lastMeterIsShortDlms ? "1C 13 20" : "62 FE DF";

    QByteArray passwordline = p.isEmpty() ? "00" : p; //if the password is empty send 0x0
    passwordline.prepend(QByteArray::number(QByteArray::fromHex(passwordline).length(), 16).rightJustified(2, '0'));
//    passwordline.prepend("0A 80"); //must be before



    QByteArray aarq =
                a1Message +
                "8A 02 07 80"
                "8B 07 60 85 74 05 08 02 01" // "8B 07 60 85 74 05 08 02 01"     //WTF?
                "AC 0A 80 " + passwordline +  // 0A 80 " + QByteArray::number(p.length() / 2, 16).rightJustified(2, '0') + p//password 8 byte !always
                + "BE 10"   // tag and length for AARQ user field
                  "04 0E"   // encoding the choice for user-information (OCTET STRING, universal) and length
                  "01"      // tag for xDLMS-Initiate request
                  "00"      // usage field for dedicated-key component – not used
                  "00"      // usage field for the response allowed component – not used
                  "00"      // usage field of the proposed-quality-of-service component – not used
                  "06"      // proposed dlms version number 6
                  "5F 1F"   // tag for conformance block
                  "04"      // length of the conformance block
                  "00"      // encoding the number of unused bits in the bit string
                  + conformBlock + // conformance block
                  "FF FF" ; // client-max-receive-pdu-size



    aarq.prepend("60 " + QByteArray::number(QByteArray::fromHex(aarq).length(), 16).rightJustified(2, '0'));// AARQ tag and len
    aarq.prepend("E6 E6 00");// LLC bytes
    return aarq;


}

//---------------------------------------------------------------------------------------------------------

void DlmsItronHelper::addObis4readDtSnVrsnDst(quint8 &itronStep, ObisList &obislist, AttributeList &attrList, const bool &addDst, const bool &lastIsShortDlms)
{


    if(lastIsShortDlms){
//        addObis4readDtSnVrsnDstSn(obislist, attrList, addDst, getVersion);
        return;
    }


//    if(!getVersion && (itronStep == 1)){
//        itronStep = 2;
//    }

    if(!addDst && itronStep > 1){
        itronStep = 0xFF;//get out
        return;
    }

    switch(itronStep){
    case 0:{
        obislist.prepend(CMD_GSET_METER_NUMBER);
        attrList.prepend(2);  //"00 01 00 00 2A 00 00 FF 02 00" //sn
        break;}

//    case 1:{
//        obislist.prepend(CMD_GET_FIRMWARE_VERSION);
//        attrList.prepend(2);  //"00 01 00 00 60 01 01 FF 02 00" //meter type  G1B or G3B
//        break;}

//    case 1:{
//        obislist.prepend(CMD_GET_MODIFICATION);
//        attrList.prepend(2);  //"00 01 00 00 60 01 02 FF" //220.F18.P2.C300.V1.R1L5  meter version
//        break;}

    case 1:{
        obislist.prepend(CMD_GSET_DATETIME);
        attrList.prepend(2);  //"00 08 00 00 01 00 00 FF 02 00"
        break;}

    case 2:{
        obislist.prepend(CMD_GSET_DATETIME);
        attrList.prepend(5);//dst begin
        break;}

    case 3:{
        obislist.prepend(CMD_GSET_DATETIME);
        attrList.prepend(6);//dst end
        break;}

    case 4:{
        obislist.prepend(CMD_GSET_DATETIME);
        attrList.prepend(7);//dst deviation
        break;}

    case 5:{
        obislist.prepend(CMD_GSET_DATETIME);
        attrList.prepend(8);//dst enabled

        break;}
    }

}

//---------------------------------------------------------------------------------------------------------

QByteArray DlmsItronHelper::addObis4writeDt(ObisList &lastObisList, const bool &lastMeterIsShortDlms)
{
    return addObis4writeDtExt(lastObisList, CMD_GSET_DATETIME, 0, lastMeterIsShortDlms, "FF FF 4C 80" );
    //I don't know why it must be FF FF 4C 80, but if I use FF FF FF FF the time is incorrect, the difference is 3600 seconds

}

//---------------------------------------------------------------------------------------------------------

QVariantHash DlmsItronHelper::getObisCodesTotal4thisMeter(const QVariantHash &hashConstData, const QString &version)
{
    int trff = hashConstData.value("trff", DEF_TARIFF_NUMB).toInt();
    if(trff < 1 || trff > TRFF_MAX_TARIFFNUMB)
        trff = DEF_TARIFF_NUMB;
    const QStringList listEnrg = hashConstData.value("listEnrg").toStringList();

    const QStringList listPlgEnrg = getSupportedEnrg(POLL_CODE_READ_TOTAL, version);

    const QStringList lAll = QString("A+,A-,R+,R-").split(",");

    QVariantHash outh;

    for(int i = 0, imax = lAll.size(); i < imax; i++){
        const QString enrgkey = lAll.at(i);
        if(!listPlgEnrg.contains(enrgkey) || !listEnrg.contains(enrgkey))
            continue;
         const quint64 obis = getObis4energyIndexTotal(i);
        if(obis < 1)
            continue;

         quint64 obisTarAdd = 0x0100000000;//01 01 08 00 FF
        for(int t = 0; t <= trff && t < TRFF_MAX_TARIFFNUMB; t++){
            const QString enrgkeyFull = QString("T%1_%2").arg(t).arg(enrgkey);
            outh.insert(enrgkeyFull, obis + obisTarAdd);
            obisTarAdd += 0x100;
        }
    }
    return outh;



}

//---------------------------------------------------------------------------------------------------------

QVariantHash DlmsItronHelper::getObisCodesVoltage4thisMeter(const QVariantHash &hashConstData, const QString &version)
{
    Q_UNUSED(hashConstData);
    Q_UNUSED(version);
//    const QStringList lAll = QString("UA,UB,UC,IA,IB,IC,PA,PB,PC,QA,QB,QC,cos_fA,cos_fB,cos_fC,F").split(',');


    const quint64 obisTarAdd = 0x0100000000;//01 01 08 00 FF
    QVariantHash outh;
    outh.insert("F", CMD_GET_INSTANT_SUMM_FREQUENCY + obisTarAdd);


    outh.insert("DLMS_QA"   , CMD_GET_INSTANT_L1_Q_PLUS     + obisTarAdd);
    outh.insert("QA"   , CMD_GET_INSTANT_L1_Q_MINUS    + obisTarAdd);
    outh.insert("IA"    , CMD_GET_INSTANT_L1_I          + obisTarAdd);
    outh.insert("UA"    , CMD_GET_INSTANT_L1_U          + obisTarAdd);
    outh.insert("cos_fA", CMD_GET_INSTANT_L1_COSF       + obisTarAdd);
    outh.insert("PA"    , CMD_GET_INSTANT_L1_P_PLUS      + obisTarAdd);
    outh.insert("DLMS_PA", CMD_GET_INSTANT_L1_P_MINUS     + obisTarAdd);


    outh.insert("DLMS_QB"   , CMD_GET_INSTANT_L2_Q_PLUS     + obisTarAdd);
    outh.insert("QB"   , CMD_GET_INSTANT_L2_Q_MINUS    + obisTarAdd);
    outh.insert("IB"    , CMD_GET_INSTANT_L2_I          + obisTarAdd);
    outh.insert("UB"    , CMD_GET_INSTANT_L2_U          + obisTarAdd);
    outh.insert("cos_fB", CMD_GET_INSTANT_L2_COSF       + obisTarAdd);
    outh.insert("PB"    , CMD_GET_INSTANT_L2_P_PLUS          + obisTarAdd);
    outh.insert("DLMS_PB", CMD_GET_INSTANT_L2_P_MINUS     + obisTarAdd);


    outh.insert("DLMS_QC"   , CMD_GET_INSTANT_L3_Q_PLUS     + obisTarAdd);
    outh.insert("QC"   , CMD_GET_INSTANT_L3_Q_MINUS    + obisTarAdd);
    outh.insert("IC"    , CMD_GET_INSTANT_L3_I          + obisTarAdd);
    outh.insert("UC"    , CMD_GET_INSTANT_L3_U          + obisTarAdd);
    outh.insert("cos_fC", CMD_GET_INSTANT_L3_COSF       + obisTarAdd);
    outh.insert("PC"    , CMD_GET_INSTANT_L3_P_PLUS          + obisTarAdd);
    outh.insert("DLMS_PC", CMD_GET_INSTANT_L3_P_MINUS     + obisTarAdd);

    return outh;

//#define CMD_GET_INSTANT_SUMM_FREQUENCY          0x01000E0700FF
//#define CMD_GET_INSTANT_L1_Q_PLUS               0x0100170700FF
//#define CMD_GET_INSTANT_L1_Q_MINUS              0x0100180700FF
//#define CMD_GET_INSTANT_L1_I                    0x01001F0700FF
//#define CMD_GET_INSTANT_L1_U                    0x0100200700FF
//#define CMD_GET_INSTANT_L1_COSF                 0x0100210700FF
//#define CMD_GET_INSTANT_L1_P                    0x0100240700FF

//#define CMD_GET_INSTANT_L2_Q_PLUS               0x01002B0700FF
//#define CMD_GET_INSTANT_L2_Q_MINUS              0x01002C0700FF
//#define CMD_GET_INSTANT_L2_I                    0x0100330700FF
//#define CMD_GET_INSTANT_L2_U                    0x0100340700FF
//#define CMD_GET_INSTANT_L2_COSF                 0x0100350700FF
//#define CMD_GET_INSTANT_L2_P                    0x0100380700FF

//#define CMD_GET_INSTANT_L3_Q_PLUS               0x01003F0700FF
//#define CMD_GET_INSTANT_L3_Q_MINUS              0x0100400700FF
//#define CMD_GET_INSTANT_L3_I                    0x0100470700FF
//#define CMD_GET_INSTANT_L3_U                    0x0100480700FF
//#define CMD_GET_INSTANT_L3_COSF                 0x0100490700FF
//#define CMD_GET_INSTANT_L3_P                    0x01004C0700FF

}

//---------------------------------------------------------------------------------------------------------

quint64 DlmsItronHelper::getObis4energyIndexTotal(const int &indx)
{
     quint64 obis = 0;
    switch(indx){
    case 0: obis = CMD_GET_ACTIVE_IMPORT_SUMM; break; // A+
    case 1: obis = CMD_GET_ACTIVE_EXPORT_SUMM; break; //A-
    case 2: obis = CMD_GET_REACTIVE_RIMPORT_SUMM; break; //R+
    case 3: obis = CMD_GET_REACTIVE_REXPORT_SUMM; break; //R-
    }
    return obis;
}

//---------------------------------------------------------------------------------------------------------

void DlmsItronHelper::addTariffAttribute2obisAndAttributeList(ObisList &obislist, AttributeList &attrList, const quint64 &obis, const bool &ask4scallerUnit, const bool &lastIsShortDlms)
{
    if(lastIsShortDlms){
//        addTariffAttribute2obisAndAttributeListSN(obislist, attrList, obis);
        return;
    }

    obislist.append(obis);
    attrList.append(ask4scallerUnit ? 3 : 2);//scaller_unit

//    obislist.append(obis);
//    attrList.append(2);//value

}

qreal DlmsItronHelper::getScaller4obisCode(const quint64 &obisCode)
{
    const quint64 obisTarAdd = 0x0100000000;//01 01 08 00 FF


    qreal scaler = 1.0;

    switch((obisCode - obisTarAdd)){

    case CMD_GET_INSTANT_SUMM_FREQUENCY :                ; break;

    case CMD_GET_INSTANT_L1_Q_PLUS      : scaler =  0.001; break;
    case CMD_GET_INSTANT_L1_Q_MINUS     : scaler = -0.001; break;
    case CMD_GET_INSTANT_L1_I           :                ; break;
    case CMD_GET_INSTANT_L1_U           :                ; break;
    case CMD_GET_INSTANT_L1_COSF        :                ; break;
    case CMD_GET_INSTANT_L1_P           : scaler =  0.001; break;
    case CMD_GET_INSTANT_L1_P_PLUS      : scaler =  0.001; break;
    case CMD_GET_INSTANT_L1_P_MINUS     : scaler = -0.001; break;

    case CMD_GET_INSTANT_L2_Q_PLUS      : scaler =  0.001; break;
    case CMD_GET_INSTANT_L2_Q_MINUS     : scaler = -0.001; break;
    case CMD_GET_INSTANT_L2_I           :                ; break;
    case CMD_GET_INSTANT_L2_U           :                ; break;
    case CMD_GET_INSTANT_L2_COSF        :                ; break;
    case CMD_GET_INSTANT_L2_P           : scaler =  0.001; break;
    case CMD_GET_INSTANT_L2_P_PLUS      : scaler =  0.001; break;
    case CMD_GET_INSTANT_L2_P_MINUS     : scaler = -0.001; break;

    case CMD_GET_INSTANT_L3_Q_PLUS      : scaler =  0.001; break;
    case CMD_GET_INSTANT_L3_Q_MINUS     : scaler = -0.001; break;
    case CMD_GET_INSTANT_L3_I           :                ; break;
    case CMD_GET_INSTANT_L3_U           :                ; break;
    case CMD_GET_INSTANT_L3_COSF        :                ; break;
    case CMD_GET_INSTANT_L3_P           : scaler =  0.001; break;
    case CMD_GET_INSTANT_L3_P_PLUS      : scaler =  0.001; break;
    case CMD_GET_INSTANT_L3_P_MINUS     : scaler = -0.001; break;



    case CMD_GET_INSTANT_SUMM_I         :                ; break;
    case CMD_GET_INSTANT_SUMM_U         :                ; break;
    case CMD_GET_INSTANT_SUMM_COSF      :                ; break;
    case CMD_GET_INSTANT_SUMM_P         : scaler =  0.001; break;

    }

    return scaler;
}

//void DlmsItronHelper::addTariffAttribute2obisAndAttributeListSN(ObisList &obislist, AttributeList &attrList, const quint64 &obis, const bool &ask4scallerUnit)
//{

//}


//---------------------------------------------------------------------------------------------------------
