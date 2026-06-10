#include "Map.hpp"
#include <cmath>
#include <algorithm>

// stała PI do obliczeń kątów
static const float PI=3.14159265f;

// domyślny konstruktor
Map::Map(){}

// dodaje czworokąt (jako dwa trójkąty) do tablicy wierzchołków
void Map::addQ(std::vector<sf::Vertex>& v,
               sf::Vector2f a,sf::Vector2f b,
               sf::Vector2f c,sf::Vector2f d,sf::Color col){
    v.push_back({a,col});v.push_back({b,col});v.push_back({c,col});
    v.push_back({a,col});v.push_back({c,col});v.push_back({d,col});
}

// dodaje prostokąt na podstawie sf::FloatRect
void Map::addR(std::vector<sf::Vertex>& v,sf::FloatRect r,sf::Color col){
    addQ(v,{r.left,r.top},{r.left+r.width,r.top},
         {r.left+r.width,r.top+r.height},{r.left,r.top+r.height},col);
}

// rysuje segment toru o zadanej szerokości
void Map::addSeg(std::vector<sf::Vertex>& v,
                 sf::Vector2f a,sf::Vector2f b,float hw,sf::Color col){
    sf::Vector2f d=b-a; float len=std::hypot(d.x,d.y);
    if(len<0.1f) return;
    sf::Vector2f n={-d.y/len,d.x/len};
    addQ(v,a+n*hw,b+n*hw,b-n*hw,a-n*hw,col);
}

// sprawdza czy auto znajduje się w granicach asfaltu
bool Map::isOnTrack(sf::Vector2f pos) const {
    // osobna logika dla dragu bo to prostokąt
    if(m_type==MapType::DRAG){
        return pos.x>=0 && pos.x<=m_size.x
               && pos.y>=160.f && pos.y<=560.f;
    }

    // dla reszty map sprawdzamy odległość od segmentów toru
    float hw=m_trackHW+4.f;
    for(auto& seg:m_trackSegs){
        sf::Vector2f a=seg.first, b=seg.second;
        sf::Vector2f d=b-a;
        float len2=d.x*d.x+d.y*d.y;
        if(len2<0.01f) continue;

        float t=((pos.x-a.x)*d.x+(pos.y-a.y)*d.y)/len2;
        t=std::clamp(t,0.f,1.f);
        sf::Vector2f closest={a.x+t*d.x, a.y+t*d.y};
        float dx=pos.x-closest.x, dy=pos.y-closest.y;

        if(dx*dx+dy*dy<=hw*hw) return true;
    }
    return false;
}

