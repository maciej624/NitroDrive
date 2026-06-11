#include "Game.hpp"
#include <filesystem>
#include <cmath>
namespace fs=std::filesystem;

// Funkcje pomocnicze

// Zamienia sekundy na format MM:SS.ss
std::string Game::fmtT(float s){
    int m=(int)(s/60); float sc=s-m*60;
    char buf[32]; snprintf(buf,32,"%02d:%05.2f",m,sc);
    return buf;
}

// Zwraca sformatowany tekst
sf::Text Game::T(const std::string& s,unsigned sz,sf::Color c){
    sf::Text t; t.setFont(m_font); t.setString(s);
    t.setCharacterSize(sz); t.setFillColor(c);
    t.setOutlineColor(sf::Color::Black); t.setOutlineThickness(1.5f);
    return t;
}

// Obsługa i rysowanie klikalnych przycisków
bool Game::Btn(sf::RenderTarget& tgt,float x,float y,float w,float h,
               const std::string& lbl,bool sel){
    bool hov=sf::FloatRect(x,y,w,h).contains(m_mouse);
    sf::RectangleShape r({w,h}); r.setPosition(x,y);
    sf::Color fc=sel?sf::Color{0,160,255,230}:
                       hov?sf::Color{40,45,90,240}:sf::Color{18,20,45,220};
    r.setFillColor(fc);
    sf::Color oc=sel?sf::Color::Cyan:hov?sf::Color{0,200,255,220}:sf::Color{60,70,130,200};
    r.setOutlineColor(oc); r.setOutlineThickness(2.f); tgt.draw(r);

    // Górny pasek podkreślający przycisk po najechaniu
    if(hov||sel){
        sf::RectangleShape ac({w,2}); ac.setPosition(x,y);
        ac.setFillColor(sel?sf::Color::Cyan:sf::Color{0,200,255,180}); tgt.draw(ac);
    }
    auto tx=T(lbl,22,sf::Color::White);
    auto b=tx.getLocalBounds();
    tx.setPosition(x+(w-b.width)/2.f-b.left,y+(h-b.height)/2.f-b.top);
    tgt.draw(tx);
    return hov&&m_click;
}

// Wspólne tło z animowaną siatką i dolną poświatą
void Game::drawBg(sf::RenderTarget& t){
    sf::RectangleShape bg({1280,720}); bg.setFillColor({6,7,18}); t.draw(bg);

    // Przesuwanie siatki z czasem
    float ox=std::fmod(m_menuTime*30.f,64.f);
    float oy=std::fmod(m_menuTime*18.f,64.f);
    sf::VertexArray ln(sf::Lines);
    for(int x=-1;x<=21;x++){
        float px=(float)x*64.f-ox;
        ln.append({{px,0},{28,32,70,50}}); ln.append({{px,720},{28,32,70,50}});
    }
    for(int y=-1;y<=13;y++){
        float py=(float)y*64.f-oy;
        ln.append({{0,py},{28,32,70,50}}); ln.append({{1280,py},{28,32,70,50}});
    }
    t.draw(ln);

    // Dolne niebieskie światło
    sf::VertexArray glow(sf::Quads,4);
    glow[0].position={0,620};   glow[0].color={0,0,0,0};
    glow[1].position={1280,620};glow[1].color={0,0,0,0};
    glow[2].position={1280,720};glow[2].color={0,80,180,60};
    glow[3].position={0,720};   glow[3].color={0,80,180,60};
    t.draw(glow);
}

