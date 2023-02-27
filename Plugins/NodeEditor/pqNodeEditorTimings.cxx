#include "pqNodeEditorTimings.h"

#include "pqServer.h"
#include "pqActiveObjects.h"

#include "vtkPVTimerInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <QtDebug>
#include <regex>
#include <algorithm>

namespace
{
  constexpr int CLIENT_PROCESS = 0;
  constexpr int SERVER_PROCESS = 1;
  constexpr int DATA_SERVER_PROCESS = 1;
  constexpr int RENDER_SERVER_PROCESS = 2;

  constexpr int DATA_MOVEMENT_CATEGORY = 0;
  constexpr int RENDERING_CATEGORY = 1;
  constexpr int APPLICATION_CATEGORY = 2;
  constexpr int PIPELINE_CATEGORY = 3;
  constexpr int PLUGINS_CATEGORY = 4;
  constexpr int EXECUTION_CATEGORY = 5;
}

std::map<vtkTypeUInt32, std::vector<double>> pqNodeEditorTimings::localTimings;
std::map<vtkTypeUInt32, std::vector<std::vector<double>>> pqNodeEditorTimings::serverTimings;
std::map<vtkTypeUInt32, std::vector<std::vector<double>>> pqNodeEditorTimings::dataServerTimings;
std::set<vtkTypeUInt32> pqNodeEditorTimings::globalIds;
double pqNodeEditorTimings::max;
double pqNodeEditorTimings::latestMax;

std::vector<vtkSmartPointer<vtkSMProxy>> pqNodeEditorTimings::LogRecorderProxies;

