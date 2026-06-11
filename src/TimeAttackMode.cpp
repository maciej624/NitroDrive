// TimeAttackMode.cpp — Tryb jazdy na czas: 3 okrazenia, gracz sam na torze.
// Brak botow — celem jest pobicie rekordu okrazenia.
// Wyswietla time (zielony/czerwony) po kazdym okrazeniu.
#include "TimeAttackMode.hpp"
#include "Game.hpp"  // TIME_ATTACK_LAPS
#include <cmath>
#include <cstdlib>


TimeAttackMode::TimeAttackMode(){}

// Tworzy proceduralne tekstury collectibli (coin = zloty, nitro = niebieski).
void TimeAttackMode::buildTex(){
    // Lambada: okragla tekstura 40x40 w zadanym kolorze
    auto mk=[](sf::Color c)->sf::Texture{
        sf::RenderTexture rt; rt.create(40,40);
        rt.clear(sf::Color::Transparent);
        sf::CircleShape ci(18); ci.setOrigin(18,18); ci.setPosition(20,20);
        ci.setFillColor(c); ci.setOutlineColor({0,0,0,100}); ci.setOutlineThickness(2);
        rt.draw(ci); rt.display(); return rt.getTexture();
    };
    m_coinTex=mk({255,215,0,255}); m_nitroTex=mk({0,180,255,255}); m_texOk=true;
}

// ── init 
// Inicjalizuje tryb time attack: generuje obwod, rozmieszcza collectible
// zeruje timery i liczniki.
void TimeAttackMode::init(Car* player, std::vector<std::unique_ptr<Bot>>& bots){
    m_player=player;
    // Time Attack: ignorujemy boty 
    // bots nie sa uzywane — gracz jedzie sam
    (void)bots;

    // Generuj losowy tor okrazeniowy (ten sam typ co circuit)
    m_map.generate(MapType::CIRCUIT);
    m_hud.loadFont("assets/fonts/racing_font.ttf");
    buildTex();
    m_cols.clear();  // wyczysc collectible z poprzedniego przebiegu

    auto& s=m_map.getStartPositions();
    auto& wps=m_map.getWaypoints();

    // Oblicz kat startowy (zgodny z kierunkiem toru)
    float startAngle = 0.f;
    if(wps.size()>=2){
        sf::Vector2f dir=wps[1]-wps[0];
        startAngle=std::atan2(dir.y,dir.x)*180.f/3.14159265f;
    }
    if(!s.empty()){ m_player->setPosition(s[0]); m_player->setAngle(startAngle); }
    m_player->reset();

    // ── Collectible: wiecej nitro niz w circuit (co 3. waypoint zamiast co 4.) 
    for(size_t i=0;i<wps.size();i++){
        sf::Vector2f off={(float)(rand()%70-35),(float)(rand()%70-35)};
        m_cols.emplace_back(CollectibleType::COIN, wps[i]+off, m_coinTex, m_nitroTex);
        if(i%3==1){  // ~33% szansa na nitro przy waypoincie
            sf::Vector2f offN={(float)(rand()%50-25),(float)(rand()%50-25)};
            m_cols.emplace_back(CollectibleType::NITRO, wps[i]+offN, m_coinTex, m_nitroTex);
        }
    }

    // Kamera 1280x720, zerowanie wszystkich licznikow
    m_camera.setSize(1280,720);
    m_raceTime=0; m_lapTime=0; m_bestLap=0; m_lap=1;
    m_coins=0; m_wpIdx=0; m_done=false;
    m_crossedFinish=false;
    m_finishCooldown=3.f;   // 3s cooldown startowy
    m_showDelta=false; m_showDeltaTimer=0.f; m_deltaTime=0.f;
    m_sectorProgress=0.f;
}

// ── handleInput 
// Klawisze: Shift = wyzszy bieg, Ctrl = nizszy bieg, E = nitro.
void TimeAttackMode::handleInput(const sf::Event& e){
    if(e.type!=sf::Event::KeyPressed) return;
    // Wbicie wyzszego biegu: tylko gdy RPM >= 6500 i cooldown = 0
    if(e.key.code==sf::Keyboard::LShift){
        if(m_player->getShiftCooldown()<=0.f && m_player->getRPM()>=6500.f)
            m_player->shiftUp();
    }
    // Redukcja biegu (np. przed ostrym zakretem)
    if(e.key.code==sf::Keyboard::LControl){
        if(m_player->getShiftCooldown()<=0.f)
            m_player->shiftDown();
    }
    // Aktywacja nitro
    if(e.key.code==sf::Keyboard::E) m_player->activateNitro();
}