// Główny konstruktor gry
Game::Game()
    :m_win(sf::VideoMode(1280,720),"Nitro Drive",sf::Style::Titlebar|sf::Style::Close)
{
    m_win.setFramerateLimit(60);
    m_save.load();
    buildAssets();

    // Inicjalizacja pojazdu gracza
    m_playerOwned = std::make_unique<Car>(m_playerTex);
    m_player = m_playerOwned.get();
    m_player->applyStats(m_garage.buildStats());

    // Generowanie botów z różnymi kolorami
    sf::Color bc[4]={{200,40,40},{40,190,40},{220,190,0},{140,40,210}};
    for(int i=0;i<4;i++){
        auto b=std::make_unique<Bot>(m_botTex[i],bc[i]);
        b->setBotId(i);
        m_bots.push_back(std::move(b));
    }

    // Łączenie gracza i botów w jeden kontener
    // Ułatwia to pętle, później rozróżniamy je za pomocą dynamic_cast
    m_cars.clear();
    m_cars.push_back(m_player);
    for(auto& b : m_bots) m_cars.push_back(b.get());

    m_garage.init(m_font,m_garageBg,m_skinTex);
    m_garage.syncFromSave(m_save.data);
}

// Wczytywanie zasobów (grafiki, czcionki) i tworzenie tekstury garażu
void Game::buildAssets(){
    fs::create_directories("assets/fonts");
    if(!m_font.loadFromFile("assets/fonts/racing_font.ttf"))
        m_font.loadFromFile("C:/Windows/Fonts/arial.ttf");

    // Skórki dla pojazdów
    static const char* skinFiles[8]={
        "assets/cars/car_formula_1.png",  // 0: niebieski (domyślny)
        "assets/cars/car_formula_9.png",  // 1: biały
        "assets/cars/car_formula_8.png",  // 2: czerwony
        "assets/cars/car_formula_6.png",  // 3: pomarańczowy
        "assets/cars/car_formula_0.png",  // 4: czarny
        "assets/cars/car_formula_4.png",  // 5: limonkowy
        "assets/cars/car_formula_7.png",  // 6: różowy
        "assets/cars/car_formula_2.png",  // 7: szary
    };

    // Konkretne tekstury przypisane do botów
    static const char* botFiles[4]={
        "assets/cars/car_formula_3.png",  // bot0: zielony
        "assets/cars/car_formula_8.png",  // bot1: czerwony
        "assets/cars/car_formula_6.png",  // bot2: pomarańczowy
        "assets/cars/car_formula_5.png",  // bot3: granatowy
    };

    // Wczytanie tekstury gracza na podstawie zapisu
    int si=std::clamp(m_save.data.skinIndex,0,7);
    if(!m_playerTex.loadFromFile(skinFiles[si]))
        m_playerTex.loadFromFile(skinFiles[0]);
    m_playerTex.setSmooth(false);

    // Wczytywanie grafik botów
    for(int i=0;i<4;i++){
        if(!m_botTex[i].loadFromFile(botFiles[i]))
            m_botTex[i]=m_playerTex;
        m_botTex[i].setSmooth(false);
    }

    // Wczytanie wszystkich skinów dla menu garażu
    for(int i=0;i<8;i++){
        if(!m_skinTex[i].loadFromFile(skinFiles[i]))
            m_skinTex[i]=m_playerTex;
        m_skinTex[i].setSmooth(false);
    }

    // Generowanie tła garażu (zwykła statyczna siatka)
    sf::RenderTexture rt; rt.create(1280,720); rt.clear({10,10,20});
    sf::VertexArray gln(sf::Lines);
    for(int x=0;x<1280;x+=64){gln.append({{(float)x,0},{30,30,55,70}});gln.append({{(float)x,720},{30,30,55,70}});}
    for(int y=0;y<720;y+=64){gln.append({{0,(float)y},{30,30,55,70}});gln.append({{1280,(float)y},{30,30,55,70}});}
    rt.draw(gln); rt.display(); m_garageBg=rt.getTexture();
}

// Główna pętla programu
void Game::run(){
    while(m_win.isOpen()){
        float dt=m_clk.restart().asSeconds();
        dt=std::min(dt,0.05f); // Zabezpieczenie przed zbyt dużą deltą
        if(m_state==GS::MENU||m_state==GS::MODE||m_state==GS::DIFF) m_menuTime+=dt;
        processEvents(); update(dt); render();
    }
}

