/*
  Q Light Controller Plus
  freedmxcontroller.cpp

  Copyright (c) 2017 Rodolphe Dejeunes

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "freedmxcontroller.h"

#include <QMutexLocker>
#include <QStringList>
#include <QDebug>

/* Number of DMX updates per second (default=25) */
#define REFRESH_RATE 25

/**
 *  FreeDmxController (constructor)
 */
FreeDmxController::FreeDmxController(
    QHostAddress address,
    quint16 port,
    QObject *parent)
    : QObject(parent)
    , m_address(address)
    , m_port(port)
    , m_packetSent(0)
    , m_packetReceived(0)
    , m_udpSocket(NULL)
    , m_sendTimer(NULL)
    , m_ackTimer(NULL)
{
    /** Create DMX frame structure */
    initDmxFrame();

    /** Create UDP socket */
    initSocket();

}

FreeDmxController::~FreeDmxController()
{
    qDebug() << Q_FUNC_INFO;

    closeSocket();
}


/**
 *  initSocket - Create the controller UDP socket
 */
void FreeDmxController::initSocket()
{
/*    QHostAddress bindAddress("192.168.4.255");
    quint16 bindPort = 12345;*/

    m_udpSocket = new QUdpSocket(this);

    /**
     * For the moment, UDP socket reception is not implemented because
     * unreliable reception cause a blocking of transmission.
     */
     /*
    if ( m_udpSocket->bind(bindAddress, bindPort) == false ) {
        qWarning() << "[FreeDmxController] Cannot bind "
                   << bindAddress.toString() << ":" << bindPort;
        qWarning() << "[FreeDmxController] Errno: "
                   << m_udpSocket->error();
        qWarning() << "[FreeDmxController] Errmgs: "
                   << m_udpSocket->errorString();
    }

    qDebug() << "[FreeDmxController::initSocket] Listen to "
             << QHostAddress::LocalHost << ":12345";

    connect(m_udpSocket, SIGNAL(readyRead()),
            this, SLOT(receiveDatagrams()));
    */

    /** Be polite! Say hello to the freeDmx device */
    writeDatagram("\xE5\x39\x60\x00", 4);

    /** Setup timer for sending channel data periodically */
    m_sendTimer = new QTimer(this);
    connect(m_sendTimer, SIGNAL(timeout()),
            this, SLOT(sendDatagrams()));
    m_sendTimer->start(1000 / REFRESH_RATE);

}

void FreeDmxController::closeSocket()
{
    QMutexLocker locker(&m_dataMutex);

    if ( m_udpSocket == NULL ) return;

    if ( m_sendTimer != NULL ) {
        m_sendTimer->stop();
        disconnect(m_sendTimer, SIGNAL(timeout()),
                   this, SLOT(sendDatagrams()));
        delete m_sendTimer;
        m_sendTimer = NULL;
    }

    /** Bye bye */
    writeDatagram("\xAC\x00", 2);

    m_udpSocket->close();
    m_udpSocket = NULL;

    qDebug() << "[freeDmxController] UDP socket closed";

    if ( m_ackTimer != NULL ) {
        m_ackTimer->stop();
        delete m_ackTimer;
        m_ackTimer = NULL;
        qDebug() << "[freeDmxController] ackTimer stopped";
    }
}


/**
 *  initDmxFrame - Initialize the DMX frame structure
 */
void FreeDmxController::initDmxFrame()
{
    /** Fill the frame with channel default values (0) */
    int pos = 0;
    quint8 code = 0xC0;
    for (int channel = 0; channel < FRAME_MAX_CHANNEL; channel++) {
        /* Select the command code for the channel */
        if ( channel == 128 ) code = 0xC2; else
        if ( channel == 256 ) code = 0xC4; else
        if ( channel == 384 ) code = 0xC6; else
        if ( channel == 511 ) code = 0x86;
        /* Add the channel 3-byte word to frame */
        m_dmxFrame[pos++] = code;
        m_dmxFrame[pos++] = channel & 0x7F;
        m_dmxFrame[pos++] = 0;
    }

    /** Cut the whole frame into slices for sending as UDP datagrams */
    int size = FRAME_MAX_CHANNEL * FRAME_BYTES_PER_CHANNEL;
    for (int slice=0; size > 0; slice++) {
        m_dmxPacketInfo[slice].ptr = &m_dmxFrame[slice*DATAGRAM_MAX_SIZE];
        if ( size > DATAGRAM_MAX_SIZE ) {
            m_dmxPacketInfo[slice].size = DATAGRAM_MAX_SIZE;
            m_dmxPacketInfo[slice].end = false;
            size = size - DATAGRAM_MAX_SIZE;
        } else {
            m_dmxPacketInfo[slice].size = size;
            m_dmxPacketInfo[slice].end = true;
            size = 0;
        }
    }

    /** Debug output */
    /*
    QByteArray qba(m_dmxFrame, FRAME_MAX_CHANNEL * FRAME_BYTES_PER_CHANNEL);
    qDebug() << "[FreeDmxController] DMX frame = " << qba.toHex();

    for (int n=0; n<=FRAME_MAX_CHANNEL * FRAME_BYTES_PER_CHANNEL / DATAGRAM_MAX_SIZE; n++) {
        PacketInfo packet = m_dmxPacketInfo[n];
        QByteArray qba2(packet.ptr, packet.size);
        qDebug() << "[FreeDmxController]" << n << qba2.toHex() << packet.size << "bytes";
        if (packet.end) break;
    }
    */
}