// ── update 
// Glowna petla: sterowanie, fizyka, collectible, czasteczki, waypoints,
// liczenie okrazen i wyswietlanie delty czasu.
void TimeAttackMode::update(float dt){
    if(!m_player||m_done) return;

    // Aktualizuj wszystkie timery
    m_raceTime+=dt; m_lapTime+=dt;
    if(m_finishCooldown>0) m_finishCooldown-=dt;   // odliczaj cooldown startowy
    if(m_showDeltaTimer>0) m_showDeltaTimer-=dt;   // odliczaj czas pokazywania delty
    else m_showDelta=false;  // ukryj delta-time po uplynieciu timera

    // ── Sterowanie gracza (WASD / strzalki) 
    float th=0,br=0,st=0; bool hb=false;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::W)||sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    th=1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)||sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  br=1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)||sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  st=-1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::D)||sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) st=1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) hb=true;
    m_player->setThrottle(th); m_player->setBrake(br);
    m_player->setSteering(st); m_player->setHandbrake(hb);

    // Trawa: ograniczone przyspieszenie poza asfaltem
    bool onTrack = m_map.isOnTrack(m_player->getPosition());
    if(!onTrack) m_player->setThrottle(th * 0.25f);  // 25% mocy na trawie
    m_player->update(dt);  // aktualizuj fizyke gracza

    // ── Collectibles 
    for(auto& c:m_cols){
        c.update(dt);  // animacja (obracanie)
        if(!c.isCollected()&&c.getBounds().intersects(m_player->getBounds())){
            c.collect();
            if(c.getType()==CollectibleType::COIN) m_coins+=50;  // +50 monet
            else m_player->addNitro(30);  // +30 nitro (wazne w time attack!)
        }
    }

    // ── Efekty czasteczkowe 
    {
        // Punkt emisji = tyl samochodu
        float ang=m_player->getAngle()*3.14159f/180.f;
        sf::Vector2f rear=m_player->getPosition()-sf::Vector2f(std::cos(ang)*30,std::sin(ang)*30);
        // Spaliny (intensywnosc wzrasta z RPM)
        m_psys.emit(ParticleEffect::EXHAUST,rear,m_player->getAngle()+180,m_player->getRPM()/8500.f);
        float da=std::abs(m_player->getSlideAngle());
        // Dym opon podczas posl izgu (slide angle > 8 stopni)
        if(m_player->isSliding()&&da>8.f){
            float inten=std::min(1.f,da/50.f);
            sf::Vector2f wL=rear+sf::Vector2f(-std::sin(ang)*14,std::cos(ang)*14);
            sf::Vector2f wR=rear+sf::Vector2f(std::sin(ang)*14,-std::cos(ang)*14);
            m_psys.emit(ParticleEffect::SMOKE_TIRE,wL,m_player->getAngle(),inten);
            m_psys.emit(ParticleEffect::SMOKE_TIRE,wR,m_player->getAngle(),inten);
            m_psys.addTireMark(wL,m_player->getAngle());  // trwale slady opon
            m_psys.addTireMark(wR,m_player->getAngle());
        }
        // Plomien nitro gdy aktywne
        if(m_player->isNitroActive())
            m_psys.emit(ParticleEffect::NITRO_FLAME,rear,m_player->getAngle()+180,1.f);
    }
    m_psys.update(dt);  // przesun i wygaszaj czasteczki

    // ── Sledzenie waypointow 
    auto& wps=m_map.getWaypoints();
    if(!wps.empty()){
        sf::Vector2f wp=wps[m_wpIdx%wps.size()];
        float dist=std::hypot(m_player->getPosition().x-wp.x,
                              m_player->getPosition().y-wp.y);
        if(dist<120.f){  // zalicz waypoint przy odleglosci < 120 px
            m_wpIdx++;
            if(m_wpIdx>=(int)wps.size()) m_wpIdx=0;  // wrap listy
        }
    }

    // ── Liczenie okrazen i roznica time 
    sf::FloatRect fz=m_map.getFinishZone();
    bool inFinish=fz.contains(m_player->getPosition());
    if(m_finishCooldown<=0){
        if(inFinish&&!m_crossedFinish){
            // Oblicz roznica-time vs najlepsze okrazenie
            if(m_bestLap>0){
                m_deltaTime=m_lapTime-m_bestLap;  // ujemny = szybciej, dodatni = wolniej
                m_showDelta=true;
                m_showDeltaTimer=2.8f;  // pokazuj przez 2.8s
            }
            // Aktualizuj rekord jesli biezace okrazenie jest szybsze
            if(m_bestLap<=0||m_lapTime<m_bestLap) m_bestLap=m_lapTime;
            m_lapTime=0;   // zeruj timer okrazenia
            m_lap++;       // nastepne okrazenie
            m_wpIdx=0;     // reset waypointow
            m_crossedFinish=true;
            m_finishCooldown=2.f;  // 2s cooldown — zapobiega podwojnemu liczeniu
        }
    }
    if(!inFinish) m_crossedFinish=false;

    // ── Kamera: sledzi gracza, nie wychodzi poza granice mapy 
    auto p=m_player->getPosition();
    auto sz=m_map.getSize();
    m_camera.setCenter(std::clamp(p.x,640.f,sz.x-640.f),
                        std::clamp(p.y,360.f,sz.y-360.f));

    // ── Aktualizacja danych HUD 
    m_hd.speed=m_player->getSpeed(); m_hd.rpm=m_player->getRPM();
    m_hd.gear=m_player->getGear();   m_hd.nitro=m_player->getNitro();
    m_hd.nitroMax=m_player->getNitroMax(); m_hd.coins=m_coins;
    m_hd.raceTime=m_raceTime; m_hd.lapTime=m_lapTime; m_hd.bestLap=m_bestLap;
    m_hd.lap=m_lap; m_hd.totalLaps=TIME_ATTACK_LAPS;
    m_hd.position=1; m_hd.totalCars=1; // zawsze "1/1" — brak przeciwnikow
    m_hd.showRPM=false; m_hd.showDrift=false; m_hd.showLap=true;
    m_hd.offTrack=!onTrack;  // ostrzezenie o zjechaniu z toru

    // ── Warunek konca: po 3 okrazeniaach 
    if(m_lap>TIME_ATTACK_LAPS){
        m_done=true;
        m_result.totalTime=m_raceTime; m_result.bestTime=m_bestLap;
        m_result.coinsEarned=m_coins; m_result.position=1;
        m_result.finished=true;
    }
}