// generuje prostą trasę do wyścigów równoległych
void Map::generateDrag(){
    m_size={3200,720}; m_bounds={0,0,3200,720};
    m_trackHW=200.f;

    // tło poza torem
    addR(m_v,{0,0,3200,720},{22,22,45});

    // detale otoczenia i trybuny
    for(float x=0;x<3200;x+=280){
        addR(m_v,{x,0,260,132},{44,44,62});
        addR(m_v,{x,588,260,132},{44,44,62});
        for(int r=0;r<5;r++){
            sf::Color rc{(uint8_t)(58+r*10),(uint8_t)(58+r*10),(uint8_t)(78+r*8)};
            addR(m_v,{x+2,4.f+r*25,256,22},rc);
            addR(m_v,{x+2,590.f+r*25,256,22},rc);
        }
    }

    // trawiaste pasy i główny asfalt
    addR(m_v,{0,132,3200,28},{40,115,40});
    addR(m_v,{0,560,3200,28},{40,115,40});
    addR(m_v,{0,160,3200,400},{56,56,63});

    // czerwono-białe bandy na krawędziach
    for(float x=0;x<3200;x+=60){
        sf::Color k=(int(x/60)%2==0)?sf::Color{215,45,45}:sf::Color{235,235,235};
        addR(m_v,{x,160,60,18},k);
        addR(m_v,{x,542,60,18},k);
    }

    // linie rozdzielające pasy
    for(int ln=1;ln<4;ln++){
        float ly=160.f+ln*(400.f/4.f);
        for(float x=0;x<3200;x+=120) addR(m_v,{x,ly-3,72,6},{185,180,125});
    }

    // szachownice na starcie i mecie
    for(int r=0;r<8;r++) for(int c=0;c<3;c++){
            sf::Color sc=((r+c)%2==0)?sf::Color::White:sf::Color::Black;
            addR(m_v,{180.f+c*18,180.f+r*50,18,50},sc);
        }
    float fx=3040;
    for(int r=0;r<8;r++) for(int c=0;c<3;c++){
            sf::Color sc=((r+c)%2==0)?sf::Color::White:sf::Color::Black;
            addR(m_v,{fx+c*18,180.f+r*50,18,50},sc);
        }

    // waypointy główne do ogólnej nawigacji
    float wy=360;
    for(float x=200;x<3200;x+=150) m_waypoints.push_back({x,wy});

    // oddzielne waypointy dla każdego pasa (żeby boty trzymały się swojej linii)
    float laneYs[5]={220.f,300.f,380.f,460.f,540.f};
    for(int ln=0;ln<5;ln++){
        m_laneWaypoints[ln].clear();
        for(float x=200;x<3200;x+=150)
            m_laneWaypoints[ln].push_back({x, laneYs[ln]});
    }

    // ustawienie gracza i botów na starcie
    m_startPositions.clear();
    m_startPositions.push_back({200.f, laneYs[2]});
    int botLanes[4]={0,1,3,4};
    for(int i=0;i<4;i++)
        m_startPositions.push_back({130.f, laneYs[botLanes[i]]});
    m_finishZone={fx-10,160,30,400};
}

// generuje zamkniętą pętlę z zakrętami do driftowania
void Map::generateDrift(){
    m_size={2400,1400}; m_bounds={0,0,2400,1400};
    m_trackHW=95.f;
    addR(m_v,{0,0,2400,1400},{20,58,20});

    // funkcja pomocnicza do wyliczania punktów na łuku (zakręcie)
    auto arcPts=[&](float cx,float cy,float rx,float ry,
                      float a0,float a1,int s=24)->std::vector<sf::Vector2f>{
        std::vector<sf::Vector2f> p;
        for(int i=0;i<=s;i++){
            float t=a0+(a1-a0)*i/s, r=t*PI/180.f;
            p.push_back({cx+rx*std::cos(r),cy+ry*std::sin(r)});
        }
        return p;
    };

    float M=180,HW=95,RX=225,RY=185;
    float lx=M+RX,rx=2400-M-RX,ty=M+RY,by=1400-M-RY;

    // łączenie prostych i zakrętów w jeden pełny tor
    std::vector<sf::Vector2f> track;
    auto app=[&](std::vector<sf::Vector2f> v){ for(auto& p:v) track.push_back(p); };
    app(arcPts(lx,ty,RX,RY,180,270,24));
    app(arcPts(rx,ty,RX,RY,270,360,24));
    app(arcPts(rx,by,RX,RY,0,90,24));
    app(arcPts(lx,by,RX,RY,90,180,24));
    track.push_back(track[0]);

    // rysowanie fizycznego toru i tarek
    for(size_t i=0;i+1<track.size();i++){
        addSeg(m_v,track[i],track[i+1],HW+45,{148,142,78}); // pobocze
        addSeg(m_v,track[i],track[i+1],HW,{56,56,63});      // asfalt
        m_trackSegs.push_back({track[i],track[i+1]});

        // krawężniki
        sf::Vector2f dd=track[i+1]-track[i];
        float len=std::hypot(dd.x,dd.y); if(len<0.1f) continue;
        sf::Vector2f n={-dd.y/len,dd.x/len};
        sf::Color ka=(i%2==0)?sf::Color{215,45,45}:sf::Color{235,235,235};
        sf::Color kb=(i%2==0)?sf::Color{235,235,235}:sf::Color{215,45,45};
        addQ(m_v,track[i]+n*HW,track[i+1]+n*HW,
             track[i+1]+n*(HW-16),track[i]+n*(HW-16),ka);
        addQ(m_v,track[i]-n*(HW-16),track[i+1]-n*(HW-16),
             track[i+1]-n*HW,track[i]-n*HW,kb);
    }

    // linia środkowa
    for(size_t i=0;i+1<track.size();i+=2)
        addSeg(m_v,track[i],track[i+1],2.f,{182,176,115});

    // trasa dla botów
    m_waypoints.assign(track.begin(),track.end()-1);

    // układanie aut na starcie obok siebie
    {
        sf::Vector2f s0=track[0],s1=track[1];
        sf::Vector2f dd=s1-s0; float len=std::hypot(dd.x,dd.y);
        sf::Vector2f fw={dd.x/len,dd.y/len};
        sf::Vector2f nr={-fw.y,fw.x};
        m_startPositions.clear();
        for(int i=0;i<5;i++){
            float lat=(i%2==0)?-28.f:28.f;
            float lon=-(i/2)*80.f;
            m_startPositions.push_back(s0+nr*lat+fw*lon);
        }
    }
    m_finishZone=sf::FloatRect(track[0].x-40,track[0].y-40,80,80);
}