void pqNodeEditorTimings::refreshTimingLogs()
{
  // Get server
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    qWarning() << "No active server located. Cannot refresh timer-log.";
    return;
  }

  
  vtkSMSessionProxyManager* pxm = server->proxyManager();

  // //######################
  // pqNodeEditorTimings::LogRecorderProxies.push_back(pxm->NewProxy("misc", "LogRecorder"));
  // pqNodeEditorTimings::LogRecorderProxies[CLIENT_PROCESS]->SetLocation(vtkSMSession::CLIENT);
  // vtkSMPropertyHelper(pqNodeEditorTimings::LogRecorderProxies[CLIENT_PROCESS], "RankEnabled").Set(0);
  // pqNodeEditorTimings::LogRecorderProxies[CLIENT_PROCESS]->UpdateVTKObjects();

  // //pqNodeEditorTimings::Ui->processComboBox->addItem("Client");
  // //vtkNew<vtkPVServerInformation> serverInfo;
  //  vtkSmartPointer<vtkPVTimerInformation> serverInfo = vtkSmartPointer<vtkPVTimerInformation>::New();

  // server->session()->GatherInformation(vtkPVSession::CLIENT, serverInfo, 0);
  // pqNodeEditorTimings::addClientTimerInformation(serverInfo);
  // //pqNodeEditorTimings::RankNumbers.push_back(serverInfo->GetNumberOfProcesses());

  // // this is how to get logs from logrecorder proxies - these are the new logs
  // // vtkNew<vtkPVLogInformation> logInformation;
  // // logInformation->SetRank(0);
  // // pqNodeEditorTimings::LogRecorderProxies[CLIENT_PROCESS]->GatherInformation(logInformation);
  // // logInformation->GetLogs();
  

  // if (server->isRemote())
  // {
  //   if (server->isRenderServerSeparate())
  //   {
  //     pqNodeEditorTimings::LogRecorderProxies.push_back(pxm->NewProxy("misc", "LogRecorder"));
  //     pqNodeEditorTimings::LogRecorderProxies[DATA_SERVER_PROCESS]->SetLocation(vtkSMSession::DATA_SERVER);
  //     pqNodeEditorTimings::LogRecorderProxies[DATA_SERVER_PROCESS]->UpdateVTKObjects();

  //     pqNodeEditorTimings::LogRecorderProxies.push_back(pxm->NewProxy("misc", "LogRecorder"));
  //     pqNodeEditorTimings::LogRecorderProxies[RENDER_SERVER_PROCESS]->SetLocation(vtkSMSession::RENDER_SERVER);
  //     pqNodeEditorTimings::LogRecorderProxies[RENDER_SERVER_PROCESS]->UpdateVTKObjects();

  //     // pqNodeEditorTimings::Ui->processComboBox->addItem("Data Server");
  //     // pqNodeEditorTimings::Ui->processComboBox->addItem("Render Server");

  //     server->session()->GatherInformation(vtkSMSession::DATA_SERVER, serverInfo, 0);
  //     pqNodeEditorTimings::addServerTimerInformation(serverInfo, true);
  //     // pqNodeEditorTimings::RankNumbers.push_back(serverInfo->GetNumberOfProcesses());
  //     server->session()->GatherInformation(vtkSMSession::RENDER_SERVER, serverInfo, 0);
  //     pqNodeEditorTimings::addServerTimerInformation(serverInfo, false);
  //     // pqNodeEditorTimings::RankNumbers.push_back(serverInfo->GetNumberOfProcesses());
  //   }
  //   else
  //   {
  //     pqNodeEditorTimings::LogRecorderProxies.push_back(pxm->NewProxy("misc", "LogRecorder"));
  //     pqNodeEditorTimings::LogRecorderProxies[SERVER_PROCESS]->SetLocation(vtkSMSession::SERVERS);
  //     pqNodeEditorTimings::LogRecorderProxies[SERVER_PROCESS]->UpdateVTKObjects();

  //     // pqNodeEditorTimings::Ui->processComboBox->addItem("Server");

  //     server->session()->GatherInformation(vtkPVSession::SERVERS, serverInfo, 0);
  //     pqNodeEditorTimings::addServerTimerInformation(serverInfo, false);
  //     // pqNodeEditorTimings::RankNumbers.push_back(serverInfo->GetNumberOfProcesses());
  //   }
  // }
  // pqNodeEditorTimings::LogRecorderProxies[CLIENT_PROCESS]->InvokeCommand("ResetLog");
  // pqNodeEditorTimings::LogRecorderProxies[CLIENT_PROCESS]->UpdateVTKObjects();
  
  

  
  // and increase log buffer size
  vtkSMProxy* proxy = pxm->NewProxy("misc", "TimerLog");
  vtkSMPropertyHelper(proxy, "MaxEntries").Set(180000);
  proxy->UpdateVTKObjects();
  // proxy->Delete();

  // Get information about the local process.
  vtkSmartPointer<vtkPVTimerInformation> timerInfo = vtkSmartPointer<vtkPVTimerInformation>::New();
  timerInfo->SetLogThreshold(0.0);
  timerInfo->CopyFromObject(nullptr);
  server->session()->GatherInformation(vtkPVSession::CLIENT, timerInfo, 0);
  pqNodeEditorTimings::addClientTimerInformation(timerInfo);

  // Get information about servers.
  if (server->isRemote())
  {
    // Clear out information by creating a new info object.
    timerInfo = vtkSmartPointer<vtkPVTimerInformation>::New();
    timerInfo->SetLogThreshold(0.0);
    server->session()->GatherInformation(vtkPVSession::RENDER_SERVER, timerInfo, 0);
    pqNodeEditorTimings::addServerTimerInformation(timerInfo, false);

    // if data server is seperate get its timings too
    if (server->isRenderServerSeparate())
    {
      // We just reported on the render server.  Now report on the data server.
      timerInfo = vtkSmartPointer<vtkPVTimerInformation>::New();
      timerInfo->SetLogThreshold(0.0);
      server->session()->GatherInformation(vtkPVSession::DATA_SERVER, timerInfo, 0);
      pqNodeEditorTimings::addServerTimerInformation(timerInfo, true);
    }
  }
  proxy->UpdateVTKObjects();
  proxy->InvokeCommand("ResetLog");
  proxy->Delete();  

  pqNodeEditorTimings::updateMax();
}

double pqNodeEditorTimings::getLatestLocalTimings(vtkTypeUInt32 global_Id)
{
  double time = 0.0;
  std::vector<double> lt = getLocalTimings(global_Id);
  if (!lt.empty())
  {
    time = lt.back();
  }
  return time;
}

std::vector<double> pqNodeEditorTimings::getLatestServerTimings(vtkTypeUInt32 global_Id)
{
  std::vector<double> times;
  std::vector<std::vector<double>> st = getServerTimings(global_Id);
  for (auto vec : st)
  {
    if (!vec.empty())
    {
      times.emplace_back(vec.back());
    }
  }
  // if (!st.empty())
  // {
  //   times = st.back();
  // }
  return times;
}

