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
  // this->heatmap->setMaximumWidth(278);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(this->heatmap);

  vtkSmartPointer<vtkColorTransferFunction> ctfDefault = vtkSmartPointer<vtkColorTransferFunction>::New();
  // linear green colormap from tacc
  // ctfDefault->AddRGBPoint(0.0, 1.0, 0.984314, 0.901961);
  // ctfDefault->AddRGBPoint(0.05, 0.960784, 0.94902, 0.670588);
  // ctfDefault->AddRGBPoint(0.1, 0.886275, 0.921569, 0.533333);
  // ctfDefault->AddRGBPoint(0.15, 0.784314, 0.878431, 0.396078);
  // ctfDefault->AddRGBPoint(0.2, 0.666667, 0.839216, 0.294118);
  // ctfDefault->AddRGBPoint(0.25, 0.556863, 0.8, 0.239216);
  // ctfDefault->AddRGBPoint(0.3, 0.431373, 0.760784, 0.160784);
  // ctfDefault->AddRGBPoint(0.35, 0.317647, 0.721569, 0.113725);
  // ctfDefault->AddRGBPoint(0.4, 0.211765, 0.678431, 0.082353);
  // ctfDefault->AddRGBPoint(0.45, 0.109804, 0.631373, 0.05098);
  // ctfDefault->AddRGBPoint(0.5, 0.082353, 0.588235, 0.082353);
  // ctfDefault->AddRGBPoint(0.55, 0.109804, 0.54902, 0.152941);
  // ctfDefault->AddRGBPoint(0.6, 0.113725, 0.521569, 0.203922);
  // ctfDefault->AddRGBPoint(0.65, 0.117647, 0.490196, 0.243137);
  // ctfDefault->AddRGBPoint(0.7, 0.117647, 0.45098, 0.270588);
  // ctfDefault->AddRGBPoint(0.75, 0.113725, 0.4, 0.278431);
  // ctfDefault->AddRGBPoint(0.8, 0.109804, 0.34902, 0.278431);
  // ctfDefault->AddRGBPoint(0.85, 0.094118, 0.278431, 0.25098);
  // ctfDefault->AddRGBPoint(0.9, 0.086275, 0.231373, 0.219608);
  // ctfDefault->AddRGBPoint(0.95, 0.07451, 0.172549, 0.180392);
  // ctfDefault->AddRGBPoint(1.0, 0.054902, 0.109804, 0.121569);

  // black body radiation map
  ctfDefault->AddRGBPoint(0.0,0.0,0.0,0.0);
  ctfDefault->AddRGBPoint(0.14285714285714285,0.2567618382302789,0.08862237092250158,0.06900234709883349);
  ctfDefault->AddRGBPoint(0.2857142857142857,0.502299529628274,0.12275205976842546,0.10654041357261984);
  ctfDefault->AddRGBPoint(0.42857142857142855,0.7353154662963063,0.1982320329476474,0.12428036101896534);
  ctfDefault->AddRGBPoint(0.5714285714285714,0.8771435867383445,0.39490510462624345,0.03816328606394868);
  ctfDefault->AddRGBPoint(0.7142857142857142,0.911232394909533,0.631724377007152,0.10048201891972874);
  ctfDefault->AddRGBPoint(0.8571428571428571,0.9072006655243174,0.8550025783221541,0.18879408728283467);
  ctfDefault->AddRGBPoint(1.0,1.0,1.0,1.0);

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
  double filterMax = pqNodeEditorTimings::getMaxTime(gid);
  // double filterMax = 0.0;
  // std::vector<double>::iterator res = std::max_element(localTime_acc.begin(),localTime_acc.end());
  // if (res != localTime_acc.end())
  //   filterMax = *res;
  // for (auto vec : serverTimes_acc)
  // {
  //   res = std::max_element(vec.begin(),vec.end());
  //   if (res != vec.end())
  //     filterMax = std::max(filterMax,*res);
  // }
  // for (auto vec : dataServerTimes_acc)
  // {
  //   res = std::max_element(vec.begin(),vec.end());
  //   if (res != vec.end())
  //     filterMax = std::max(filterMax,*res);
  // }

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
      if (iter < rankVec.size())
        allRanks.emplace_back(rankVec[iter]); 
      //FIXME ^ this may mixup ranks from previous executions as the first rows
      // are populated first. Meaning if there are missing timings then those
      // holes are closed.
    }
    for (auto rankVec : dataServerTimes_acc)
    {
      if (iter < rankVec.size())
        allRanks.emplace_back(rankVec[iter]);
    }
    allRanks.resize(num_ranks); //add missing zeroes if timings are missing

    // sort them by execution time
    std::sort(allRanks.begin(),allRanks.end(), std::greater<double>());

    // iteratively fill a line of the heatmap
    for (int rank = 0; rank < num_ranks; rank++)
    {
      double current = allRanks[rank] / filterMax;
      const unsigned char* color = this->ctf->MapValue(current);
      QRgb value = qRgb(static_cast<int>(color[0]), static_cast<int>(color[1]), static_cast<int>(color[2]));
      image.setPixel(rank, iter, value);
    }
  }

  //set the heatmap
  this->heatmap->setImage(image);
}