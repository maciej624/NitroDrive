#include "SaveSystem.hpp"
#include <fstream>
const std::string SaveSystem::FILE="save.dat";
bool SaveSystem::load(){
    std::ifstream f(FILE); if(!f) return false;
    std::string ln;
    while(std::getline(f,ln)){
        if(ln.empty()||ln[0]=='#') continue;
        auto eq=ln.find('='); if(eq==std::string::npos) continue;
        std::string k=ln.substr(0,eq),v=ln.substr(eq+1);
        try{
            if(k=="coins")             data.coins=std::stoi(v);
            if(k=="engineLevel")       data.engineLevel=std::stoi(v);
            if(k=="tireLevel")         data.tireLevel=std::stoi(v);
            if(k=="nitroLevel")        data.nitroLevel=std::stoi(v);
            if(k=="gearboxLevel")      data.gearboxLevel=std::stoi(v);
            if(k=="skinIndex")         data.skinIndex=std::stoi(v);
            if(k=="bestDragTime")      data.bestDragTime=std::stof(v);
            if(k=="bestCircuitLap")    data.bestCircuitLap=std::stof(v);
            if(k=="bestTimeAttackLap") data.bestTimeAttackLap=std::stof(v);
            if(k=="bestDriftScore")    data.bestDriftScore=std::stoi(v);
            if(k=="bestDragDiff")      data.bestDragDiff=std::stoi(v);
            if(k=="bestDriftDiff")     data.bestDriftDiff=std::stoi(v);
            if(k=="bestCircDiff")      data.bestCircuitDiff=std::stoi(v);
            if(k=="difficulty")        data.difficulty=std::stoi(v);
        }catch(...){}
    }
    return true;
}
bool SaveSystem::save(){
    std::ofstream f(FILE); if(!f) return false;
    f<<"# NitroDrive\n";
    f<<"coins="<<data.coins<<"\n";
    f<<"engineLevel="<<data.engineLevel<<"\n";
    f<<"tireLevel="<<data.tireLevel<<"\n";
    f<<"nitroLevel="<<data.nitroLevel<<"\n";
    f<<"gearboxLevel="<<data.gearboxLevel<<"\n";
    f<<"skinIndex="<<data.skinIndex<<"\n";
    f<<"bestDragTime="<<data.bestDragTime<<"\n";
    f<<"bestCircuitLap="<<data.bestCircuitLap<<"\n";
    f<<"bestTimeAttackLap="<<data.bestTimeAttackLap<<"\n";
    f<<"bestDriftScore="<<data.bestDriftScore<<"\n";
    f<<"bestDragDiff="<<data.bestDragDiff<<"\n";
    f<<"bestDriftDiff="<<data.bestDriftDiff<<"\n";
    f<<"bestCircDiff="<<data.bestCircuitDiff<<"\n";
    f<<"difficulty="<<data.difficulty<<"\n";
    return true;
}
