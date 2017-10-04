/*
  Q Light Controller Plus
  freedmxconfigure.cpp

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

#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QDebug>

#include "freedmxconfigure.h"
#include "freedmxplugin.h"

#define KMapColumnInterface     0
#define KMapColumnUniverse      1
#define KMapColumnOutputAddress 2
#define KMapColumnOutputPort    3

#define PROP_UNIVERSE (Qt::UserRole + 0)
#define PROP_LINE (Qt::UserRole + 1)
#define PROP_TYPE (Qt::UserRole + 2)

/*****************************************************************************
 * Initialization
 *****************************************************************************/

FreeDmxConfigure::FreeDmxConfigure(FreeDmxPlugin* plugin, QWidget* parent)
        : QDialog(parent)
{
    Q_ASSERT(plugin != NULL);
    m_plugin = plugin;

    /* Setup UI controls */
    setupUi(this);

    connect(m_oscPathEdit, SIGNAL(textChanged(QString)),
            this, SLOT(slotOSCPathChanged(QString)));

    fillMappingTree();
}

FreeDmxConfigure::~FreeDmxConfigure()
{
}

void FreeDmxConfigure::fillMappingTree()
{
    QTreeWidgetItem* inputItem = NULL;
    QTreeWidgetItem* outputItem = NULL;

    QList<OSCIO> IOmap = m_plugin->getIOMapping();
    foreach(OSCIO io, IOmap)
    {
        if (io.controller == NULL)
            continue;

        FreeDmxController *controller = io.controller;
        if (controller == NULL)
            continue;

        qDebug() << "[FreeDmx] controller IP" << controller->getNetworkIP().toString() << "type:" << controller->type();
        if ((controller->type() & FreeDmxController::Input) && inputItem == NULL)
        {
            inputItem = new QTreeWidgetItem(m_uniMapTree);
            inputItem->setText(KMapColumnInterface, tr("Inputs"));
            inputItem->setExpanded(true);
        }
        if ((controller->type() & FreeDmxController::Output) && outputItem == NULL)
        {
            outputItem = new QTreeWidgetItem(m_uniMapTree);
            outputItem->setText(KMapColumnInterface, tr("Outputs"));
            outputItem->setExpanded(true);
        }
        foreach(quint32 universe, controller->universesList())
        {
            UniverseInfo *info = controller->getUniverseInfo(universe);
            QString networkIP = controller->getNetworkIP().toString();
            QString baseIP = networkIP.mid(0, networkIP.lastIndexOf(".") + 1);
            baseIP.append("1");

            if (info->type & FreeDmxController::Input)
            {
                QTreeWidgetItem *item = new QTreeWidgetItem(inputItem);
                item->setData(KMapColumnInterface, PROP_UNIVERSE, universe);
                item->setData(KMapColumnInterface, PROP_LINE, controller->line());
                item->setData(KMapColumnInterface, PROP_TYPE, FreeDmxController::Input);
                item->setText(KMapColumnInterface, networkIP);
                item->setText(KMapColumnUniverse, QString::number(universe + 1));

                QSpinBox *inSpin = new QSpinBox(this);
                inSpin->setRange(1, 65535);
                inSpin->setValue(info->inputPort);
                m_uniMapTree->setItemWidget(item, KMapColumnInputPort, inSpin);

                if (controller->getNetworkIP() == QHostAddress::LocalHost)
                {
                    // localhost (127.0.0.1) does not need configuration
                    item->setText(KMapColumnOutputAddress, info->feedbackAddress.toString());
                }
                else
                {
                    QWidget *IPwidget;
                    if (info->feedbackAddress == QHostAddress::Null)
                        IPwidget = new QLineEdit(baseIP);
                    else
                        IPwidget = new QLineEdit(info->feedbackAddress.toString());
                    m_uniMapTree->setItemWidget(item, KMapColumnOutputAddress, IPwidget);
                }

                QSpinBox *outSpin = new QSpinBox(this);
                outSpin->setRange(1, 65535);
                outSpin->setValue(info->feedbackPort);
                m_uniMapTree->setItemWidget(item, KMapColumnOutputPort, outSpin);
            }
            if (info->type & FreeDmxController::Output)
            {
                QTreeWidgetItem *item = new QTreeWidgetItem(outputItem);
                item->setData(KMapColumnInterface, PROP_UNIVERSE, universe);
                item->setData(KMapColumnInterface, PROP_LINE, controller->line());
                item->setData(KMapColumnInterface, PROP_TYPE, FreeDmxController::Output);

                item->setText(KMapColumnInterface, networkIP);
                item->setText(KMapColumnUniverse, QString::number(universe + 1));

                if (controller->getNetworkIP() == QHostAddress::LocalHost)
                {
                    // localhost (127.0.0.1) does not need configuration
                    item->setText(KMapColumnOutputAddress, info->outputAddress.toString());
                }
                else
                {
                    QWidget *IPwidget;
                    if (info->outputAddress == QHostAddress::Null)
                        IPwidget = new QLineEdit(baseIP);
                    else
                        IPwidget = new QLineEdit(info->outputAddress.toString());
                    m_uniMapTree->setItemWidget(item, KMapColumnOutputAddress, IPwidget);
                }

                QSpinBox *spin = new QSpinBox(this);
                spin->setRange(1, 65535);
                spin->setValue(info->outputPort);
                m_uniMapTree->setItemWidget(item, KMapColumnOutputPort, spin);
            }
        }
    }

    m_uniMapTree->header()->resizeSections(QHeaderView::ResizeToContents);
}

