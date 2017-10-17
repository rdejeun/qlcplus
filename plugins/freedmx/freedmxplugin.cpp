/*
  Q Light Controller Plus
  freedmxplugin.cpp

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

#include "freedmxplugin.h"

/**
 * Default AP configuration
 */
#define DEFAULT_AP_ADDRESS "192.168.4.1"
#define DEFAULT_AP_PORT 10100



/*****************************************************************************
 * Initialization
 *****************************************************************************/

 /**
  * De-initialize the plugin. This is the last thing that is called
  * for the plugin so make sure nothing is lingering in the twilight
  * after this call.
  */
FreeDmxPlugin::~FreeDmxPlugin()
{
    /** Clean up the plugin resources here.
     *  E.g. running threads, allocated memory, etc..
     */
    foreach (FreeDmxInterface line, m_OutputLines)
    {
        if ( line.controller != NULL ) {
            delete line.controller;
            line.controller = NULL;
        }
    }

}


/**
 * Initialize the plugin. Since plugins cannot have a user-defined
 * constructor, any initialization prior to opening any HW must be
 * done through this second-stage initialization method. This method is
 * called exactly once after each plugin has been successfully loaded
 * and before calling other plugin interface methods.
 */
void FreeDmxPlugin::init()
{
    /**
     * Add a unique interface
     */
    if ( (quint32)m_OutputLines.count() == 0 ) {
        FreeDmxInterface tmp;
        tmp.universe = 0;
        tmp.address = QHostAddress(DEFAULT_AP_ADDRESS);
        tmp.port = DEFAULT_AP_PORT;
        tmp.controller = NULL;
        m_OutputLines.append(tmp);

        qDebug() << "[FreeDmxPlugin::init] Create output line for "
                 << tmp.address.toString() << ":" << tmp.port;
    }
}


/**
 * Get the plugin's name. Plugin's name must not change over time.
 */
QString FreeDmxPlugin::name()
{
    return QString("freeDMX_AP");
}


/**
 * Get plugin capabilities as an OR'ed bitmask, from following values:
 * Output, Input, Feedback, Infinite
 *
 * See the QLCIOPlugin Capability enum for usage
 */
int FreeDmxPlugin::capabilities() const
{
    return QLCIOPlugin::Output;
}


/**
 * Get the plugin's description info.
 *
 * This is a pure virtual method that must be implemented by all plugins.
 */
QString FreeDmxPlugin::pluginInfo()
{
    /** Return a description of the purpose of this plugin
     *  in HTML format */
    QString str;

    str += QString("<html>");
    str += QString("<head>");
    str += QString("<title>%1</title>").arg(name());
    str += QString("</head>");
    str += QString("<body>");
    str += QString("<h3>%1</h3>").arg(name());
    str += QString("<p>");

    /**
     * Interface picture as base-64 inline GIF image
     */
    str += QString("<img width=210 height=130 border=0 align=\"right\" src=\"data:image/gif;base64,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\">");

    /**
     * Plugin description
     */
    str += tr("This plugin provides output for Eurolite® freeDMX AP Wi-Fi Interface (No. 51860130).");
    str += QString("</p><p>");
    str += tr("The freeDMX AP interface receives control signals for 512 channels over the network (WLAN) and transfers it to the DMX chain. It provides either an Access Point mode or an Infrastructure mode.");
    str += QString("</p>");

    /**
     * Beta version warning
     */
    str += QString("<p>");
    str += tr("Warning: this is a beta version of the plugin working only for device with default factory settings (Access Point mode).");
    str += QString("</p>");

    return str;
}


/*****************************************************************************
 * Outputs
 *****************************************************************************/

 /**
  * Provide a list of output line names. The names must be always in the
  * same order i.e. the first name is the name of output line number 0,
  * the next one is output line number 1, etc..
  *
  * @return A list of available output names
  */
 QStringList FreeDmxPlugin::outputs()
 {
     QStringList list;
     int j = 0;
     /*init();*/
     foreach (FreeDmxInterface line, m_OutputLines)
     {
         list << QString("%1: %2:%3").arg(j + 1).arg(line.address.toString()).arg(line.port);
         j++;
     }
     return list;
 }


 /**
  * Open the specified output line so that the plugin can start sending
  * DMX data through that line.
  *
  * @param output The output line to open
  * @param universe the QLC+ universe index this line is going to be patched to
  */
