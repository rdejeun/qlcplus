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

    str += QString("<HTML>");
    str += QString("<HEAD>");
    str += QString("<TITLE>%1</TITLE>").arg(name());
    str += QString("</HEAD>");
    str += QString("<BODY>");

    str += QString("<P>");
    str += QString("<H3>%1</H3>").arg(name());
    str += tr("This plugin provides output for Eurolite freeDMX AP Wi-Fi interface.");
    str += QString("</P>");

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
        str += QString("<H3>%1</H3>").arg(outputs()[output]);
        str += QString("<ul>");
        str += QString("<li>Universe: %1</li>").arg(m_OutputLines[output].universe);
        str += QString("<li>Address: %1</li>").arg(m_OutputLines[output].address.toString());
        str += QString("<li>Port: %1</li>").arg(m_OutputLines[output].port);
        if ( m_OutputLines[output].controller != NULL )
            str += QString("<li>Sent/received packets: %1 / %2</li>").arg(m_OutputLines[output].controller->getPacketSentNumber()).arg(m_OutputLines[output].controller->getPacketReceivedNumber());
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
