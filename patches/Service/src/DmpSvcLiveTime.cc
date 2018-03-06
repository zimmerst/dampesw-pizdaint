/*
 * =====================================================================================
 *
 *       Filename:  DmpSvcLiveTime.cc
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2016-11-17 17:05:48 +08:00
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  Jiang Wei (jiangwei@pmo.ac.cn)
 *   Organization:  PMO
 *
 * =====================================================================================
 */
#include "DmpSvcLiveTime.h"
#include "DmpSvcHKDataRead.h"
#include "DmpIOSvc.h"
#include "DmpHKDPayloadDataProcesser.h"
#include "TTree.h"
#include "TString.h"
#include "TSQLServer.h"
#include "TSQLResult.h"
#include "TSQLRow.h"
#include <map>
#include <utility>
#include <cstdlib>
#include <fstream>
DmpSvcLiveTime* gSvcLiveTime = DmpSvcLiveTime::GetInstance();
DmpSvcLiveTime* DmpSvcLiveTime::GetInstance()
{
    static DmpSvcLiveTime instance;
    return &instance;
}
DmpSvcLiveTime::DmpSvcLiveTime()
    : fServer(NULL),
      fResult(NULL),
      fRows(NULL),
      fIsLoadedHKData(false),
      fHasLoadFileMap(false)
{
}
DmpSvcLiveTime::~DmpSvcLiveTime() {}
double DmpSvcLiveTime::GetLiveTime(double start, double end)
{
    if(ConnectedDB())
    {
        return GetLiveTimeFromDB(start, end);
    }
    if(CanReadHKTree())
    {
        return GetLiveTimeFromHK(start, end);
    }
    if(ExistedBinFile())
    {
        return GetLiveTimeFromBin(start, end);
    }
    DmpLogError << "Cannot find any source for LiveTime" << DmpLogEndl;
}
double DmpSvcLiveTime::GetLiveTimeFromDB(double start, double end)
{
    long startCode = 1000 * start;
    long endCode = 1000 * end;
    TString sql = TString::Format("select sum(LiveTime) from t_service_satellite where  time_code between %ld and %ld and is_in_saa = 0",
                                  startCode, endCode);
    //DmpLogInfo << sql <<DmpLogEndl;
    fResult = fServer->Query(sql.Data());
    fRows = fResult->Next();
    TString LiveTimeStr(fRows->GetField(0));
    return LiveTimeStr.Atof() * 1e-3;
}
double DmpSvcLiveTime::GetLiveTimeFromBin(double start, double end)
{
    return 0.0;
}
bool DmpSvcLiveTime::LoadHKTree()
{
    if(fIsLoadedHKData)
    {
        return true;
    }
    TTree* hkTree = (TTree*) gIOSvc->GetTree("HousekeepingData/PayloadDataProcesser");
    DmpHKDPayloadDataProcesser* processor = new DmpHKDPayloadDataProcesser();
    hkTree->SetBranchAddress("DmpHKDPayloadDataProcessor", &processor);
    for(int i = 0; i < hkTree->GetEntries(); i++)
    {
        hkTree->GetEntry(i);
        long time_s = atoi(processor->GetPayloadDataProcesser().at(0).c_str());
        int time_ms = atoi(processor->GetPayloadDataProcesser().at(1).c_str());
        int effTrgs = atoi(processor->GetPayloadDataProcesser().at(109).c_str());
        int coinTrgs = atoi(processor->GetPayloadDataProcesser().at(110).c_str());
        fTriggerCounts[1000 * time_s + time_ms] = std::make_pair(effTrgs, coinTrgs);
    }
    int lastEff = 0, lastCoin = 0;
    int tempEff = 0, tempCoin = 0;
    long lastTime = 0;
    for(std::map <long, std::pair<int, int> > :: iterator it = fTriggerCounts.begin();
            it != fTriggerCounts.end(); ++it)
    {
        tempEff = it->second.first;
        tempCoin = it->second.second;
        if(it->first - lastTime < 1000 || it->first - lastTime > 5000)
        {
            it->second.first = 0;
            it->second.second = 0;
        }
        else
        {
            it->second.first = (tempEff - lastEff + 0x10000) % 0x10000;
            it->second.second = (tempCoin - lastCoin + 0x10000) % 0x10000;
        }
        lastTime = it->first;
        lastEff = tempEff;
        lastCoin = tempCoin;
    }
    fIsLoadedHKData = true;
    return fIsLoadedHKData;
}
double DmpSvcLiveTime::GetLiveTimeFromHK(double start, double end)
{
    double LiveTime = 0;
    if(! LoadHKTree()) { return -1.0; }
    std::map <long, std::pair<int, int> > :: iterator it = fTriggerCounts.begin();
    std::map <long, std::pair<int, int> > :: iterator startIt = it;
    std::map <long, std::pair<int, int> > :: iterator lastIt = it;
    bool isStartedCalc = false;
    for(; it != fTriggerCounts.end(); ++it)
    {
        if(it->first > end * 1000) { break; }
        if(it->first < start * 1000) { continue; }
        if(! isStartedCalc)
        {
            startIt = it;
            isStartedCalc = true;
        }
        if( it->second.first && it->second.second )
        { LiveTime += ( it->first - lastIt->first - 3072.5 * it->second.first ) * 1e-3; }
        lastIt = it;
    }
    if( it != fTriggerCounts.end() )
    {
        if(it->first - lastIt->first < 4500)
        {
            LiveTime += ( it->first - lastIt->first - 3072.5 * it->second.first  ) * 1e-3
                        * (end * 1000 - lastIt->first) / (it->first - lastIt->first);
        }
    }
    if(startIt != fTriggerCounts.begin())
    {
    }
    return LiveTime;
}
static bool failedConnect = false;
bool DmpSvcLiveTime::ConnectedDB()
{
    if(failedConnect) return false;
    if( fServer && fServer->IsConnected() ) return true;
    fServer = TSQLServer::Connect( "mysql://192.168.1.154/dampesas", "sas", "123456" );
    if(!fServer || !fServer->IsConnected())
    {
        fServer = TSQLServer::Connect( "mysql://172.16.0.2/dampesas", "sas", "123456" );
    }
    if(!fServer || !fServer->IsConnected()) failedConnect = true;
    return fServer && fServer->IsConnected();
}
bool DmpSvcLiveTime::ExistedBinFile()
{
    return false;
}
bool DmpSvcLiveTime::CanReadHKTree()
{
    return gIOSvc->GetTree("");
}
// double DmpSvcLiveTime::GetLiveTime(std::string& filelist)
// {
// return GetLiveTime(filelist.c_str());
// }
double DmpSvcLiveTime::GetLiveTime(const char* txtFilelist)
{
    std::string filenameStr(txtFilelist);
    if(filenameStr.find("~/") == 0)
    {
        filenameStr.replace(0, 1, getenv("HOME"));
    }
    std::ifstream ifFlist(filenameStr.c_str());
    std::vector<std::string> filelist;
    std::string rootFile;
    while(!ifFlist.eof())
    {
        ifFlist >> rootFile;
        if(ifFlist.fail()) { break; }
        filelist.push_back(rootFile);
    }
    //std::cout << filelist.size() << std::endl;
    return GetLiveTime(filelist);
}
double DmpSvcLiveTime::GetLiveTime( std::vector<std::string>& filelist)
{
    if(!fHasLoadFileMap)
    {
        if(!LoadFileStrMap())
        {
            DmpLogError << "Failed to find any way to load livetime from filename.." << DmpLogEndl;
            return -1;
        }
        fHasLoadFileMap = true;
    }
    if(fLivetimeRootFiles.size() == 0)
    {
        DmpLogError << "Cannot link any file info....." << DmpLogEndl;
        return -1;
    }
    double sumTime = 0;
    for(std::vector<std::string> :: iterator it = filelist.begin(); it != filelist.end(); ++it)
    {
        std::string randKeyword = GetRandKeywordFromFilename(*it);
        std::map<std::string, double> :: iterator itfound =  fLivetimeRootFiles.find(randKeyword);
        if( itfound == fLivetimeRootFiles.end() )
        {
            DmpLogWarning << "Cannot find any information from file: " << *it << DmpLogEndl;
            continue;
        }
        sumTime += itfound->second;
    }
    return 1e-3 * sumTime;
}
std::string DmpSvcLiveTime::GetRandKeywordFromFilename( std::string filename)
{
    std::size_t found_sb = filename.rfind("OBS_");
    std::size_t found_pt = filename.find_last_of('.');
    if(found_sb == std::string::npos)
    {
        found_sb = -1;
    }
    if(found_pt == std::string::npos)
    {
        found_pt = filename.length();
    }
    return filename.substr(found_sb + 4, found_pt - found_sb - 4);
}
bool DmpSvcLiveTime::LoadFileStrMap()
{
    if(ConnectedDB())
    {
        TSQLResult* thisRes = fServer->Query("select file_name, live_time from t_service_file_info ");
        TSQLRow* aRow = NULL;
        while( aRow = thisRes->Next() )
        {
            std::string aKeywd = GetRandKeywordFromFilename(std::string(aRow->GetField(0)));
            float aLt = atof(aRow->GetField(1));
            fLivetimeRootFiles.insert(std::make_pair( aKeywd, aLt));
        }
        return true;
    }
    if(!getenv("DMPSWSYS"))
    {
        DmpLogWarning << "Cannot find dmpsw, please check it.. " << DmpLogEndl;
        return false;
    }
    char configurePath[500];
    sprintf(configurePath, "%s/share/Configuration/filelist_livetime.cfg", getenv("DMPSWSYS"));
    std::ifstream ifStrMap(configurePath);
    std::string aRandKeyword;
    double livetime = 0.0;
    while (!ifStrMap.eof())
    {
        ifStrMap >> aRandKeyword >> livetime;
        if(ifStrMap.fail()) { break; }
        fLivetimeRootFiles.insert(std::make_pair( aRandKeyword, livetime ));
    }
    return true;
}