// Obsługa okna i wejścia użytkownika
void Game::processEvents(){
    m_click=false;
    sf::Event e;
    while(m_win.pollEvent(e)){
        if(e.type==sf::Event::Closed) m_win.close();
        if(e.type==sf::Event::KeyPressed&&e.key.code==sf::Keyboard::F11) toggleFs();
        if(e.type==sf::Event::MouseMoved) m_mouse={(float)e.mouseMove.x,(float)e.mouseMove.y};
        if(e.type==sf::Event::MouseButtonPressed&&e.mouseButton.button==sf::Mouse::Left){
            m_mouse={(float)e.mouseButton.x,(float)e.mouseButton.y}; m_click=true;
        }

        // Zarządzanie stanami gry za pomocą Escape
        if(e.type==sf::Event::KeyPressed&&e.key.code==sf::Keyboard::Escape){
            if(m_state==GS::RACE||m_state==GS::COUNTDOWN) m_state=GS::PAUSE;
            else if(m_state==GS::PAUSE) m_state=GS::RACE;
            else if(m_state!=GS::MENU) m_state=GS::MENU;
            else m_win.close();
        }

        // Przekazywanie wejścia do aktualnego trybu gry
        if(m_state==GS::RACE||m_state==GS::COUNTDOWN)
            if(m_active) m_active->handleInput(e);

        // Obsługa interfejsu garażu
        if(m_state==GS::GARAGE)
            m_garage.handleEvent(e,m_win);
    }
}

// Logika i aktualizacja gry
void Game::update(float dt){
    // Odliczanie przed wyścigiem
    if(m_state==GS::COUNTDOWN){
        m_cdTimer+=dt;
        if(m_cdTimer>=4.7f&&!m_cdDone){ m_cdDone=true; m_state=GS::RACE; }
    }

    // Logika w trakcie wyścigu
    if(m_state==GS::RACE&&m_active){
        m_active->update(dt);
        if(m_active->isFinished()){
            m_lastResult=m_active->getResult();
            m_save.data.coins+=m_lastResult.coinsEarned;
            auto& sd=m_save.data;

            // Zapisywanie najlepszych wyników w zależności od trybu
            if(m_mode==0&&(sd.bestDragTime<=0||m_lastResult.bestTime<sd.bestDragTime)){
                sd.bestDragTime=m_lastResult.bestTime; sd.bestDragDiff=(int)m_diff; }
            if(m_mode==1&&(sd.bestCircuitLap<=0||m_lastResult.bestTime<sd.bestCircuitLap)){
                sd.bestCircuitLap=m_lastResult.bestTime; sd.bestCircuitDiff=(int)m_diff; }
            if(m_mode==2&&(sd.bestTimeAttackLap<=0||m_lastResult.bestTime<sd.bestTimeAttackLap)){
                sd.bestTimeAttackLap=m_lastResult.bestTime; }

            m_garage.setCoins(m_save.data.coins);
            m_save.save();
            m_state=GS::RESULTS;
        }
    }

    // Logika powrotu z garażu do menu
    if(m_state==GS::GARAGE){
        if(m_garage.update(dt)){
            m_garage.syncToSave(m_save.data);
            m_save.save();
            buildAssets(); // Przeładuj zasoby po zmianach w garażu
            m_player->getSprite().setTexture(m_skinTex[m_save.data.skinIndex]);
            m_player->applyStats(m_garage.buildStats());
            m_state=GS::MENU;
        }
    }
}

// Rysowanie poszczególnych ekranów gry
void Game::render(){
    m_win.clear({6,7,18});
    switch(m_state){
    case GS::MENU:      drawMenu();      break;
    case GS::MODE:      drawMode();      break;
    case GS::DIFF:      drawDiff();      break;
    case GS::COUNTDOWN:
        if(m_active) m_active->draw(m_win);
        drawCountdown(); break;
    case GS::RACE:
        if(m_active) m_active->draw(m_win); break;
    case GS::PAUSE:
        if(m_active) m_active->draw(m_win);
        drawPause(); break;
    case GS::RESULTS:   drawResults();   break;
    case GS::GARAGE:    m_garage.draw(m_win); break;
    case GS::RECORDS:   drawRecords();   break;
    }
    m_win.display();
}