// generuje rozbudowany klasyczny tor wyścigowy
void Map::generateCircuit(){
    m_size={2400,1200}; m_bounds={0,0,2400,1200};
    float HW=55.f;
    m_trackHW=HW;

    // rysowanie bazy pod trawę z paskami dla lepszego wyglądu
    addR(m_v,{0,0,2400,1200},{18,52,18});
    for(int x=0;x<2400;x+=120)
        addR(m_v,{(float)x,0,60,1200},{22,58,22,180});

    float rx=880.f, ry=420.f;
    float cx=1200.f, cy=600.f;

    // tworzenie głównej elipsy toru
    std::vector<sf::Vector2f> track;
    int segs=80;
    for(int i=0;i<segs;i++){
        float t=(float)i/segs * 2.f * PI;
        track.push_back({cx+rx*std::cos(t), cy+ry*std::sin(t)});
    }
    track.push_back(track[0]);

    // zapisanie krawędzi kolizyjnych
    for(size_t i=0;i+1<track.size();i++)
        m_trackSegs.push_back({track[i],track[i+1]});

    // dodawanie band ochronnych wokół toru
    for(size_t i=0;i+1<track.size();i+=3){
        sf::Vector2f p1=track[i],p2=track[i+1];
        sf::Vector2f dd=p2-p1; float len=std::hypot(dd.x,dd.y);
        if(len<0.1f) continue;
        sf::Vector2f n={-dd.y/len,dd.x/len};

        // strefy zderzeniowe na zewnątrz (często jako żółte opony)
        float bOff=HW+30.f;
        sf::Color barrier1=(i%6<3)?sf::Color{40,40,40}:sf::Color{220,180,0};
        addQ(m_v, p1+n*bOff, p2+n*bOff, p2+n*(bOff+12), p1+n*(bOff+12), barrier1);

        // bandy po wewnętrznej stronie
        sf::Color barrier2=(i%6<3)?sf::Color{220,50,50}:sf::Color{235,235,235};
        addQ(m_v, p1-n*(bOff+12), p2-n*(bOff+12), p2-n*bOff, p1-n*bOff, barrier2);
    }

    // strefy wyjazdowe i główny pas ruchu
    for(size_t i=0;i+1<track.size();i++)
        addSeg(m_v,track[i],track[i+1],HW+22,{148,142,78}); // piasek
    for(size_t i=0;i+1<track.size();i++)
        addSeg(m_v,track[i],track[i+1],HW,{52,52,60});      // asfalt

    // tarki na łukach
    for(size_t i=0;i+1<track.size();i++){
        sf::Vector2f p1=track[i],p2=track[i+1];
        sf::Vector2f dd=p2-p1; float len=std::hypot(dd.x,dd.y);
        if(len<0.1f) continue;
        sf::Vector2f n={-dd.y/len,dd.x/len};
        sf::Color ka=(i%2==0)?sf::Color{215,45,45}:sf::Color{235,235,235};
        sf::Color kb=(i%2==0)?sf::Color{235,235,235}:sf::Color{215,45,45};
        addQ(m_v,p1+n*HW,p2+n*HW,p2+n*(HW-12),p1+n*(HW-12),ka);
        addQ(m_v,p1-n*(HW-12),p2-n*(HW-12),p2-n*HW,p1-n*HW,kb);
    }

    // naklejanie przerywanej linii na środek drogi
    for(size_t i=0;i+1<track.size();i+=2)
        addSeg(m_v,track[i],track[i+1],2.f,{182,176,115});

    // budowa widowni wzdłuż prostej startowej
    {
        sf::Vector2f s0=track[0],s1=track[1];
        sf::Vector2f dd=s1-s0; float len=std::hypot(dd.x,dd.y);
        sf::Vector2f n={-(s1.y-s0.y)/len,(s1.x-s0.x)/len};
        sf::Vector2f fw={dd.x/len,dd.y/len};

        for(int sec=0;sec<4;sec++){
            sf::Vector2f base = s0 + n*(HW+50) + fw*(sec*210.f - 420.f);

            // betonowa wylewka
            addQ(m_v, base, base+fw*195.f, base+fw*195.f+n*110.f, base+n*110.f, {55,55,75});

            // siedzenia dla widzów
            for(int row=0;row<5;row++){
                sf::Color seatColor = (row%2==0) ? sf::Color{180,40,40} : sf::Color{220,220,220};
                addQ(m_v,
                     base+n*(15.f+row*18.f)+fw*5.f,
                     base+n*(15.f+row*18.f)+fw*190.f,
                     base+n*(28.f+row*18.f)+fw*190.f,
                     base+n*(28.f+row*18.f)+fw*5.f,
                     seatColor);
            }
        }
    }

    // zjazdy do boksów i stanowiska serwisowe
    {
        sf::Vector2f s0=track[0],s1=track[1];
        sf::Vector2f dd=s1-s0; float len=std::hypot(dd.x,dd.y);
        sf::Vector2f n={-(s1.y-s0.y)/len,(s1.x-s0.x)/len};
        sf::Vector2f fw={dd.x/len,dd.y/len};

        sf::Vector2f pitStart = s0 - n*(HW+5.f) - fw*320.f;

        // asfalt pit lane
        addQ(m_v, pitStart, pitStart+fw*500.f, pitStart+fw*500.f - n*40.f, pitStart - n*40.f, sf::Color{45,45,55});

        // garaże
        for(int box=0;box<4;box++){
            addQ(m_v,
                 pitStart+fw*(box*125.f+2.f),
                 pitStart+fw*(box*125.f+4.f),
                 pitStart+fw*(box*125.f+4.f)-n*38.f,
                 pitStart+fw*(box*125.f+2.f)-n*38.f,
                 sf::Color{220,220,220,200});
        }

        // czerwone oznaczenie
        addR(m_v,{pitStart.x+10,pitStart.y+8,60,8},{220,60,60});
    }

    // ozdobne drzewa rozsiane po mapie
    struct TreePos{ float x,y; };
    static const TreePos trees[]={
                                    // drzewa na obrzeżach
                                    {320,200},{320,1000},{2080,200},{2080,1000},
                                    {640,100},{960,100},{1280,100},{1600,100},{1920,100},
                                    {640,1100},{960,1100},{1280,1100},{1600,1100},{1920,1100},
                                    {100,400},{100,600},{100,800},
                                    {2300,400},{2300,600},{2300,800},
                                    // drzewa wewnątrz toru
                                    {1200,480},{1200,720},{1050,600},{1350,600},
                                    {1000,530},{1400,530},{1000,670},{1400,670},
                                    };
    for(auto& tp : trees){
        sf::Vector2f tc={tp.x,tp.y};

        // cienie drzew
        addQ(m_v, tc+sf::Vector2f(-14,8), tc+sf::Vector2f(14,8),
             tc+sf::Vector2f(14,22), tc+sf::Vector2f(-14,22),
             sf::Color{0,30,0,80});

        // pnie
        addQ(m_v, tc+sf::Vector2f(-5,4), tc+sf::Vector2f(5,4),
             tc+sf::Vector2f(5,18), tc+sf::Vector2f(-5,18),
             sf::Color{100,65,30});

        // trójwarstwowe korony z liści
        addQ(m_v, tc+sf::Vector2f(-22,-10), tc+sf::Vector2f(22,-10),
             tc+sf::Vector2f(22,8), tc+sf::Vector2f(-22,8), sf::Color{25,100,25});
        addQ(m_v, tc+sf::Vector2f(-17,-22), tc+sf::Vector2f(17,-22),
             tc+sf::Vector2f(17,-8), tc+sf::Vector2f(-17,-8), sf::Color{35,130,35});
        addQ(m_v, tc+sf::Vector2f(-10,-32), tc+sf::Vector2f(10,-32),
             tc+sf::Vector2f(10,-18), tc+sf::Vector2f(-10,-18), sf::Color{50,160,50});
    }

    // grafika mety w kratkę
    {
        sf::Vector2f p1=track[0],p2=track[1];
        sf::Vector2f dd=p2-p1; float len=std::hypot(dd.x,dd.y);
        if(len>0){
            sf::Vector2f n={-dd.y/len,dd.x/len},fw={dd.x/len,dd.y/len};
            float sq=HW*2/8.f;
            for(int r=0;r<8;r++) for(int c=0;c<3;c++){
                    sf::Color fc=((r+c)%2==0)?sf::Color::White:sf::Color::Black;
                    float r0=HW-r*sq,r1=HW-(r+1)*sq;
                    addQ(m_v,p1+n*r0,p1+n*r1,
                         p1+n*r1+fw*22.f,p1+n*r0+fw*22.f,fc);
                }
            float fx=p1.x, fy=p1.y;
            m_finishZone={fx-HW, fy-36, HW*2, 72};
        }
    }

    // czyszczenie i ładowanie nowej trasy dla AI
    m_waypoints.clear();
    for(size_t i=0;i<(size_t)segs;i+=2)
        m_waypoints.push_back(track[i]);

    // ustawienie uczestników na odpowiednich polach
    {
        sf::Vector2f s0=track[0],s1=track[1];
        sf::Vector2f dd=s1-s0; float len=std::hypot(dd.x,dd.y);
        sf::Vector2f fw={dd.x/len,dd.y/len},nr={-fw.y,fw.x};
        m_startPositions.clear();
        for(int i=0;i<5;i++){
            float lat=(i%2==0)?-35.f:35.f;
            float lon=-(i/2)*110.f - 80.f;
            m_startPositions.push_back(s0+nr*lat+fw*lon);
        }
    }
}