bool FreeDmxPlugin::openOutput(quint32 output, quint32 universe)
{
    qDebug() << "[FreeDmxPlugin::openOutput] Open output line #"
             << output << " for universe " << universe;

    /** Check for output index validity and, in case, return false */

    /** If the controller doesn't exist, create it */
    if ( m_OutputLines[output].controller == NULL ) {
        /**
         * Create the controller with the default AP address and port.
         *
         * Nota: at the moment, only one controller is associated to the
         *       output line, which is a temporary behaviour until the
         *       configuration code of the plugin is developed.
         */
        FreeDmxController *controller = new FreeDmxController(
            m_OutputLines.at(output).address,
            m_OutputLines.at(output).port,
            (QObject*)this);

        m_OutputLines[output].universe = universe;
        m_OutputLines[output].controller = controller;
    }

    addToMap(universe, output, Output);

    return true;
}


void FreeDmxPlugin::closeOutput(quint32 output, quint32 universe)
{
    qDebug() << "[FreeDmxPlugin::closeOutput] Close output line #"
             << output << " on universe " << universe;

    /** Check for output index validity and, in case, return */
    if (output >= (quint32)m_OutputLines.length())
        return;

    removeFromMap(output, universe, Output);

    /** Do the plugin specific operations to
     *  close the requested output line */
    FreeDmxController *controller = m_OutputLines.at(output).controller;
    if ( controller != NULL )
    {
        delete controller;
        m_OutputLines[output].controller = NULL;
    }
}


QString FreeDmxPlugin::outputInfo(quint32 output)
{
    /**
     * Provide an informational text regarding the specified output line.
     * This text is in HTML format and it is shown to the user.
     */
    QString str;

    if ( output != QLCIOPlugin::invalidLine() ) {
        str += QString("<h3>%1</h3>").arg(outputs()[output]);
        str += QString("<ul>");
        str += QString("<li>Universe: <b>%1</b></li>").arg(m_OutputLines[output].universe);
        str += QString("<li>Address: <b>%1</b></li>").arg(m_OutputLines[output].address.toString());
        str += QString("<li>Port: <b>%1</b></li>").arg(m_OutputLines[output].port);
        if ( m_OutputLines[output].controller != NULL )
            str += QString("<li>Sent/received packets: <b>%1</b> / <b>%2</b></li>").arg(m_OutputLines[output].controller->getPacketSentNumber()).arg(m_OutputLines[output].controller->getPacketReceivedNumber());
        str += QString("</ul>");
    }
    return str;
}


void FreeDmxPlugin::writeUniverse(quint32 universe, quint32 output, const QByteArray &data)
{
    Q_UNUSED(universe)

    /**
	 * This method is very important as it is the implementation of the plugin
     * data transmission. It is called at the rate of the QLC+ MasterTimer clock
     * and it should never block for more than 20ms.
     * If this plugin cannot predict the duration of a universe transmission,
     * it is then safer to exchange data with a thread, running indipendently
     * and not risking to hang QLC+
     */

    /** Check for output index validity and, in case, return. */
    if ( output >= (quint32)m_OutputLines.count() )
		return;

	/** Send DMX channel data to the controller */
	FreeDmxController *controller = m_OutputLines.at(output).controller;
    if ( controller != NULL ) {
		controller->updateDmx(data);
    }
}


/*****************************************************************************
 * Configuration
 *****************************************************************************/

void FreeDmxPlugin::configure()
{
    //FreeDmxApConfigure conf(this);
    //if (conf.exec() == QDialog::Accepted)
    //{

    //}
}


/**
 * Check, whether calling configure() on a plugin has any effect. If this
 * method returns false, the plugin cannot be configured by the user.
 *
 * @return true if the plugin can be configured, otherwise false.
 */
bool FreeDmxPlugin::canConfigure()
{
    /**
     * For the time being, only one AP is handled with factory default address
     * and port settings, so that no configuration is needed.
     */
    return false;
}

void FreeDmxPlugin::setParameter(quint32 universe, quint32 line, Capability type,
                             QString name, QVariant value)
{
    Q_UNUSED(universe)
    Q_UNUSED(line)
    Q_UNUSED(type)
    Q_UNUSED(value)

    /** This method is provided to QLC+ to set the plugin specific settings.
     *  Those settings are saved in a project workspace and when it is loaded,
     *  this method is called after QLC+ has opened the input/output lines
     *  mapped in the project workspace as well.
     */

    if (type == Output && name == "FreeDmxParameter")
    {
        // do something smart :)
    }

    /** Remember to call the base QLCIOPlugin method to actually inform
     *  QLC+ to store the parameter in the project workspace XML */
    QLCIOPlugin::setParameter(universe, line, type, name, value);
}

/*****************************************************************************
 * Plugin export
 ****************************************************************************/
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(freedmxplugin, FreeDmxPlugin)
#endif