std::vector<double> pqNodeEditorTimings::getLatestDataServerTimings(vtkTypeUInt32 global_Id)
{
  std::vector<double> times;
  std::vector<std::vector<double>> dst = getDataServerTimings(global_Id);
  for (auto vec : dst)
  {
    if (!vec.empty())
    {
      times.emplace_back(vec.back());
    }
  }
  // if (!dst.empty())
  // {
  //   times = dst.back();
  // }
  return times;
}

std::vector<double> pqNodeEditorTimings::getLocalTimings(vtkTypeUInt32 global_Id)
{
  std::vector<double> lt;
  try
  {
    lt = pqNodeEditorTimings::localTimings.at(global_Id);
  }
  catch (const std::out_of_range& e)
  {
    // global id does not occur in logs
  }
  return lt;
}

std::vector<std::vector<double>> pqNodeEditorTimings::getServerTimings(vtkTypeUInt32 global_Id)
{
  std::vector<std::vector<double>> st;
  try
  {
    st = pqNodeEditorTimings::serverTimings.at(global_Id);
  }
  catch (const std::out_of_range& e)
  {
    // global id does not occur in logs
  }
  return st;
}

std::vector<std::vector<double>> pqNodeEditorTimings::getDataServerTimings(vtkTypeUInt32 global_Id)
{
  std::vector<std::vector<double>> dst;
  try
  {
    dst = pqNodeEditorTimings::dataServerTimings.at(global_Id);
  }
  catch (const std::out_of_range& e)
  {
    // global id does not occur in logs
  }
  return dst;
}

double pqNodeEditorTimings::getMaxTime()
{
  return pqNodeEditorTimings::max;
}

double pqNodeEditorTimings::getMaxTime(vtkTypeUInt32 global_Id)
{
  double max = 0.0;
  std::vector<double> timings = getLocalTimings(global_Id);
  if (!timings.empty())
  {
    double temp = *std::max_element(timings.begin(),timings.end());
    max = std::max(temp, max);
  }
  
  std::vector<std::vector<double>> rankTimings = getServerTimings(global_Id);
  for (auto t : rankTimings)
  {
    if (!t.empty())
      {
        double temp = *std::max_element(t.begin(),t.end());
        max = std::max(temp, max);
      }
  }

  rankTimings = getDataServerTimings(global_Id);
  for (auto t : rankTimings)
  {
    if (!t.empty())
    {
      double temp = *std::max_element(t.begin(),t.end());
      max = std::max(temp, max);
    }
  }

  return max;
}

double pqNodeEditorTimings::getLatestMaxTime()
{
  return pqNodeEditorTimings::latestMax;
}

double pqNodeEditorTimings::getLatestMaxTime(vtkTypeUInt32 global_Id)
{
  double max = getLatestLocalTimings(global_Id);
  std::vector<double> lst = getLatestServerTimings(global_Id);
  if (!lst.empty())
  {
    double temp = *std::max_element(lst.begin(), lst.end());
    max = std::max(temp, max);
  }
  lst = getLatestDataServerTimings(global_Id);
  if (!lst.empty())
  {
    double temp = *std::max_element(lst.begin(), lst.end());
    max = std::max(temp, max);
  }
  return max;
}

void pqNodeEditorTimings::addClientTimerInformation(vtkSmartPointer<vtkPVTimerInformation> timerInfo)
{
  // check if there are logs to parse
  int numLogs = timerInfo->GetNumberOfLogs();
  std::string str;
  if (numLogs < 1)
  {
    qWarning() << "No client timer info could be retrieved";
  }
  else
  {
    // std::cout << "convert " << numLogs << " log in timings map" << std::endl;
    str = timerInfo->GetLog(0);
  }

  //create regex
  std::regex line_re("id:[ ]*([0-9]+),[ ]*(\\d+.\\d+(e-?\\d+)?)[ ]*seconds");
  auto log_begin = std::sregex_iterator(str.begin(), str.end(), line_re);
  auto log_end = std::sregex_iterator();

  // search for matches: first submatch is the global id and second is the execution time in seconds
  for (std::sregex_iterator i = log_begin; i != log_end; ++i)
  {
    std::smatch match = *i;
    std::string match_str = match.str();
    //std::cout << "match  " << match_str << '\n';

    // std::cout << "new match: \n";
    // std::cout << match.str()    << std::endl;
    // std::cout << match[0].str() << std::endl;
    // std::cout << match[1].str() << std::endl;
    // std::cout << match[2].str() << std::endl;

    double seconds = std::stod(match[2].str());
    vtkTypeUInt32 global_id = std::stoi(match[1].str());

    pqNodeEditorTimings::localTimings[global_id].emplace_back(seconds);
  }
}

