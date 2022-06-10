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

#include <QtCharts/QBoxPlotSeries>

#include <vector>
#include <stdio.h>

#include <pqNodeEditorUtils.h>

QT_CHARTS_USE_NAMESPACE

pqNodeEditorTimingsWidget::pqNodeEditorTimingsWidget(QWidget *parent, vtkTypeUInt32 g_id) : global_id(g_id)
{
  setMinimumSize(200, 200);

  pqNodeEditorTimings::addGlobalId(this->global_id);

  // create chart
  this->timingsChart = new QChart();
  this->timingsChart->setTheme(QChart::ChartThemeLight);
  this->timingsChart->setAnimationOptions(QChart::SeriesAnimations);
  // this->timingsChart->setAnimationOptions(QChart::AnimationOption::NoAnimation);
  this->timingsChart->layout()->setContentsMargins(0,0,0,0);
  this->timingsChart->setBackgroundRoundness(0);
  this->timingsChart->setBackgroundVisible(false);
  this->timingsChart->setPlotAreaBackgroundVisible(true);
  this->timingsChart->legend()->setVisible(false);
  this->timingsChart->legend()->hide();
  
  QColor c = palette().dark().color();
  this->updateTimings();
    
  QChartView *chartView = new QChartView(this->timingsChart);
  chartView->setRenderHint(QPainter::Antialiasing);
  
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->addWidget(chartView);
}

pqNodeEditorTimingsWidget::~pqNodeEditorTimingsWidget()
{
  pqNodeEditorTimings::removeGlobalId(this->global_id);
}