// Włączanie/wyłączanie trybu pełnoekranowego
void Game::toggleFs(){
    m_fs=!m_fs; m_win.close();
    if(m_fs) m_win.create(sf::VideoMode::getDesktopMode(),"Nitro Drive",sf::Style::Fullscreen);
    else      m_win.create(sf::VideoMode(1280,720),"Nitro Drive",sf::Style::Titlebar|sf::Style::Close);
    m_win.setFramerateLimit(60);
}

// Setup przed nowym wyścigiem
void Game::startRace(){
    m_player->applyStats(m_garage.buildStats());

    // Reset każdego auta w kontenerze
    // Wyłapujemy boty rzutowaniem, żeby zresetować ich unikalne parametry AI
    for(Car* car : m_cars){
        car->reset();
        if(Bot* bot = dynamic_cast<Bot*>(car)){
            bot->resetProgress();
        }
    }

    // Odpalenie wybranego trybu
    switch(m_mode){
    case 0:
        m_drag=std::make_unique<DragMode>();
        m_drag->setDifficulty(m_diff); m_drag->setFont(m_font);
        m_drag->init(m_player,m_bots);
        m_active=m_drag.get(); break;
    case 2:
        m_timeAttack=std::make_unique<TimeAttackMode>();
        m_timeAttack->setFont(m_font);
        m_timeAttack->setPreviousBest(m_save.data.bestTimeAttackLap);
        m_timeAttack->init(m_player,m_bots);
        m_active=m_timeAttack.get(); break;
    default:
        m_circuit=std::make_unique<CircuitMode>();
        m_circuit->setDifficulty(m_diff); m_circuit->setFont(m_font);
        m_circuit->init(m_player,m_bots);
        m_active=m_circuit.get(); break;
    }
    m_cdTimer=0; m_cdDone=false;
    m_state=GS::COUNTDOWN;
}

// Ekran menu głównego
void Game::drawMenu(){
    drawBg(m_win);

    // Poświata tytułu dla lepszego efektu
    float pulse=0.5f+0.5f*std::sin(m_menuTime*2.5f);
    sf::RectangleShape aura({700,130}); aura.setPosition(290,30);
    aura.setFillColor({0,100,220,(uint8_t)(20+pulse*30)});
    m_win.draw(aura);

    // Tytuł w dwóch kolorach
    auto n1=T("NITRO",80,{0,220,255,255});   n1.setPosition(240,40);  m_win.draw(n1);
    auto n2=T("DRIVE",80,sf::Color::White);
    auto nb=n2.getLocalBounds();
    auto n1b=n1.getLocalBounds();
    n2.setPosition(240+n1b.width+n1b.left+12,40); m_win.draw(n2);

    // Kreska akcentująca pod tytułem
    sf::RectangleShape ul({680,4}); ul.setPosition(240,130);
    ul.setFillColor({0,180,255,(uint8_t)(120+pulse*80)}); m_win.draw(ul);

    // Wersja gry
    auto su=T("Racing Game  v2.0",20,{100,130,180,200}); su.setPosition(440,142); m_win.draw(su);

    // Ilość hajsu z prawym górnym rogu
    sf::RectangleShape cbox({180,38}); cbox.setPosition(1090,14);
    cbox.setFillColor({18,16,6,210}); cbox.setOutlineColor({200,170,0,150});
    cbox.setOutlineThickness(1.5f); m_win.draw(cbox);
    auto co=T("$ "+std::to_string(m_save.data.coins),22,{255,215,0,255});
    co.setPosition(1098,18); m_win.draw(co);

    // Główne przyciski
    float bx=540,by=220,bw=200,bh=54,gap=74;
    if(Btn(m_win,bx,by,      bw,bh,"PLAY"))    m_state=GS::MODE;
    if(Btn(m_win,bx,by+gap,  bw,bh,"GARAGE")){ m_garage.syncFromSave(m_save.data); m_state=GS::GARAGE; }
    if(Btn(m_win,bx,by+gap*2,bw,bh,"RECORDS")) m_state=GS::RECORDS;
    if(Btn(m_win,bx,by+gap*3,bw,bh,"QUIT"))    m_win.close();

    // Dolny pasek ze sterowaniem
    auto hint=T("WASD/Arrows: Drive    Space: Handbrake    E: Nitro    LShift: Gear Up    LCtrl: Gear Down",
                  12,{70,90,140,200});
    hint.setPosition(80,700); m_win.draw(hint);
}

