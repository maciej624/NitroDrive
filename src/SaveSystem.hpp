#pragma once
#include <string>
//przechowuje dane stanu gracza
struct SaveData {
    int coins=0,engineLevel=0,tireLevel=0,nitroLevel=0,gearboxLevel=0,skinIndex=0;
    float bestDragTime=0,bestCircuitLap=0,bestTimeAttackLap=0;
    int bestDriftScore=0,bestDragDiff=0,bestDriftDiff=0,bestCircuitDiff=0;
    int difficulty=1;
};
//klasa obsluguje zapis i odczyt stanu gry z plikow
class SaveSystem {
public:
    SaveSystem(){ reset(); }
    bool load();
    bool save();
    void reset(){ data=SaveData{}; }
    SaveData data;
private:
    static const std::string FILE;
};
