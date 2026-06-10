#include "Garage.hpp"
constexpr int Garage::ENG_C[5],Garage::TIR_C[5],Garage::NIT_C[5],Garage::GBX_C[3];

Garage::Garage(){}
bool Garage::init(const sf::Font& f,const sf::Texture& bg,
                  const std::array<sf::Texture,8>& sk){
    m_font=f; m_bgTex=bg; m_bgSpr.setTexture(m_bgTex); m_skins=sk; return true;
}
sf::Text Garage::T(const std::string& s,unsigned sz,sf::Color c)const{
    sf::Text t; t.setFont(m_font); t.setString(s); t.setCharacterSize(sz);
    t.setFillColor(c); t.setOutlineColor(sf::Color::Black); t.setOutlineThickness(1.f);
    return t;
}
void Garage::syncFromSave(const SaveData& sd){
    m_eng=sd.engineLevel; m_tire=sd.tireLevel; m_nit=sd.nitroLevel;
    m_gear=sd.gearboxLevel; m_skin=sd.skinIndex; m_coins=sd.coins;
}
void Garage::syncToSave(SaveData& sd)const{
    sd.engineLevel=m_eng; sd.tireLevel=m_tire; sd.nitroLevel=m_nit;
    sd.gearboxLevel=m_gear; sd.skinIndex=m_skin; sd.coins=m_coins;
}
// Przeliczanie poziomów ulepszeń na faktyczne statystyki samochodu w grze
CarStats Garage::buildStats()const{
    CarStats s;
    s.maxSpeed     = 180.f + m_eng * 22.f;
    s.acceleration = 90.f  + m_eng * 14.f;
    s.grip         = 0.55f + m_tire * 0.09f;
    s.nitroCapacity= 80.f  + m_nit * 25.f;
    s.skinIndex    = m_skin;
    return s;
}
bool Garage::update(float){ bool r=m_back; m_back=false; return r; }
void Garage::handleEvent(const sf::Event& e,sf::RenderWindow&){
    m_click=false;
    if(e.type==sf::Event::MouseMoved) m_mouse={(float)e.mouseMove.x,(float)e.mouseMove.y};
    if(e.type==sf::Event::MouseButtonPressed&&e.mouseButton.button==sf::Mouse::Left){
        m_mouse={(float)e.mouseButton.x,(float)e.mouseButton.y}; m_click=true;
    }
    if(e.type==sf::Event::KeyPressed&&e.key.code==sf::Keyboard::Escape) m_back=true;
}
bool Garage::Btn(sf::RenderTarget& t,float x,float y,float w,float h,
                  const std::string& lbl,bool bought,bool afford){
    bool hov=sf::FloatRect(x,y,w,h).contains(m_mouse);
    sf::Color fc=bought?sf::Color{30,140,30,240}:
                  !afford?sf::Color{50,50,50,180}:
                  hov?sf::Color{0,180,255,240}:sf::Color{22,25,55,220};
    sf::RectangleShape r({w,h}); r.setPosition(x,y); r.setFillColor(fc);
    sf::Color oc=bought?sf::Color{50,200,50,200}:hov?sf::Color{0,200,255,220}:sf::Color{70,80,140,200};
    r.setOutlineColor(oc); r.setOutlineThickness(2.f); t.draw(r);
    // Accent line
    sf::RectangleShape ac({w-4,2}); ac.setPosition(x+2,y+2);
    ac.setFillColor({255,255,255,20}); t.draw(ac);
    auto tx=T(lbl,16,sf::Color::White); tx.setPosition(x+10,y+9); t.draw(tx);
    return hov&&m_click&&!bought&&afford;
}

void Garage::drawStats(sf::RenderTarget& t,float x,float y){
    struct Stat{ const char* name; float val; float max; std::string label; };
    float spd=180.f+m_eng*22.f, acc=90.f+m_eng*14.f;
    float grp=(0.55f+m_tire*0.09f), nit=80.f+m_nit*25.f;
    int gbx=m_gear;
    Stat stats[4]={
        {"Top Speed",   spd,   290.f, std::to_string((int)spd)+" km/h"},
        {"Acceleration",acc,   200.f, std::to_string((int)acc)},
        {"Grip",        grp*100,100.f,std::to_string((int)(grp*100))+"%"},
        {"Nitro Tank",  nit,   205.f, std::to_string((int)nit)+" L"}
    };
    for(int i=0;i<4;i++){
        auto lb=T(stats[i].name,14,{180,200,230,255}); lb.setPosition(x,y+i*38); t.draw(lb);
        sf::RectangleShape bg({220,14}); bg.setPosition(x+130,y+i*38+3);
        bg.setFillColor({15,15,25,200}); bg.setOutlineColor({60,70,120,180});
        bg.setOutlineThickness(1); t.draw(bg);
        float frac=std::min(1.f,stats[i].val/stats[i].max);
        if(frac>0){
            sf::RectangleShape fl({220*frac,14}); fl.setPosition(x+130,y+i*38+3);
            uint8_t rr=(uint8_t)(50+frac*200); uint8_t gg=(uint8_t)(200-frac*80);
            fl.setFillColor({rr,gg,40,220}); t.draw(fl);
        }
        auto vt=T(stats[i].label,12,{160,220,160,255}); vt.setPosition(x+356,y+i*38+3); t.draw(vt);
    }
    auto gt=T("Gearbox: Lv"+std::to_string(gbx+1)+"/4",14,{180,200,230,255});
    gt.setPosition(x,y+4*38+4); t.draw(gt);
}