// Ekran wyboru typu wyścigu
void Game::drawMode(){
    drawBg(m_win);

    // Pasek z tytułem ekranu
    sf::RectangleShape hbar({1280,58}); hbar.setPosition(0,0);
    hbar.setFillColor({10,12,28,240}); m_win.draw(hbar);
    sf::RectangleShape hl({1280,2}); hl.setPosition(0,58);
    hl.setFillColor({0,150,255,100}); m_win.draw(hl);
    auto ti=T("SELECT MODE",40,sf::Color::White); ti.setPosition(30,10); m_win.draw(ti);

    // Opisy dostępnych trybów
    struct MI{const char*n,*d,*d2;};
    MI ms[3]={
        {"DRAG RACE",
         "Sprint to the finish against 4 AI drivers.",
         "Collect coins, use nitro, shift gears at the right moment."},
        {"CIRCUIT",
         "5 laps vs 4 AI opponents. Race for 1st place.",
         "Shift gears, use nitro on straights, cut corners with handbrake."},
        {"TIME ATTACK",
         "Solo run, 3 laps, no opponents.",
         "Beat your personal best lap time. Delta shown on every lap."}
    };

    // Generowanie kart wyboru trybu
    for(int i=0;i<3;i++){
        float bx=200, by=100+i*170.f;
        bool sel=(m_mode==i);
        bool hov=sf::FloatRect(bx,by,880,145).contains(m_mouse);
        sf::RectangleShape card({880,145}); card.setPosition(bx,by);
        card.setFillColor(sel?sf::Color{12,25,60,240}:hov?sf::Color{15,18,42,240}:sf::Color{10,12,28,220});
        sf::Color oc=sel?sf::Color{0,220,255,240}:hov?sf::Color{60,80,160,200}:sf::Color{40,50,100,150};
        card.setOutlineColor(oc); card.setOutlineThickness(sel?3.f:2.f);
        m_win.draw(card);

        // Cienki kolorowy pasek wyróżniający kartę z lewej
        sf::RectangleShape bar({5,145}); bar.setPosition(bx,by);
        bar.setFillColor(i==0?sf::Color{255,140,0,220}:i==1?sf::Color{0,200,255,220}:sf::Color{200,80,255,220});
        m_win.draw(bar);

        auto nt=T(ms[i].n,34,sel?sf::Color{0,220,255,255}:sf::Color::White);
        nt.setPosition(bx+24,by+20); m_win.draw(nt);
        auto d1=T(ms[i].d,17,{160,180,220,220}); d1.setPosition(bx+24,by+72); m_win.draw(d1);
        auto d2=T(ms[i].d2,17,{120,140,180,180}); d2.setPosition(bx+24,by+100); m_win.draw(d2);

        // Akcja po kliknięciu (przejście dalej w zależności od wybranego trybu)
        if(m_click&&hov){
            m_mode=i;
            if(i==2){ startRace(); } // Time Attack od razu startuje, nie ma opcji trudności
            else     m_state=GS::DIFF;
        }
    }
    if(Btn(m_win,50,660,140,44,"< BACK")) m_state=GS::MENU;
}