/**
 *  updateDmxFrame - Update the main DMX frame with channel data
 *
 *  It is called at the rate of the QLC+ MasterTimer clock
 *  and it should never block for more than 20ms.
 */
void FreeDmxController::updateDmx(const QByteArray &data)
{
    QMutexLocker locker(&m_dataMutex);

    /** Check the number of channels */
    int chCount = data.length();
    if ( chCount > FRAME_MAX_CHANNEL )
        qWarning() << "[freeDmxController] updateDmx: Unexpected number of channels: "
                   << chCount;

    /** Update all channel values in frame */
    char *ptrDmxFrame = m_dmxFrame;
    for ( int channel = 0; channel < chCount; channel++ ) {
        char dmxValue = data.at(channel);
        /** First byte = C0/C2/C4/C6 + 1 if DMX value is > 127 */
        *ptrDmxFrame &= 0xFE;
        *ptrDmxFrame |= ( dmxValue >> 7 ) & 0x01;
        ptrDmxFrame++;
        /** Second byte if unchanged */
        ptrDmxFrame++;
        /** Third byte is the DMX value ( - 128 if > 127) */
        *ptrDmxFrame = dmxValue & 0x7F;
        ptrDmxFrame++;
    }
}



quint64 FreeDmxController::getPacketSentNumber()
{
    return m_packetSent;
}

quint64 FreeDmxController::getPacketReceivedNumber()
{
    return m_packetReceived;
}


/**
 *  receviceDatagrams - Incoming datagram received
 *
 *  This function is called whenever datagrams arrive. It only gets every
 *  incoming datagram and triggers a timer to check that the freeDMX device
 *  is still alive.
 */
void FreeDmxController::receiveDatagrams()
{
    QHostAddress senderAddress;
    QByteArray datagram;
    qint64 size;

    /** Stop the timeout if created */
    if (m_ackTimer != NULL) {
        m_ackTimer->stop();
        delete m_ackTimer;
        m_ackTimer = NULL;
    }

    /** Receive pending datagrams */
    while ( m_udpSocket->hasPendingDatagrams() ) {
        size = m_udpSocket->pendingDatagramSize();
        datagram.resize(size);
        m_udpSocket->readDatagram(datagram.data(), size, &senderAddress);
        m_packetReceived++;
    }

    qDebug() << "[freeDmxController] Received" << size
             << "bytes from " << senderAddress.toString();

    /** Fires a timeout if nothing has been received for 2 seconds */
    m_ackTimer = new QTimer(this);
    m_ackTimer->singleShot(2000, this, SLOT(slotAckTimeOut()));
}


/**
 *  sendDatagrams - Send UDP packets
 *
 *  Nota: This method is called every 20ms and must not contain any heavy
 *        time-consuming processing.
 */
void FreeDmxController::sendDatagrams()
{
    QMutexLocker locker(&m_dataMutex);

    /** Return right now if UDP socket is not active */
    if ( m_udpSocket == NULL ) return;

    /** Transmit the DMX frame in pre-defined small packets */
    for ( int n = 0; n <= ( FRAME_MAX_CHANNEL * FRAME_BYTES_PER_CHANNEL / DATAGRAM_MAX_SIZE ); n++ ) {
        PacketInfo packet = m_dmxPacketInfo[n];
        writeDatagram((const char *)packet.ptr, (qint64)packet.size);
        if ( packet.end ) break;
    }

    /** Debug output */
    /*
    for (int n=0; n<=FRAME_MAX_CHANNEL * FRAME_BYTES_PER_CHANNEL / DATAGRAM_MAX_SIZE; n++) {
        PacketInfo packet = m_dmxPacketInfo[n];
        QByteArray qba2(packet.ptr, packet.size);
        qDebug() << "[FreeDmxController]" << n << qba2.toHex() << packet.size << "bytes";
        if (packet.end) break;
    }
    */
}

void FreeDmxController::ackTimeout()
{
    qWarning() << "[freeDmxControlled] Acknowledge timeout!";
}




void FreeDmxController::writeDatagram(const char* data, qint64 size) {

    if ( m_udpSocket == NULL ) return;

    if ( m_udpSocket->writeDatagram(data, size, m_address, m_port) < size ) {
        qWarning() << "[freeDmxController] writeDatagram failed";
        qWarning() << "[freeDmxController] Errno: " << m_udpSocket->error();
        qWarning() << "[freeDmxController] Errmgs: " << m_udpSocket->errorString();
    }
    else m_packetSent++;
}
