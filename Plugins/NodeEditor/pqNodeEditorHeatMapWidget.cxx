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

  vtkSmartPointer<vtkColorTransferFunction> ctfDefault = vtkSmartPointer<vtkColorTransferFunction>::New();
  ctfDefault->AddRGBPoint(0.0, 1.0, 0.984314, 0.901961);
  ctfDefault->AddRGBPoint(0.05, 0.960784, 0.94902, 0.670588);
  ctfDefault->AddRGBPoint(0.1, 0.886275, 0.921569, 0.533333);
  ctfDefault->AddRGBPoint(0.15, 0.784314, 0.878431, 0.396078);
  ctfDefault->AddRGBPoint(0.2, 0.666667, 0.839216, 0.294118);
  ctfDefault->AddRGBPoint(0.25, 0.556863, 0.8, 0.239216);
  ctfDefault->AddRGBPoint(0.3, 0.431373, 0.760784, 0.160784);
  ctfDefault->AddRGBPoint(0.35, 0.317647, 0.721569, 0.113725);
  ctfDefault->AddRGBPoint(0.4, 0.211765, 0.678431, 0.082353);
  ctfDefault->AddRGBPoint(0.45, 0.109804, 0.631373, 0.05098);
  ctfDefault->AddRGBPoint(0.5, 0.082353, 0.588235, 0.082353);
  ctfDefault->AddRGBPoint(0.55, 0.109804, 0.54902, 0.152941);
  ctfDefault->AddRGBPoint(0.6, 0.113725, 0.521569, 0.203922);
  ctfDefault->AddRGBPoint(0.65, 0.117647, 0.490196, 0.243137);
  ctfDefault->AddRGBPoint(0.7, 0.117647, 0.45098, 0.270588);
  ctfDefault->AddRGBPoint(0.75, 0.113725, 0.4, 0.278431);
  ctfDefault->AddRGBPoint(0.8, 0.109804, 0.34902, 0.278431);
  ctfDefault->AddRGBPoint(0.85, 0.094118, 0.278431, 0.25098);
  ctfDefault->AddRGBPoint(0.9, 0.086275, 0.231373, 0.219608);
  ctfDefault->AddRGBPoint(0.95, 0.07451, 0.172549, 0.180392);
  ctfDefault->AddRGBPoint(1.0, 0.054902, 0.109804, 0.121569);
  this->ctf = ctfDefault;
}

pqNodeEditorHeatMapWidget::~pqNodeEditorHeatMapWidget(){}

void pqNodeEditorHeatMapWidget::update(vtkTypeUInt32 gid)
{
  std::vector<double> localTime_acc = pqNodeEditorTimings::getLocalTimings(gid);
  std::vector<std::vector<double>> serverTimes_acc = pqNodeEditorTimings::getServerTimings(gid);
  std::vector<std::vector<double>> dataServerTimes_acc = pqNodeEditorTimings::getDataServerTimings(gid);

  // check if there are any iterations and ranks
  int num_iter = localTime_acc.size();
  if (!num_iter && !serverTimes_acc.empty())
    num_iter = serverTimes_acc.front().size();

  int num_ranks = !localTime_acc.empty();
  num_ranks += serverTimes_acc.size();

  // abort if not
  if (!num_ranks || !num_iter)
    return;

  // // allocate the image
  QImage image(num_ranks,num_iter,QImage::Format::Format_RGB32);

  // get maxmimum of all execution times on each rank for this filter
  double filterMax = 0.0;
  std::vector<double>::iterator res = std::max_element(localTime_acc.begin(),localTime_acc.end());
  if (res != localTime_acc.end())
    filterMax = *res;
  for (auto vec : serverTimes_acc)
  {
    res = std::max_element(vec.begin(),vec.end());
    if (res != vec.end())
      filterMax = std::max(filterMax,*res);
  }
  for (auto vec : dataServerTimes_acc)
  {
    res = std::max_element(vec.begin(),vec.end());
    if (res != vec.end())
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
    for (auto rankVec : serverTimes_acc)
    {
      allRanks.emplace_back(rankVec[iter]);
    }
    for (auto rankVec : dataServerTimes_acc)
    {
      allRanks.emplace_back(rankVec[iter]);
    }

    // sort them by execution time
    std::sort(allRanks.begin(),allRanks.end(), std::greater<double>());

    // iteratively fill a line of the heatmap
    for (int rank = 0; rank < num_ranks; rank++)
    {
      double current = 1.0 - allRanks[rank] / filterMax;
      // std::cout << "current " << current << "   filtermax " << filterMax << std::endl;
      const unsigned char* color = this->ctf->MapValue(current);
      // std::cout << "r: " << static_cast<int>(color[0]) << "  g: " << static_cast<int>(color[1]) << "  b: " << static_cast<int>(color[2]) << std::endl;
      QRgb value = qRgb(static_cast<int>(color[0]), static_cast<int>(color[1]), static_cast<int>(color[2]));
      image.setPixel(rank, iter, value);
    }
  }

  //set the heatmap
  this->heatmap->setImage(image);
}