// Ekran wyboru poziomu trudności botów
void Game::drawDiff(){
    drawBg(m_win);
    sf::RectangleShape hbar({1280,58}); hbar.setPosition(0,0);
    hbar.setFillColor({10,12,28,240}); m_win.draw(hbar);
    sf::RectangleShape hl({1280,2}); hl.setPosition(0,58);
    hl.setFillColor({0,150,255,100}); m_win.draw(hl);
    auto ti=T("DIFFICULTY",40,sf::Color::White); ti.setPosition(30,10); m_win.draw(ti);

    struct DI{const char*n,*d; sf::Color col;};
    DI ds[3]={
        {"EASY",   "AI drives slower and takes wide lines. Good for beginners.",  {50,200,80,255}},
        {"MEDIUM", "Balanced AI speed. Fair competition, winnable with good driving.",{255,200,0,255}},
        {"HARD",   "Fast and precise AI. Uses nitro on straights. Real challenge.",  {255,60,60,255}}
    };

    // Karty trudności z unikalnymi kolorami
    for(int i=0;i<3;i++){
        float bx=200, by=110+i*175.f;
        bool sel=((int)m_diff==i);
        bool hov=sf::FloatRect(bx,by,880,140).contains(m_mouse);
        sf::RectangleShape card({880,140}); card.setPosition(bx,by);
        card.setFillColor(sel?sf::Color{12,25,60,240}:hov?sf::Color{15,18,42,240}:sf::Color{10,12,28,220});
        sf::Color oc=sel?ds[i].col:hov?sf::Color{60,80,160,200}:sf::Color{40,50,100,150};
        card.setOutlineColor(oc); card.setOutlineThickness(sel?3.f:2.f);
        m_win.draw(card);

        sf::RectangleShape bar({5,140}); bar.setPosition(bx,by);
        bar.setFillColor(ds[i].col); m_win.draw(bar);

        auto nt=T(ds[i].n,30,sel?ds[i].col:sf::Color::White);
        nt.setPosition(bx+24,by+18); m_win.draw(nt);
        auto dt=T(ds[i].d,16,{150,170,210,210}); dt.setPosition(bx+24,by+68); m_win.draw(dt);

        // Start wyścigu po wyborze
        if(m_click&&hov){ m_diff=(Difficulty)i; startRace(); }
    }
    if(Btn(m_win,50,660,140,44,"< BACK")) m_state=GS::MODE;
}

// Wyświetla odliczanie 3-2-1-GO na ciemniejszym tle
void Game::drawCountdown(){
    sf::RectangleShape ov({1280,720}); ov.setFillColor({0,0,0,100}); m_win.draw(ov);
    if(m_cdTimer<1.5f){
        auto t=T("GET READY!",60,{255,220,0,255});
        auto b=t.getLocalBounds();
        t.setPosition(640-b.width/2.f-b.left,290-b.height/2.f-b.top);
        m_win.draw(t);
    } else {
        float el=m_cdTimer-1.5f;
        int num=3-(int)(el/0.8f);
        float phase=fmod(el,0.8f)/0.8f;
        float scale=1.f+(1.f-phase)*0.6f; // Ciekawy efekt skalowania przy odliczaniu
        std::string str=(num>0)?std::to_string(num):"GO!";
        sf::Color col=(num>0)?sf::Color::White:sf::Color{0,255,80,255};
        unsigned sz=(unsigned)(80*scale);
        auto t=T(str,sz,col);
        auto b=t.getLocalBounds();
        t.setPosition(640-b.width/2.f-b.left,300-b.height/2.f-b.top);
        m_win.draw(t);
    }
}

