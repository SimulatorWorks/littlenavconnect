/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "datareaderthread.h"

#include "navservercommon.h"
#include "simconnecthandler.h"
#include "settings/settings.h"

#include <QDebug>
#include <QDateTime>

DataReaderThread::DataReaderThread(QObject *parent, bool verboseLog)
  : QThread(parent), verbose(verboseLog)
{
  qDebug() << "Datareader started";
  setObjectName("DataReaderThread");

  using atools::settings::Settings;
}

DataReaderThread::~DataReaderThread()
{
  qDebug() << "Datareader deleted";
}

void DataReaderThread::connectToSimulator(SimConnectHandler *handler)
{
  int counter = 0;
  while(!terminate)
  {
    if((counter % reconnectRateSec) == 0)
    {
      if(handler->connect())
        break;

      qInfo(gui).noquote() << tr("Not connected to the simulator. Will retry in %1 seconds.").arg(
        reconnectRateSec);
      counter = 0;
    }
    counter++;
    QThread::sleep(1);
  }
  qInfo(gui).noquote() << tr("Connected to simulator.");
}

void DataReaderThread::run()
{
  qDebug() << "Datareader run";

  SimConnectHandler handler(verbose);

  // Connect to the simulator
  connectToSimulator(&handler);

  int i = 0;

  while(!terminate)
  {
    atools::fs::sc::SimConnectData data;

    if(handler.fetchData(data))
    {
      data.setPacketId(i);
      data.setPacketTimestamp(QDateTime::currentDateTime().toTime_t());
      emit postSimConnectData(data);
      i++;
    }
    else
    {
      if(handler.getState() != sc::OK)
      {
        qWarning(gui).noquote() << tr("Error fetching data from simulator.");

        if(!handler.isSimRunning())
          // Try to reconnect if we lost connection to simulator
          connectToSimulator(&handler);
      }
    }
    QThread::msleep(updateRate);
  }
  terminate = false; // Allow restart
  qDebug() << "Datareader exiting run";
}

void DataReaderThread::setReconnectRateSec(int reconnectSec)
{
  reconnectRateSec = reconnectSec;
}

void DataReaderThread::setUpdateRate(unsigned int updateRateMs)
{
  updateRate = updateRateMs;
}

void DataReaderThread::setTerminate(bool terminateFlag)
{
  terminate = terminateFlag;
}
