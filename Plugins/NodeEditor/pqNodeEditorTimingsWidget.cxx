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

#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QHorizontalBarSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLegend>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QAbstractAxis>
#include <QLineEdit>
#include <QtCharts/QBoxPlotSeries>
#include <QtCharts/QLineSeries>
#include <QImage>

#include <vector>
#include <stdio.h>

#include <pqNodeEditorTimingsWidget.h>
#include <pqNodeEditorTimings.h>
#include <pqNodeEditorUtils.h>

QT_CHARTS_USE_NAMESPACE

pqNodeEditorTimingsWidget::pqNodeEditorTimingsWidget(QWidget *parent, vtkTypeUInt32 g_id) : global_id(g_id)
{
  setMinimumSize(pqNodeEditorUtils::CONSTS::NODE_WIDTH-2*pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH, 200);
  setMaximumSize(pqNodeEditorUtils::CONSTS::NODE_WIDTH-2*pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH, 200);

  pqNodeEditorTimings::addGlobalId(this->global_id);

  // create chart
  this->timingsChart = new QChart();
  this->timingsChart->setTheme(QChart::ChartThemeLight);
  this->timingsChart->setAnimationOptions(QChart::SeriesAnimations);
  this->timingsChart->layout()->setContentsMargins(0,0,0,0);
  this->timingsChart->setBackgroundRoundness(0);
  this->timingsChart->setBackgroundVisible(false);
  this->timingsChart->setPlotAreaBackgroundVisible(true);
  this->timingsChart->legend()->setVisible(false);
  this->timingsChart->legend()->hide();

  QChartView *chartView = new QChartView(this->timingsChart);
  chartView->setRenderHint(QPainter::Antialiasing);
  chartView->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding));

  this->heatmap = new pqNodeEditorHeatMapWidget();
  this->heatmap->setVisible(false);
  
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->setAlignment(Qt::AlignHCenter);
  layout->addWidget(this->heatmap);
  layout->addWidget(chartView);

  this->updateGeometry();
  
  this->setupQChartAxis();
  this->updateTimings();
}

pqNodeEditorTimingsWidget::~pqNodeEditorTimingsWidget()
{
  pqNodeEditorTimings::removeGlobalId(this->global_id);
}

void pqNodeEditorTimingsWidget::updateTimings()
{
  if (this->mode == 1)
    updateTimingsBoxPlot();
  else if (this->mode == 2)
    updateTimingsBarChart();
  else if (this->mode == 3)
    updateTimingsHeatMap();
  else
    updateTimingsLinePlot();

}

void pqNodeEditorTimingsWidget::updateTimingsBoxPlot()
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
  allTimes_bs->setBrush(pqNodeEditorUtils::CONSTS::COLOR_BASE_ORANGE);
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

  // append acc then local as first categories
  QStringList categories;
  categories << "acc";
  categories << "l";
  
  // set min max
  double min = 0.0;
  double max = pqNodeEditorTimings::getMaxTime();
  max += (max-min)/10.0;

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

  std::vector<QAbstractAxis*> axis = this->updateQChartAxis(min, max, categories);
  boxplots->attachAxis(axis.at(0));
  boxplots->attachAxis(axis.at(1));

  this->updateGeometry();
}