// Klasyczne okno pauzy na środku ekranu
void Game::drawPause(){
    sf::RectangleShape ov({1280,720}); ov.setFillColor({0,0,0,170}); m_win.draw(ov);

    sf::RectangleShape panel({320,290}); panel.setPosition(480,150);
    panel.setFillColor({10,12,28,240});
    panel.setOutlineColor({60,80,150,200}); panel.setOutlineThickness(2.f);
    m_win.draw(panel);

    sf::RectangleShape ptop({320,4}); ptop.setPosition(480,150);
    ptop.setFillColor({0,160,255,180}); m_win.draw(ptop);

    auto ti=T("PAUSED",44,sf::Color::White);
    auto tb=ti.getLocalBounds();
    ti.setPosition(640-tb.width/2.f-tb.left,162); m_win.draw(ti);

    float bx=530,by=240,bw=220,bh=50,gap=66;
    if(Btn(m_win,bx,by,      bw,bh,"RESUME"))     m_state=GS::RACE;
    if(Btn(m_win,bx,by+gap,  bw,bh,"GARAGE")){
        m_garage.syncFromSave(m_save.data); m_state=GS::GARAGE; }
    if(Btn(m_win,bx,by+gap*2,bw,bh,"MAIN MENU"))  m_state=GS::MENU;
}

// Podsumowanie zaraz po ukończeniu wyścigu
void Game::drawResults(){
    drawBg(m_win);
    sf::RectangleShape hbar({1280,58}); hbar.setPosition(0,0);
    hbar.setFillColor({10,12,28,240}); m_win.draw(hbar);
    sf::RectangleShape hl({1280,2}); hl.setPosition(0,58);
    hl.setFillColor({0,150,255,100}); m_win.draw(hl);
    auto ti=T("RESULTS",40,sf::Color::White); ti.setPosition(30,10); m_win.draw(ti);

    // Dynamiczny kolor i tekst dla zajętego miejsca
    std::string ps; sf::Color pc;
    switch(m_lastResult.position){
    case 1: ps="1ST PLACE!";  pc={255,215,0,255}; break;
    case 2: ps="2ND PLACE";   pc={140,220,140,255}; break;
    case 3: ps="3RD PLACE";   pc={100,180,255,255}; break;
    default:ps=std::to_string(m_lastResult.position)+"TH PLACE"; pc=sf::Color::White;
    }
    auto pt=T(ps,58,pc);
    auto pb=pt.getLocalBounds();
    pt.setPosition(640-pb.width/2.f-pb.left,80); m_win.draw(pt);

    // Tło dla statystyk
    sf::RectangleShape spanel({520,240}); spanel.setPosition(380,175);
    spanel.setFillColor({10,12,28,210}); spanel.setOutlineColor({60,80,140,180});
    spanel.setOutlineThickness(1.5f); m_win.draw(spanel);

    float sx=410,sy=195,gap=50;
    auto row=[&](const std::string& k,const std::string& v){
        auto kt=T(k,22,{160,180,220,220}); kt.setPosition(sx,sy); m_win.draw(kt);
        auto vt=T(v,22,sf::Color::White);
        auto vb=vt.getLocalBounds();
        vt.setPosition(870-vb.width-vb.left,sy); m_win.draw(vt);

        // Cienka linia między wierszami
        sf::RectangleShape sep({480,1}); sep.setPosition(sx,sy+40);
        sep.setFillColor({60,70,120,100}); m_win.draw(sep);
        sy+=gap;
    };

    if(m_mode==0){
        // Drag race jest krótki, więc wyświetlamy tylko jeden czas
        row("Finish time:", fmtT(m_lastResult.totalTime));
    } else {
        row("Total time:", fmtT(m_lastResult.totalTime));
        row(m_mode==2?"Best lap:":"Best lap:", fmtT(m_lastResult.bestTime));
    }
    row("Coins earned:", "$"+std::to_string(m_lastResult.coinsEarned));
    row("Total coins:", "$"+std::to_string(m_save.data.coins));

    if(Btn(m_win,530,620,220,52,"CONTINUE")) m_state=GS::MENU;
}