// ── draw 
// Renderuje scene, HUD, delta-time (zielony jesli szybciej, czerwony jesli wolniej)
// i naglowek "TIME ATTACK" w gornym lewym rogu.
void TimeAttackMode::draw(sf::RenderTarget& t){
    auto orig=t.getView(); t.setView(m_camera);  // widok swiatowy
    m_map.draw(t);   // podklad toru
    m_psys.draw(t);  // czasteczki i slady opon
    for(auto& c:m_cols) c.draw(t);  // collectible
    m_player->draw(t);  // gracz (brak botow w tym trybie)
    t.setView(orig);    // przywroc widok ekranowy
    m_hud.draw(t,m_hd,{1280u,720u});  // HUD

    // ── roznica-time: wyswietlaj przez 2.8s po przejezdzie lini mety 
    if(m_showDelta && m_showDeltaTimer>0){
        sf::Font fnt; fnt.loadFromFile("assets/fonts/racing_font.ttf");
        bool faster=(m_deltaTime<0);  // ujemna delta = szybciej niz rekord
        // Kolor: zielony = poprawa, czerwony = pogorszenie
        sf::Color dc=faster?sf::Color{0,255,80,255}:sf::Color{255,60,60,255};
        char buf[32];
        // Format: -0.123 (szybciej) lub +0.456 (wolniej)
        if(faster) snprintf(buf,32,"-%0.3f",std::abs(m_deltaTime));
        else        snprintf(buf,32,"+%0.3f",m_deltaTime);
        sf::Text dt; dt.setFont(fnt); dt.setString(buf);
        dt.setCharacterSize(36); dt.setFillColor(dc);
        dt.setOutlineColor(sf::Color::Black); dt.setOutlineThickness(2.f);
        // Fadeout: pelna nieprzezroczystosc przez pierwsze 0.5s od znikniecia
        float alpha=std::min(1.f,m_showDeltaTimer/0.5f);
        dc.a=(uint8_t)(alpha*255);
        dt.setFillColor(dc);
        auto b=dt.getLocalBounds();
        dt.setPosition(640.f-b.width/2.f-b.left, 280.f);  // wycentruj poziomo
        t.draw(dt);

        // Komunikat "NEW RECORD!" gdy pobito poprzedni rekord
        if(faster && m_prevBest>0 && (m_bestLap < m_prevBest)){
            sf::Text rec; rec.setFont(fnt);
            rec.setString("NEW RECORD!");
            rec.setCharacterSize(28); rec.setFillColor({255,215,0,(uint8_t)(alpha*255)});
            rec.setOutlineColor(sf::Color::Black); rec.setOutlineThickness(2.f);
            auto rb=rec.getLocalBounds();
            rec.setPosition(640.f-rb.width/2.f-rb.left, 322.f);  // pod delta-time
            t.draw(rec);
        }
    }

    // ── Naglowek "TIME ATTACK" w gornym lewym rogu 
    sf::Font fnt2; fnt2.loadFromFile("assets/fonts/racing_font.ttf");
    sf::Text header; header.setFont(fnt2);
    header.setString("TIME ATTACK");
    header.setCharacterSize(18); header.setFillColor({255,200,0,200});
    header.setOutlineColor(sf::Color::Black); header.setOutlineThickness(1.f);
    header.setPosition(8,2);  // lewy gorny rog ekranu
    t.draw(header);
}
