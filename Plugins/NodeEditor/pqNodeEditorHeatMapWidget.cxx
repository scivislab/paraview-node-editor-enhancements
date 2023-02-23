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

  int num_ranks = localTime_acc.size();
  if (!serverTimes_acc.empty())
    num_ranks += serverTimes_acc.front().size();

  // adding a pixel image 
  QImage i(num_ranks,1,QImage::Format::Format_RGB32);
  i.fill(QColor(Qt::red));
  i = i.scaled(50,10);

  this->heatmap->setImage(i);
}