// Widok z najlepszymi czasami gracza
void Game::drawRecords(){
    drawBg(m_win);
    sf::RectangleShape hbar({1280,58}); hbar.setPosition(0,0);
    hbar.setFillColor({10,12,28,240}); m_win.draw(hbar);
    sf::RectangleShape hl({1280,2}); hl.setPosition(0,58);
    hl.setFillColor({0,150,255,100}); m_win.draw(hl);
    auto ti=T("RECORDS",40,sf::Color::White); ti.setPosition(30,10); m_win.draw(ti);

    auto& sd=m_save.data;
    static const char* dn[]={"Easy","Medium","Hard"};
    float ty=100,gap=80;

    // Tytuły kolumn
    auto hm=T("MODE",18,{120,140,180,200}); hm.setPosition(240,ty); m_win.draw(hm);
    auto hv=T("BEST",18,{120,140,180,200}); hv.setPosition(580,ty); m_win.draw(hv);
    auto hd=T("DIFF",18,{120,140,180,200}); hd.setPosition(820,ty); m_win.draw(hd);
    sf::RectangleShape sep({800,1}); sep.setPosition(240,ty+30);
    sep.setFillColor({60,70,120,160}); m_win.draw(sep);
    ty+=50;

    // Funkcja do rysowania wiersza tabeli z wynikami
    auto row=[&](const char* mode,const std::string& val,int diff){
        sf::RectangleShape rbg({800,60}); rbg.setPosition(240,ty-6);
        rbg.setFillColor({12,14,32,(uint8_t)(gap==80?180:140)}); m_win.draw(rbg);

        auto mt=T(mode,24,{200,210,240,240}); mt.setPosition(240,ty); m_win.draw(mt);
        auto vt=T(val,24,sf::Color::White);   vt.setPosition(580,ty); m_win.draw(vt);

        sf::Color dc=diff==0?sf::Color{80,220,80,255}:diff==1?sf::Color{255,200,0,255}:sf::Color{255,80,80,255};
        auto dt=T(dn[diff],20,dc); dt.setPosition(820,ty+2); m_win.draw(dt);

        sf::RectangleShape s2({800,1}); s2.setPosition(240,ty+52);
        s2.setFillColor({40,50,100,100}); m_win.draw(s2);
        ty+=gap;
    };

    // Dane do wyświetlenia
    row("DRAG RACE",    sd.bestDragTime>0?fmtT(sd.bestDragTime):"--:--.--", sd.bestDragDiff);
    row("CIRCUIT",      sd.bestCircuitLap>0?fmtT(sd.bestCircuitLap):"--:--.--", sd.bestCircuitDiff);

    // Time Attack jest wyodrębniony, bo nie posiada poziomu trudności botów
    {
        sf::RectangleShape rbg({800,60}); rbg.setPosition(240,ty-6);
        rbg.setFillColor({12,14,32,180}); m_win.draw(rbg);
        auto mt=T("TIME ATTACK",24,{255,200,0,240}); mt.setPosition(240,ty); m_win.draw(mt);
        auto vt=T(sd.bestTimeAttackLap>0?fmtT(sd.bestTimeAttackLap):"--:--.--",24,sf::Color::White);
        vt.setPosition(580,ty); m_win.draw(vt);
        auto dt=T("Solo",20,{150,180,255,220}); dt.setPosition(820,ty+2); m_win.draw(dt);
        sf::RectangleShape s2({800,1}); s2.setPosition(240,ty+52);
        s2.setFillColor({40,50,100,100}); m_win.draw(s2);
        ty+=80;
    }

    // Bezpieczne usuwanie danych żeby nikt nie skasował ich przez przypadek
    if(m_confirmReset){
        auto c=T("Are you sure? This cannot be undone!",22,{255,80,80,255});
        auto cb=c.getLocalBounds();
        c.setPosition(640-cb.width/2.f-cb.left,530); m_win.draw(c);
        if(Btn(m_win,340,575,200,46,"YES, DELETE")){
            m_save.reset(); m_save.save(); m_garage.syncFromSave(m_save.data); m_confirmReset=false;
        }
        if(Btn(m_win,560,575,130,46,"CANCEL")) m_confirmReset=false;
    } else {
        if(Btn(m_win,480,575,220,46,"RESET SAVE")) m_confirmReset=true;
    }
    if(Btn(m_win,50,660,140,44,"< BACK")) m_state=GS::MENU;
}