void Garage::draw(sf::RenderTarget& t){
    t.draw(m_bgSpr);

    sf::RectangleShape ov({1280,720}); ov.setFillColor({0,0,0,160}); t.draw(ov);

    // Header bar
    sf::RectangleShape hbar({1280,56}); hbar.setPosition(0,0);
    hbar.setFillColor({12,14,32,240});
    hbar.setOutlineColor({0,140,255,120}); hbar.setOutlineThickness(0); t.draw(hbar);
    sf::RectangleShape hline({1280,2}); hline.setPosition(0,56);
    hline.setFillColor({0,160,255,100}); t.draw(hline);

    auto ti=T("GARAGE",40,sf::Color::White); ti.setPosition(30,8); t.draw(ti);
    // Coin display
    sf::RectangleShape coinBg({180,40}); coinBg.setPosition(1090,8);
    coinBg.setFillColor({20,18,8,200}); coinBg.setOutlineColor({200,170,0,150});
    coinBg.setOutlineThickness(1.5f); t.draw(coinBg);
    auto co=T("$ "+std::to_string(m_coins),24,{255,215,0,255}); co.setPosition(1100,13); t.draw(co);

    // Tabs
    struct TabInfo{ const char* name; sf::Color col; };
    TabInfo tabs[5]={
        {"ENGINE",     {200,70,20,255}},
        {"SUSPENSION", {20,120,200,255}},
        {"NITRO",      {0,180,100,255}},
        {"GEARBOX",    {180,120,0,255}},
        {"PAINT",      {130,40,180,255}}
    };
    for(int i=0;i<5;i++){
        bool sel=(m_tab==i);
        sf::RectangleShape tb({148,36}); tb.setPosition(30.f+i*152,64);
        sf::Color c=tabs[i].col; c.a=sel?240:130;
        tb.setFillColor(c);
        tb.setOutlineColor(sel?sf::Color::White:sf::Color{80,80,80,120});
        tb.setOutlineThickness(sel?2.f:1.f); t.draw(tb);
        if(sel){

            sf::RectangleShape ul({148,3}); ul.setPosition(30.f+i*152,98);
            ul.setFillColor(tabs[i].col); t.draw(ul);
        }
        auto tl=T(tabs[i].name,13,sel?sf::Color::White:sf::Color{200,200,200,180});
        tl.setPosition(36.f+i*152,72); t.draw(tl);
        if(m_click&&tb.getGlobalBounds().contains(m_mouse)) m_tab=i;
    }


    sf::RectangleShape spanel({400,210}); spanel.setPosition(26,104);
    spanel.setFillColor({10,12,28,200}); spanel.setOutlineColor({50,60,120,180});
    spanel.setOutlineThickness(1.5f); t.draw(spanel);
    drawStats(t,34,112);

    float cx=34,cy=328;

    if(m_tab==0){
        auto desc=T("Bigger engine = higher top speed and better acceleration",13,{140,180,240,200});
        desc.setPosition(cx,cy-26); t.draw(desc);
        const char* names[5]={"Stock 1.6T","Engine 2.0T","Engine 2.5T Turbo","V6 3.0","V8 4.0"};
        for(int i=0;i<5;i++){
            bool bought=(m_eng>i),aff=(m_coins>=ENG_C[i]);
            std::string l=bought?"[INSTALLED]":(std::string(names[i])+" -- $"+std::to_string(ENG_C[i]));
            if(Btn(t,cx,cy+i*58,340,44,l,bought,aff||bought)&&m_eng==i){ m_coins-=ENG_C[i]; m_eng++; }
            int spd=180+(i+1)*22, ac=90+(i+1)*14;
            auto ds=T("Speed: "+std::to_string(spd)+"km/h  Acc: "+std::to_string(ac),12,{140,150,200,220});
            ds.setPosition(cx+350,cy+i*58+14); t.draw(ds);
        }
    }
    else if(m_tab==1){
        auto desc=T("Better suspension = more grip and faster cornering",13,{140,180,240,200});
        desc.setPosition(cx,cy-26); t.draw(desc);
        const char* names[5]={"Stock","Sport Springs","Coilovers","Racing Suspension","Full Aero Kit"};
        for(int i=0;i<5;i++){
            bool bought=(m_tire>i),aff=(m_coins>=TIR_C[i]);
            std::string l=bought?"[INSTALLED]":(std::string(names[i])+" -- $"+std::to_string(TIR_C[i]));
            if(Btn(t,cx,cy+i*58,340,44,l,bought,aff||bought)&&m_tire==i){ m_coins-=TIR_C[i]; m_tire++; }
            int grp=(int)((0.55f+(i+1)*0.09f)*100);
            auto ds=T("Grip: "+std::to_string(grp)+"%",12,{140,150,200,220});
            ds.setPosition(cx+350,cy+i*58+14); t.draw(ds);
        }
    }
    else if(m_tab==2){
        auto desc=T("Bigger tank = longer nitro boost (press E)",13,{140,180,240,200});
        desc.setPosition(cx,cy-26); t.draw(desc);
        const char* names[5]={"Small Tank (80L)","Medium Tank (105L)","Large Tank (130L)","Twin NOS (155L)","Race NOS (180L)"};
        for(int i=0;i<5;i++){
            bool bought=(m_nit>i),aff=(m_coins>=NIT_C[i]);
            std::string l=bought?"[INSTALLED]":(std::string(names[i])+" -- $"+std::to_string(NIT_C[i]));
            if(Btn(t,cx,cy+i*58,340,44,l,bought,aff||bought)&&m_nit==i){ m_coins-=NIT_C[i]; m_nit++; }
            int cap=80+(i+1)*25;
            auto ds=T("Capacity: "+std::to_string(cap)+"L",12,{140,150,200,220});
            ds.setPosition(cx+350,cy+i*58+14); t.draw(ds);
        }
    }
    else if(m_tab==3){
        auto desc=T("Better gearbox = smoother shifts and higher RPM range",13,{140,180,240,200});
        desc.setPosition(cx,cy-26); t.draw(desc);
        const char* names[3]={"Sport 6-Speed","Sequential Racing","Full Race Gearbox"};
        const char* efx[3]={"Shift speed +10%","Shift speed +25%, no lag","Instant shifts, max RPM"};
        for(int i=0;i<3;i++){
            bool bought=(m_gear>i),aff=(m_coins>=GBX_C[i]);
            std::string l=bought?"[INSTALLED]":(std::string(names[i])+" -- $"+std::to_string(GBX_C[i]));
            if(Btn(t,cx,cy+i*58,340,44,l,bought,aff||bought)&&m_gear==i){ m_coins-=GBX_C[i]; m_gear++; }
            auto ds=T(efx[i],12,{140,150,200,220}); ds.setPosition(cx+350,cy+i*58+14); t.draw(ds);
        }
    }
    else{
        auto desc=T("Change your car's color (visual only)",13,{140,180,240,200});
        desc.setPosition(cx,cy-26); t.draw(desc);
        const char* names[8]={
            "Blue Formula (free)","White Formula","Red Formula",
            "Orange Formula","Black Formula","Lime Formula",
            "Pink Formula","Gray Formula"};
        for(int i=0;i<8;i++){
            bool bought=(m_skin==i),aff=(i==0||m_coins>=250);
            std::string l=bought?"[ACTIVE]":(i==0?std::string(names[i]):std::string(names[i])+" -- $250");
            if(Btn(t,cx,cy+i*44,320,38,l,bought,aff)){ if(i>0&&m_skin!=i) m_coins-=250; m_skin=i; }
        }
    }


    if(m_skin<(int)m_skins.size()){

        sf::RectangleShape ppanel({260,180}); ppanel.setPosition(840,460);
        ppanel.setFillColor({10,12,30,200}); ppanel.setOutlineColor({60,80,140,180});
        ppanel.setOutlineThickness(1.5f); t.draw(ppanel);
        sf::Sprite sp; sp.setTexture(m_skins[m_skin]);
        auto b=sp.getLocalBounds(); sp.setOrigin(b.width/2,b.height/2);
        sp.setScale(2.8f,2.8f); sp.setPosition(970,560); t.draw(sp);
        auto la=T("YOUR CAR",14,{140,160,210,200}); la.setPosition(898,468); t.draw(la);
    }

    bool bh=sf::FloatRect(30,666,140,40).contains(m_mouse);
    sf::RectangleShape bb({140,40}); bb.setPosition(30,666);
    bb.setFillColor(bh?sf::Color{200,50,50,240}:sf::Color{120,25,25,200});
    bb.setOutlineColor(bh?sf::Color::White:sf::Color{160,40,40,160});
    bb.setOutlineThickness(1.5f); t.draw(bb);
    auto bl=T("< BACK",18,sf::Color::White); bl.setPosition(46,672); t.draw(bl);
    if(m_click&&bb.getGlobalBounds().contains(m_mouse)) m_back=true;
}