// główna funkcja do zarządzania tworzeniem planszy
void Map::generate(MapType type,unsigned){
    // resetowanie starych danych
    m_type=type; m_v.clear(); m_waypoints.clear(); m_startPositions.clear();
    m_trackSegs.clear();

    // wybór wariantu i wywołanie odpowiedniego generatora
    switch(type){
    case MapType::DRAG:    generateDrag();    break;
    case MapType::DRIFT:   generateDrift();   break;
    case MapType::CIRCUIT: generateCircuit(); break;
    }

    // ładowanie tekstury jeśli taka istnieje jako podkład pod geometrię
    {
        const char* bgPath=(m_type==MapType::DRAG)?"assets/maps/map_drag.png":"assets/maps/map_circuit.png";
        m_bgLoaded=m_bgTex.loadFromFile(bgPath);
        if(m_bgLoaded){
            m_bgTex.setSmooth(false);
            m_bgSpr.setTexture(m_bgTex);
            auto ts=m_bgTex.getSize();
            m_bgSpr.setScale(m_size.x/(float)ts.x,m_size.y/(float)ts.y);
        }
    }
}

// wyświeta mapę na ekranie
void Map::draw(sf::RenderTarget& tgt,sf::RenderStates st){
    if(m_bgLoaded)
        tgt.draw(m_bgSpr,st);
    if(!m_v.empty()) tgt.draw(m_v.data(),m_v.size(),sf::Triangles,st);
}