void pqNodeEditorTimingsWidget::updateTimings()
{
  //############ BOX PLOTS #################
  std::vector<double> localTime_acc = pqNodeEditorTimings::getLocalTimings(this->global_id);
  std::vector<std::vector<double>> serverTimes_acc = pqNodeEditorTimings::getServerTimings(this->global_id);
  std::vector<std::vector<double>> dataServerTimes_acc = pqNodeEditorTimings::getDataServerTimings(this->global_id);

  std::vector<double> allTimes_acc = localTime_acc;
  for (auto vec : serverTimes_acc)
  {
    allTimes_acc.insert(allTimes_acc.end(),vec.begin(),vec.end());
  }
  for (auto vec : dataServerTimes_acc)
  {
    allTimes_acc.insert(allTimes_acc.end(),vec.begin(),vec.end());
  }
  
  this->timingsChart->removeAllSeries();

  QBoxPlotSeries *boxplots = new QBoxPlotSeries();

  // all times accumulated
  QBoxSet* allTimes_bs = this->createBoxSetFromVector(allTimes_acc);
  allTimes_bs->setBrush(pqNodeEditorUtils::CONSTS::COLOR_DARK_ORANGE);
  boxplots->append(allTimes_bs);

  // local times
  QBoxSet* localTime_bs = this->createBoxSetFromVector(localTime_acc);
  localTime_bs->setBrush(palette().highlight());
  boxplots->append(localTime_bs);

  // server times
  for (std::vector<double> ts : serverTimes_acc)
  {
    QBoxSet* temp = this->createBoxSetFromVector(ts);
    temp->setBrush(palette().mid());
    boxplots->append(temp);
  }

  // data server times
  for (std::vector<double> ts : dataServerTimes_acc)
  {
    QBoxSet* temp = this->createBoxSetFromVector(ts);
    temp->setBrush(palette().mid());
    boxplots->append(temp);
  }

  // add series to chart
  this->timingsChart->addSeries(boxplots);

  // append local as first category
  QStringList categories;
  categories << "acc";
  categories << "l";
  
  // set min max
  double min = 0.0;
  double max = pqNodeEditorTimings::getMaxTime();

  //append server categories
  for (int stIdx = 0; stIdx < serverTimes_acc.size(); stIdx++)
  {
    if (stIdx == 0)
      categories << "s0";
    else
      categories << QString("s") + QString::number(stIdx);
  }
  
  //append data server times and categories
  for (int stIdx = 0; stIdx < dataServerTimes_acc.size(); stIdx++)
  {
    if (stIdx == 0)
      categories << "d0";
    else
      categories << QString("d") + QString::number(stIdx);
  }

  // set colors
  QColor c = palette().text().color();
  QColor g = palette().dark().color();
  QPen axisPen(palette().dark().color());
  axisPen.setWidth(1);

  // configure horizontal axis and set categories
  QList<QAbstractAxis*> axisListHoriz = this->timingsChart->axes(Qt::Horizontal);
  if (axisListHoriz.empty())
  {
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->setLabelsBrush(QBrush(c));
    axisX->setGridLineColor(g);
    axisX->append(categories);
    axisX->setRange("acc","");
    this->timingsChart->addAxis(axisX, Qt::AlignBottom);
    boxplots->attachAxis(axisX);

    axisX->setLinePen(axisPen);
  }
  else
  {
    static_cast<QBarCategoryAxis*>(axisListHoriz.at(0))->clear();
    static_cast<QBarCategoryAxis*>(axisListHoriz.at(0))->append(categories);
    boxplots->attachAxis(axisListHoriz.at(0));
  }

  // configure vertical axis and set range
  QList<QAbstractAxis*> axisList = this->timingsChart->axes(Qt::Vertical);
  if (axisList.empty())
  {
    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelsBrush(QBrush(c));
    axisY->setGridLineColor(g);
    this->timingsChart->addAxis(axisY, Qt::AlignLeft);
    boxplots->attachAxis(axisY);
    axisY->setRange(min,max);

    axisY->setLinePen(axisPen);
  }
  else
  {
    boxplots->attachAxis(axisList.at(0));
    axisList.at(0)->setRange(min,max);
  }

/*
  //############ BAR CHARTS ################

  // fetch data from pqNodeEditorTimings
  double localTime = pqNodeEditorTimings::getLatestLocalTimings(this->global_id);
  std::vector<double> serverTimes = pqNodeEditorTimings::getLatestServerTimings(this->global_id);
  std::vector<double> dataServerTimes = pqNodeEditorTimings::getLatestDataServerTimings(this->global_id); // this is only needed if render and data server are seperate

  // remove old data if any is there
  // this->timingsChart->removeAllSeries();

  // append local as first category
  QStringList categories;
  categories << "acc";
  categories << "l";
  
  // set min max
  double min = 0.0;
  double max = localTime;

  //create QBarSet and append local time
  QBarSet* data = new QBarSet("");
  *data << localTime;

  //append server times and categories
  for (int stIdx = 0; stIdx < serverTimes.size(); stIdx++)
  {
    if (stIdx == 0)
      categories << "s0";
    else
      categories << QString("s") + QString::number(stIdx);
    double serverTime = serverTimes.at(stIdx);
    *data << serverTime;
    // max = serverTime > max ? serverTime : max;
  }
  
  //append data server times and categories
  for (int stIdx = 0; stIdx < dataServerTimes.size(); stIdx++)
  {
    if (stIdx == 0)
      categories << "d0";
    else
      categories << QString("d") + QString::number(stIdx);
    double dataServerTime = dataServerTimes.at(stIdx);
    *data << dataServerTime;
    // max = dataServerTime > max ? dataServerTime : max;
  }

  max = pqNodeEditorTimings::getMaxTime();

  // set properties of  QBarSeries
  data->setBorderColor(QColor(Qt::transparent));
  QBarSeries* timingBarSeries = new QBarSeries();
  timingBarSeries->setLabelsVisible(false);
  timingBarSeries->append(data);

  // add series to chart
  // this->timingsChart->addSeries(timingBarSeries);

  // set colors
  QColor c = palette().text().color();
  QColor g = palette().dark().color();
  QPen axisPen(palette().dark().color());
  axisPen.setWidth(1);

  // configure horizontal axis and set categories
  QList<QAbstractAxis*> axisListHoriz = this->timingsChart->axes(Qt::Horizontal);
  if (axisListHoriz.empty())
  {
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->setLabelsBrush(QBrush(c));
    axisX->setGridLineColor(g);
    axisX->append(categories);
    axisX->setRange("acc","");
    this->timingsChart->addAxis(axisX, Qt::AlignBottom);
    timingBarSeries->attachAxis(axisX);
    boxplots->attachAxis(axisX);

    axisX->setLinePen(axisPen);
  }
  else
  {
    static_cast<QBarCategoryAxis*>(axisListHoriz.at(0))->clear();
    static_cast<QBarCategoryAxis*>(axisListHoriz.at(0))->append(categories);
    timingBarSeries->attachAxis(axisListHoriz.at(0));
    boxplots->attachAxis(axisListHoriz.at(0));
  }

  // configure vertical axis and set range
  QList<QAbstractAxis*> axisList = this->timingsChart->axes(Qt::Vertical);
  if (axisList.empty())
  {
    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelsBrush(QBrush(c));
    axisY->setGridLineColor(g);
    this->timingsChart->addAxis(axisY, Qt::AlignLeft);
    timingBarSeries->attachAxis(axisY);
    boxplots->attachAxis(axisY);
    axisY->setRange(min,max);

    axisY->setLinePen(axisPen);
  }
  else
  {
    timingBarSeries->attachAxis(axisList.at(0));
    boxplots->attachAxis(axisList.at(0));
    axisList.at(0)->setRange(min,max);
  }
  */
  
  this->updateGeometry();
}


QBoxSet* pqNodeEditorTimingsWidget::createBoxSetFromVector(std::vector<double> timings)
{
  QBoxSet* bs = new QBoxSet();
  if (timings.empty())
  {
    bs->setPen(QPen(Qt::transparent));
    return bs;
  }
  
  std::sort(timings.begin(),timings.end());
  double median = timings[timings.size()/2];
  double low_quart = timings[timings.size()/2 + timings.size()/4];
  double upp_quart = timings[timings.size()/4];

  bs->setValue(QBoxSet::LowerExtreme, timings.front());
  bs->setValue(QBoxSet::UpperExtreme, timings.back());
  bs->setValue(QBoxSet::Median, median);
  bs->setValue(QBoxSet::LowerQuartile, low_quart);
  bs->setValue(QBoxSet::UpperQuartile, upp_quart);
  return bs;
}
