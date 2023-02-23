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

#include <pqNodeEditorHeatMapWidget.h>
#include <pqNodeEditorTimings.h>
#include <pqNodeEditorUtils.h>

pqNodeEditorHeatMapWidget::pqNodeEditorHeatMapWidget()
{
  this->heatmap = new QHeatMap();
  this->heatmap->setMargin(0);
  this->heatmap->setMinimumHeight(40);
  this->heatmap->setMaximumWidth(278);
  this->heatmap->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(this->heatmap);
}

pqNodeEditorHeatMapWidget::~pqNodeEditorHeatMapWidget(){}

void pqNodeEditorHeatMapWidget::update(vtkTypeUInt32 gid)
{
  std::vector<double> localTime_acc = pqNodeEditorTimings::getLocalTimings(gid);
  std::vector<std::vector<double>> serverTimes_acc = pqNodeEditorTimings::getServerTimings(gid);
  std::vector<std::vector<double>> dataServerTimes_acc = pqNodeEditorTimings::getDataServerTimings(gid);

  // check if there are any iterations and ranks
  int num_iter = localTime_acc.size();
  if (!num_iter)
    num_iter = serverTimes_acc.size();

  int num_ranks = !localTime_acc.empty();
  if (!serverTimes_acc.empty())
    num_ranks += serverTimes_acc.front().size();

  // abort if not
  if (!num_ranks || !num_iter)
    return;

  // get maxmimum of all execution times on each rank for this filter
  double filterMax;
  std::vector<double>::iterator res = std::max_element(localTime_acc.begin(),localTime_acc.end());
  filterMax = *res;
  for (auto vec : serverTimes_acc)
  {
    res = std::max_element(vec.begin(),vec.end());
    filterMax = std::max(filterMax,*res);
  }
  for (auto vec : dataServerTimes_acc)
  {
    res = std::max_element(vec.begin(),vec.end());
    filterMax = std::max(filterMax,*res);
  }

  // for each iteration 
  for (int iter = 0; iter < num_iter; iter++)
  {
    //gather execution times of all ranks
    std::vector<double> allRanks;
    allRanks.reserve(num_ranks);
    if (iter < localTime_acc.size())
      allRanks.emplace_back(localTime_acc.at(iter));
    if (iter < serverTimes_acc.size())
      allRanks.insert(allRanks.end(), serverTimes_acc.at(iter).begin(), serverTimes_acc.at(iter).end());
    if (iter < dataServerTimes_acc.size())
      allRanks.insert(allRanks.end(), dataServerTimes_acc.at(iter).begin(), dataServerTimes_acc.at(iter).end());

    // sort them by execution time
    std::sort(allRanks.begin(),allRanks.end(), std::greater<double>());

    // setup color map for 

    // iteratively fill a line of the heatmap

  

  }

  // adding a pixel image 
  QImage i(num_ranks,1,QImage::Format::Format_RGB32);
  i.fill(QColor(Qt::red));
  i = i.scaled(50,10);

  //set the heatmap
  this->heatmap->setImage(i);
}