void pqNodeEditorTimings::addServerTimerInformation(vtkSmartPointer<vtkPVTimerInformation> timerInfo, bool isDataServer)
{
  int numLogs = timerInfo->GetNumberOfLogs();
  if (numLogs < 1)
  {
    qWarning() << "No server timer info could be retrieved";
  }

  // std::cout << "Found " << numLogs << " server logs" << std::endl;
  // for (int i = 0; i < numLogs; i++)
  // {
  //   std::cout << "########### print log " << i << std::endl;
  //   std::cout << timerInfo->GetLog(i) << std::endl;
  // }

  for (int logId = 0; logId < numLogs; logId++)
  {
    std::string str = timerInfo->GetLog(logId);
    
    //create regex
    std::regex line_re("id:[ ]*([0-9]+),[ ]*(\\d+.\\d+(e-?\\d+)?)[ ]*seconds");
    auto log_begin = std::sregex_iterator(str.begin(), str.end(), line_re);
    auto log_end = std::sregex_iterator();

     // search for matches: first submatch is the global id and second is the execution time in seconds
    for (std::sregex_iterator i = log_begin; i != log_end; ++i)
    {
      std::smatch match = *i;

      // std::cout << "new match: \n";
      // std::cout << match.str()    << std::endl;
      // std::cout << match[0].str() << std::endl;
      // std::cout << match[1].str() << std::endl;
      // std::cout << match[2].str() << std::endl;

      double seconds = std::stod(match[2].str());
      vtkTypeUInt32 global_id = std::stoi(match[1].str());

      if (isDataServer)
      {
	      pqNodeEditorTimings::dataServerTimings[global_id].resize(numLogs);	
	      pqNodeEditorTimings::dataServerTimings[global_id][logId].emplace_back(seconds);
      }
      else
      {
        pqNodeEditorTimings::serverTimings[global_id].resize(numLogs);	
        pqNodeEditorTimings::serverTimings[global_id][logId].emplace_back(seconds);
      }
    }
  }
}

void pqNodeEditorTimings::updateMax()
{
  double new_max = 0.0;
  double new_latestMax = 0.0;

  // get max from local timings
  for (auto& timings : pqNodeEditorTimings::localTimings)
  {
    //only consider timings of currently active modules
    if (pqNodeEditorTimings::globalIds.count(timings.first))
    {
      for (size_t i = 0; i < timings.second.size(); i++)
      {
          double t = timings.second[i];
          new_max = std::max(new_max, t);
          if (i == timings.second.size() -1)
            new_latestMax = std::max(new_latestMax, t);
      }
    }
  }

  for (auto& timings : pqNodeEditorTimings::serverTimings)
  {
    if (pqNodeEditorTimings::globalIds.count(timings.first) )
    {
      for (auto& rank : timings.second)
      {
        for (size_t i = 0; i < rank.size(); i++)
        {
          double t = rank[i];
          new_max = std::max(new_max, t);
          if (i == rank.size() -1)
            new_latestMax = std::max(new_latestMax, t);
        }
      } 
    }
  }

  for (auto& timings : pqNodeEditorTimings::dataServerTimings)
  {
    if (pqNodeEditorTimings::globalIds.count(timings.first) )
    {
      for (auto& rank : timings.second)
      {
        for (size_t i = 0; i < rank.size(); i++)
        {
          double t = rank[i];
          new_max = std::max(new_max, t);
          if (i == rank.size() -1)
            new_latestMax = std::max(new_latestMax, t);
        }
      } 
    }
  }

  pqNodeEditorTimings::max = new_max;
  pqNodeEditorTimings::latestMax = new_latestMax;
}

void pqNodeEditorTimings::addGlobalId(vtkTypeUInt32 gid)
{
  pqNodeEditorTimings::globalIds.insert(gid);
}

void pqNodeEditorTimings::removeGlobalId(vtkTypeUInt32 gid)
{
  pqNodeEditorTimings::globalIds.erase(gid);
}