void pqNodeEditorTimingsWidget::updateTimingsLinePlot()
{
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
  allTimes_bs->setBrush(pqNodeEditorUtils::CONSTS::COLOR_BASE_ORANGE);
  boxplots->append(allTimes_bs);

  // if there are local timings: take their amount and make that many timing plots
  // if there are none: do not include them or set them to 0
  // the amount of timings is then determined by server times and if thos are 0 then dataservertimes

  // 1. step specify number of line series:
  int numLineSeries = 0;
  int localNumLineSeries = localTime_acc.size();
  if (! numLineSeries && !serverTimes_acc.empty())
  {
    numLineSeries += serverTimes_acc.at(0).size();
  }
  if (! numLineSeries && !dataServerTimes_acc.empty())
  {
    numLineSeries += dataServerTimes_acc.at(0).size();
  }
  std::vector<QLineSeries*> localLineSeries;
  std::vector<QLineSeries*> lineSeries;
  for(int i = 0; i < localNumLineSeries; i++)
  {
    localLineSeries.emplace_back(new QLineSeries());
  }
  for(int i = 0; i < numLineSeries; i++)
  {
    lineSeries.emplace_back(new QLineSeries());
  }

  for (int lls_idx = 0; lls_idx < localNumLineSeries; lls_idx++)
  {
    if (localTime_acc.size() > lls_idx)
    {
      localLineSeries[lls_idx]->append(QPointF(1.0,localTime_acc[lls_idx]));
      // std::cout << "append local time" << localTime_acc[lls_idx] << std::endl;
    }
  }
  
  for (int ls_idx = 0; ls_idx < numLineSeries; ls_idx++)
  {
    for (int st_idx = 0; st_idx < serverTimes_acc.size(); st_idx++)
    {
      auto st = serverTimes_acc[st_idx];
      if (st.size() > ls_idx)
      {
        lineSeries[ls_idx]->append(QPointF(2.0+static_cast<double>(st_idx),st[ls_idx]));
      }
    } 

    for (int st_idx = 0; st_idx < dataServerTimes_acc.size(); st_idx++)
    {
      auto st = dataServerTimes_acc[st_idx];
      if (st.size() > ls_idx)
      {
        lineSeries[ls_idx]->append(QPointF(2.0+static_cast<double>(st_idx),st[ls_idx])); //TODO check if there is an error in the indexing here
      }
    } 
  }

  // add series to chart
  int max_series = 5;
  float max_width = 3;

  for (int i = 0; i < localLineSeries.size(); i++)
  {    
    max_series = static_cast<int>(localLineSeries.size());
    int step_transp = 255/max_series;
    QPen pen(QColor(61, 107, 233, step_transp*(i+1)));
    if (i == localLineSeries.size()-1)
    {
      pen = QPen(pqNodeEditorUtils::CONSTS::COLOR_BASE_ORANGE);
    }  
    float size_step = (max_width-1.0f)/max_series;
    pen.setWidthF(1.0f + size_step*(i+1));

    QLineSeries* ls = localLineSeries[i];
    ls->setPen(pen);
    ls->setPointsVisible(true);
    this->timingsChart->addSeries(ls);

  }

  //TODO add only max series line plots
  for (int i = 0; i < lineSeries.size(); i++)
  {    
    max_series = static_cast<int>(lineSeries.size());
    int step_transp = 255/max_series;
    QPen pen(QColor(61, 107, 233, step_transp*(i+1)));
    if (i == lineSeries.size()-1)
    {
      pen = QPen(pqNodeEditorUtils::CONSTS::COLOR_BASE_ORANGE);
    }
    float size_step = (max_width-1.0f)/max_series;
    pen.setWidthF(1.0f + size_step*(i+1));
    

    QLineSeries* ls = lineSeries[i];
    ls->setPen(pen);
    ls->setPointsVisible(true);
    this->timingsChart->addSeries(ls);
  }
  
  this->timingsChart->addSeries(boxplots);

  // append local as first category
  QStringList categories;
  categories << "acc";
  categories << "l";
  
  // set min max
  double min = 0.0;
  double max = pqNodeEditorTimings::getMaxTime();
  max += (max-min)/10.0;

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

  std::vector<QAbstractAxis*> axis = this->updateQChartAxis(min, max, categories);
  boxplots->attachAxis(axis.at(0));
  boxplots->attachAxis(axis.at(1));
  for (auto ls : localLineSeries)
  {
    ls->attachAxis(axis.at(0));
    ls->attachAxis(axis.at(1));
  }
  for (auto ls : lineSeries)
  {
    ls->attachAxis(axis.at(0));
    ls->attachAxis(axis.at(1));
  }

  this->updateGeometry();
}

void pqNodeEditorTimingsWidget::updateTimingsBarChart()
{

  // fetch data from pqNodeEditorTimings
  double localTime = pqNodeEditorTimings::getLatestLocalTimings(this->global_id);
  std::vector<double> serverTimes = pqNodeEditorTimings::getLatestServerTimings(this->global_id);
  std::vector<double> dataServerTimes = pqNodeEditorTimings::getLatestDataServerTimings(this->global_id); // this is only needed if render and data server are seperate

  // remove old data if any is there
  this->timingsChart->removeAllSeries();

  // append local as first category
  QStringList categories;
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
  }

  max = pqNodeEditorTimings::getMaxTime();
  max += (max-min)/10;

  // set properties of  QBarSeries
  data->setBorderColor(QColor(Qt::transparent));
  QBarSeries* timingBarSeries = new QBarSeries();
  timingBarSeries->setLabelsVisible(false);
  timingBarSeries->append(data);

  // add series to chart
  this->timingsChart->addSeries(timingBarSeries);

  std::vector<QAbstractAxis*> axis = this->updateQChartAxis(min, max, categories);
  timingBarSeries->attachAxis(axis.at(0));
  timingBarSeries->attachAxis(axis.at(1));
}

