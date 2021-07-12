#include "dlmsitronhelper.h"


//---------------------------------------------------------------------------------------------------------

QStringList DlmsItronHelper::getSupportedEnrg(const quint8 &code, const QString &version)
{

    if(!isPollCodeSupported(code))
        return QStringList();


    const QString defEnrg = version.isEmpty() ? "A+,A-,R+,R-" : "";

    QString v;
    if(!version.isEmpty()){
        v = (version.contains("A")) ? "A+" : "A+,A-,R+,R-";
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