void FreeDmxConfigure::showIPAlert(QString ip)
{
    QMessageBox::critical(this, tr("Invalid IP"), tr("%1 is not a valid IP.\nPlease fix it before confirming.").arg(ip));
}

/*****************************************************************************
 * Dialog actions
 *****************************************************************************/

void FreeDmxConfigure::accept()
{
    for(int i = 0; i < m_uniMapTree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *topItem = m_uniMapTree->topLevelItem(i);
        for(int c = 0; c < topItem->childCount(); c++)
        {
            QTreeWidgetItem *item = topItem->child(c);
            if (item->data(KMapColumnInterface, PROP_UNIVERSE).isValid() == false)
                continue;

            quint32 universe = item->data(KMapColumnInterface, PROP_UNIVERSE).toUInt();
            quint32 line = item->data(KMapColumnInterface, PROP_LINE).toUInt();
            FreeDmxController::Type type = FreeDmxController::Type(item->data(KMapColumnInterface, PROP_TYPE).toInt());
            QLCIOPlugin::Capability cap = QLCIOPlugin::Input;
            if (type == FreeDmxController::Output)
                cap = QLCIOPlugin::Output;

            QSpinBox *inSpin = qobject_cast<QSpinBox*>(m_uniMapTree->itemWidget(item, KMapColumnInputPort));
            if (inSpin != NULL)
                m_plugin->setParameter(universe, line, cap, OSC_INPUTPORT, inSpin->value());

            QLineEdit *ipEdit = qobject_cast<QLineEdit*>(m_uniMapTree->itemWidget(item, KMapColumnOutputAddress));
            if (ipEdit != NULL)
            {
                QHostAddress newHostAddress(ipEdit->text());
                if (newHostAddress.isNull() && ipEdit->text().size() > 0)
                {
                    showIPAlert(ipEdit->text());
                    return;
                }

                if (type == FreeDmxController::Input)
                    m_plugin->setParameter(universe, line, QLCIOPlugin::Output, OSC_FEEDBACKIP, newHostAddress.toString());
                else
                    m_plugin->setParameter(universe, line, cap, OSC_OUTPUTIP, newHostAddress.toString());
            }

            QSpinBox *outSpin = qobject_cast<QSpinBox*>(m_uniMapTree->itemWidget(item, KMapColumnOutputPort));
            if (outSpin != NULL)
            {
                if (type == FreeDmxController::Input)
                    m_plugin->setParameter(universe, line, QLCIOPlugin::Output, OSC_FEEDBACKPORT, outSpin->value());
                else
                    m_plugin->setParameter(universe, line, cap, OSC_OUTPUTPORT, outSpin->value());
            }
        }
    }

    QDialog::accept();
}

void FreeDmxConfigure::slotOSCPathChanged(QString path)
{
    m_chNumSpin->setValue(qChecksum(path.toUtf8().data(), path.length()));
}

int FreeDmxConfigure::exec()
{
    return QDialog::exec();
}