void pqNodeEditorTimingsWidget::updateTimingsHeatMap()
{
// fetch data from pqNodeEditorTimings
  double localTime = pqNodeEditorTimings::getLatestLocalTimings(this->global_id);
  std::vector<double> serverTimes = pqNodeEditorTimings::getLatestServerTimings(this->global_id);
  std::vector<double> dataServerTimes = pqNodeEditorTimings::getLatestDataServerTimings(this->global_id); // this is only needed if render and data server are seperate

  // remove old data if any is there
  this->timingsChart->removeAllSeries();
  
  // set min max
  double min = 0.0;
  double max = localTime;
  max = pqNodeEditorTimings::getMaxTime();
  max += (max-min)/10;

  //create QBarSet and append local time
  QBarSet* data = new QBarSet("");

  double filterMaxTime = localTime;
  for (double time : serverTimes)
    filterMaxTime = std::max(time, filterMaxTime);
  for (double time : dataServerTimes)
    filterMaxTime = std::max(time, filterMaxTime);
  *data << filterMaxTime;

  // set properties of  QBarSeries
  data->setBorderColor(QColor(Qt::transparent));
  QHorizontalBarSeries* timingBarSeries = new QHorizontalBarSeries();
  timingBarSeries->setLabelsVisible(false);
  timingBarSeries->append(data);
  timingBarSeries->setBarWidth(0.8);

  // add series to chart
  this->timingsChart->addSeries(timingBarSeries);

  QValueAxis* valAxis = this->updateQChartAxis(min,max);
  timingBarSeries->attachAxis(valAxis);

  this->heatmap->update(this->global_id);
}

void pqNodeEditorTimingsWidget::mousePressEvent(QMouseEvent *event)
{
  this->mode = (this->mode+1) % 4;
  this->heatmap->setVisible(!static_cast<bool>(mode-3));
  this->updateTimings();
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

void pqNodeEditorTimingsWidget::sortMPIRanksByTime(std::vector<double> timings)
{}

void pqNodeEditorTimingsWidget::setupQChartAxis()
{
  // set colors
  QColor c = palette().text().color();
  QColor g = palette().dark().color();
  QPen axisPen(palette().dark().color());
  axisPen.setWidth(1);
  QFont f;
  f.setPointSize(8);

  //category x axis
  QBarCategoryAxis *catAxisX = new QBarCategoryAxis();
  catAxisX->setLabelsBrush(QBrush(c));
  catAxisX->setGridLineColor(g);
  catAxisX->setLinePen(axisPen);
  catAxisX->setLabelsFont(f);
  this->timingsChart->addAxis(catAxisX, Qt::AlignBottom);

  // value x axis
  QValueAxis *valAxisX = new QValueAxis();
  valAxisX->setTitleText(QString("max rank time"));
  valAxisX->setLabelsBrush(QBrush(c));
  valAxisX->setGridLineColor(g);
  valAxisX->setLinePen(axisPen);
  valAxisX->setLabelFormat(QString("%.1f"));
  valAxisX->setLabelsFont(f);
  this->timingsChart->addAxis(valAxisX, Qt::AlignBottom);

  // configure vertical axis and set range
  QValueAxis *axisY = new QValueAxis();
  axisY->setLabelsBrush(QBrush(c));
  axisY->setGridLineColor(g);
  axisY->setLinePen(axisPen);
  axisY->setLabelFormat(QString("%.1f"));
  axisY->setLabelsFont(f);
  this->timingsChart->addAxis(axisY, Qt::AlignLeft);

}

QValueAxis* pqNodeEditorTimingsWidget::updateQChartAxis(double min, double max)
{
  QList<QAbstractAxis*> axisListHoriz = this->timingsChart->axes(Qt::Horizontal);
  QValueAxis* valAxisX = static_cast<QValueAxis*>(axisListHoriz.at(1));
  valAxisX->setTitleText(QString("max rank time"));
  valAxisX->setRange(min,max);

  valAxisX->setVisible(true);
  QList<QAbstractAxis*> axisListVert = this->timingsChart->axes(Qt::Vertical);
  axisListVert.at(0)->setVisible(false);
  axisListHoriz.at(0)->setVisible(false);
  return valAxisX;
}

std::vector<QAbstractAxis*> pqNodeEditorTimingsWidget::updateQChartAxis(double min, double max, QStringList categories)
{
  QList<QAbstractAxis*> axisListHoriz = this->timingsChart->axes(Qt::Horizontal);
  QBarCategoryAxis* catAxisX = static_cast<QBarCategoryAxis*>(axisListHoriz.at(0));
  catAxisX->clear();
  catAxisX->append(categories);
  catAxisX->setRange("acc","");

  QList<QAbstractAxis*> axisListVert = this->timingsChart->axes(Qt::Vertical);
  QAbstractAxis* valAxisY = axisListVert.at(0);
  valAxisY->setRange(min,max);

  // update visibility
  catAxisX->setVisible(true);
  axisListHoriz.at(1)->setVisible(false);
  valAxisY->setVisible(true);

  std::vector<QAbstractAxis*> axis = {catAxisX, valAxisY};
  return axis;
}