/*=========================================================================

  Program:   ParaView
  Plugin:    NodeEditor

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  ParaViewPluginsNodeEditor - BSD 3-Clause License - Copyright (C) 2021 Jonas Lukasczyk

  See the Copyright.txt file provided
  with ParaViewPluginsNodeEditor for license information.
-------------------------------------------------------------------------*/

#include "pqNodeEditorTimingsWidget.h"
#include "pqNodeEditorTimings.h"
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLegend>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QAbstractAxis>
#include <QLineEdit>

#include <vector>
#include <stdio.h>

QT_CHARTS_USE_NAMESPACE

pqNodeEditorTimingsWidget::pqNodeEditorTimingsWidget(QWidget *parent, vtkTypeUInt32 g_id) : global_id(g_id)
{
  setMinimumSize(200, 200);

  // create chart
  this->timingsChart = new QChart();
  //this->timingsChart->setTheme(QChart::ChartThemeDark);
  this->timingsChart->setAnimationOptions(QChart::SeriesAnimations);
  this->timingsChart->layout()->setContentsMargins(0, 0, 0, 0);
  this->timingsChart->setBackgroundRoundness(0);
  this->timingsChart->setBackgroundVisible(false);
  this->timingsChart->setPlotAreaBackgroundVisible(true);
  this->timingsChart->legend()->setVisible(false);
  

  QColor c = palette().mid().color();
  this->timingsChart->setPlotAreaBackgroundBrush(QBrush(c));
  this->timingsChart->setPlotAreaBackgroundPen(QPen(c));

  this->updateTimings();
  //std::cout << "chart theme is " << this->timingsChart->theme() << std::endl;
    
  QChartView *chartView = new QChartView(this->timingsChart);
  chartView->setRenderHint(QPainter::Antialiasing);
  
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->addWidget(chartView);
}

pqNodeEditorTimingsWidget::~pqNodeEditorTimingsWidget()
{
}

void pqNodeEditorTimingsWidget::updateTimings()
{
  // fetch data from pqNodeEditorTimings
  double localTime = pqNodeEditorTimings::getLocalTimings(this->global_id);
  std::vector<double> serverTimes = pqNodeEditorTimings::getServerTimings(this->global_id);
  std::vector<double> dataServerTimes = pqNodeEditorTimings::getDataServerTimings(this->global_id); // this is only needed if render and data server are seperate

  // remove old data if any is there
  this->timingsChart->removeAllSeries();

  QStringList categories;
  categories << "l";
  
  double min = 0.0;
  double max = localTime;
  QBarSet* data = new QBarSet("");
  *data << localTime;

  for (int stIdx = 0; stIdx < serverTimes.size(); stIdx++)
  {
    if (stIdx == 0)
      categories << "s0";
    else
      categories << QString("s") + QString::number(stIdx);
    double serverTime = serverTimes.at(stIdx);
    *data << serverTime;
    max = serverTime > max ? serverTime : max;
  }
  
  for (int stIdx = 0; stIdx < dataServerTimes.size(); stIdx++)
  {
    if (stIdx == 0)
      categories << "d0";
    else
      categories << QString("d") + QString::number(stIdx);
    double dataServerTime = dataServerTimes.at(stIdx);
    *data << dataServerTime;
    max = dataServerTime > max ? dataServerTime : max;
  }

  max = pqNodeEditorTimings::getMaxTime();

  QBarSeries* timingBarSeries = new QBarSeries();
  timingBarSeries->setLabelsVisible(false);
  timingBarSeries->append(data);

  // add series to chart
  this->timingsChart->addSeries(timingBarSeries);

  QColor c = palette().text().color();

  QList<QAbstractAxis*> axisListHoriz = this->timingsChart->axes(Qt::Horizontal);
  if (axisListHoriz.empty())
  {
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->setLabelsBrush(QBrush(c));
    axisX->append(categories);
    axisX->setRange("l","");
    this->timingsChart->addAxis(axisX, Qt::AlignBottom);
    timingBarSeries->attachAxis(axisX);
  }
  else
  {
    static_cast<QBarCategoryAxis*>(axisListHoriz.at(0))->clear();
    static_cast<QBarCategoryAxis*>(axisListHoriz.at(0))->append(categories);
    timingBarSeries->attachAxis(axisListHoriz.at(0));
  }

  QList<QAbstractAxis*> axisList = this->timingsChart->axes(Qt::Vertical);
  if (axisList.empty())
  {
    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelsBrush(QBrush(c));
    this->timingsChart->addAxis(axisY, Qt::AlignLeft);
    timingBarSeries->attachAxis(axisY);
    axisY->setRange(min,max);
  }
  else
  {
    timingBarSeries->attachAxis(axisList.at(0));
    axisList.at(0)->setRange(min,max);
  }
  
  this->updateGeometry();
}
