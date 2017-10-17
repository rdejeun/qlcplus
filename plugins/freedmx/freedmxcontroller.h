/*
  Q Light Controller Plus
  freedmxcontroller.h

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

#ifndef FREEDMXCONTROLLER_H
#define FREEDMXCONTROLLER_H

#if defined(ANDROID)
#include <QNetworkInterface>
#include <QHostAddress>
#include <QUdpSocket>
#include <QScopedPointer>
#include <QSharedPointer>
#else
#include <QtNetwork>
#endif
#include <QMutex>
#include <QTimer>

/** Number of channels contained within a DMX frame */
#define FRAME_MAX_CHANNEL 512

/** Number of bytes used to define a channel value in a DMX frame */
#define FRAME_BYTES_PER_CHANNEL 3

/** Maximum size of UDP datagrams for sending the DMX frame */
#define DATAGRAM_MAX_SIZE 250

/** Information about each packet (slice) of the DMX frame */
typedef struct {
    const char* ptr;  /* Pointer to the begining of the packet   */
    qint64 size;      /* Size in bytes of the packet             */
    bool end;         /* True for the final packet of the frame  */
} PacketInfo;


class FreeDmxController : public QObject
{
    Q_OBJECT

    /*********************************************************************
     * Initialization
     *********************************************************************/
public:
    FreeDmxController(QHostAddress address,
                      quint16 port,
                      QObject *parent = 0);

    ~FreeDmxController();

    /** Update DMX channel values */
    void updateDmx(const QByteArray& data);

    /** Get the number of packets sent/received by this controller */
    quint64 getPacketSentNumber();
    quint64 getPacketReceivedNumber();


private:
    void initSocket();
    void closeSocket();
    void initDmxFrame();
    void sayHello();
    void sayGoodbye();

    /** IP address and port of the freeDMX device */
    QHostAddress m_address;
    quint16 m_port;

    /** Counter for transmitted/received packets */
    quint64 m_packetSent;
    quint64 m_packetReceived;

    /** QLC+ line to be used when emitting a signal */
    quint32 m_line;

    /** UDP socket used to send/receive FreeDmx packets */
    QUdpSocket* m_udpSocket;

    /** QLC+ universe transmitted by this controller */
    ushort m_universe;

    /** Encoded DMX output frame
     *
     *  This buffer is updated periodically on QLC+ MasterTimer clock, by
     *  calling method updateDmx(), and is used by slot sendDatagrams(),
     *  triggered by timer m_sendTimer, for transmission to the device.
     */
    char m_dmxFrame[FRAME_MAX_CHANNEL * FRAME_BYTES_PER_CHANNEL];

    /** Slice pointers and size */
    PacketInfo m_dmxPacketInfo[FRAME_MAX_CHANNEL * FRAME_BYTES_PER_CHANNEL / DATAGRAM_MAX_SIZE + 1];

    /** Mutex to handle the change of output IP address or in general
     *  variables that could be used to transmit/receive data */
    QMutex m_dataMutex;

    /** Timer to send channel data every 20ms */
    QTimer* m_sendTimer;

    /** Acknowledge datagram timer */
    QTimer* m_ackTimer;

protected slots:
    void sendDatagrams();
    void receiveDatagrams();
    void ackTimeout();

private:
    void writeDatagram(const char* data, qint64 size);

signals:

};

#endif
