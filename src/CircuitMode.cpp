// Tryb wyścigu na okrążenia (gracz i 4 boty)
// Obsługuje trasę, AI, liczenie okrążeń, zbieranie przedmiotów i rysowanie.

#include "CircuitMode.hpp"
#include "Game.hpp"  // CIRCUIT_LAPS
#include <cmath>
#include <cstdlib>

// Konstruktor
CircuitMode::CircuitMode(){}

// Generowanie tekstur dla zbieranych przedmiotów
void CircuitMode::buildTex(){
    // Funkcja pomocnicza rysująca okrągłe tekstury
    auto mk=[](sf::Color c)->sf::Texture{
        sf::RenderTexture rt; rt.create(40,40);
        rt.clear(sf::Color::Transparent);
        sf::CircleShape ci(18); ci.setOrigin(18,18); ci.setPosition(20,20);
        ci.setFillColor(c); ci.setOutlineColor({0,0,0,100}); ci.setOutlineThickness(2);
        rt.draw(ci); rt.display(); return rt.getTexture();
    };

    // Złote to monety, niebieskie to nitro
    m_coinTex=mk({255,215,0,255}); m_nitroTex=mk({0,180,255,255}); m_texOk=true;
}

// Przygotowanie wyścigu: mapa, pozycje startowe, boty i przedmioty
void CircuitMode::init(Car* player, std::vector<std::unique_ptr<Bot>>& bots){
    m_player=player; m_bots=&bots;

    // Generowanie trasy i ładowanie fontu
    m_map.generate(MapType::CIRCUIT);
    m_hud.loadFont("assets/fonts/racing_font.ttf");
    buildTex();
    m_cols.clear();

    auto& s=m_map.getStartPositions();
    auto& wps=m_map.getWaypoints();

    // Obliczanie kąta startowego na podstawie pierwszego odcinka
    float startAngle = 0.f;
    if(wps.size()>=2){
        sf::Vector2f dir=wps[1]-wps[0];
        startAngle=std::atan2(dir.y,dir.x)*180.f/3.14159265f;
    }

    // Ustawienie gracza na pierwszej pozycji
    if(!s.empty()){ m_player->setPosition(s[0]); m_player->setAngle(startAngle); }
    m_player->reset();

    // Ustawienie botów na kolejnych pozycjach
    for(size_t i=0;i<bots.size()&&i+1<s.size();i++){
        bots[i]->setPosition(s[i+1]); bots[i]->setAngle(startAngle); bots[i]->reset();
        bots[i]->setWaypoints(m_map.getWaypoints());
        bots[i]->setDifficulty(m_diff);
        bots[i]->setMaxLaps(CIRCUIT_LAPS + 1); // +1 bo boty liczą od 0, a gracz od 1
        bots[i]->resetProgress();
    }

    // Rozmieszczenie przedmiotów w pobliżu waypointów (co 4 to nitro)
    for(size_t i=0;i<wps.size();i++){
        sf::Vector2f off={(float)(rand()%70-35),(float)(rand()%70-35)};
        m_cols.emplace_back(CollectibleType::COIN, wps[i]+off, m_coinTex, m_nitroTex);
        if(i%4==2){
            sf::Vector2f offN={(float)(rand()%50-25),(float)(rand()%50-25)};
            m_cols.emplace_back(CollectibleType::NITRO, wps[i]+offN, m_coinTex, m_nitroTex);
        }
    }

    // Konfiguracja kamery
    m_camera.setSize(1280,720);

    // Zerowanie liczników i stanu wyścigu
    m_raceTime=0; m_lapTime=0; m_bestLap=0; m_lap=1;
    m_coins=0; m_wpIdx=0; m_done=false;
    m_crossedFinish=false;
    m_finishCooldown=8.f; // Czas na odjechanie od mety przed liczeniem okrążeń
}

// Obsługa sterowania z klawiatury
void CircuitMode::handleInput(const sf::Event& e){
    if(e.type!=sf::Event::KeyPressed) return;

    // Wyższy bieg (tylko przy odpowiednich obrotach)
    if(e.key.code==sf::Keyboard::LShift){
        if(m_player->getShiftCooldown()<=0.f && m_player->getRPM()>=6500.f)
            m_player->shiftUp();
    }
    // Niższy bieg
    if(e.key.code==sf::Keyboard::LControl){
        if(m_player->getShiftCooldown()<=0.f)
            m_player->shiftDown();
    }
    // Użycie nitro
    if(e.key.code==sf::Keyboard::E) m_player->activateNitro();
}

// Sprawdzanie miejsca gracza w stawce na podstawie postępów na trasie
int CircuitMode::calcPos() const {
    auto& wps=m_map.getWaypoints();
    int total=(int)wps.size();

    float playerProg=(float)((m_lap-1)*total + m_wpIdx);
    int pos=1; // Zakładamy prowadzenie

    for(auto& b:*m_bots){
        if(b->getProgress() > playerProg) ++pos; // Bot wyprzedza gracza
    }
    return pos;
}

// Główna pętla wyścigu
void CircuitMode::update(float dt){
    if(!m_player) return;

    // Boty aktualizują się niezależnie
    for(auto& b:*m_bots) b->updateAI(dt);

    // Zatrzymanie logiki gracza po ukończeniu wyścigu
    if(m_done) return;

    // Aktualizacja czasu
    m_raceTime+=dt; m_lapTime+=dt;
    if(m_finishCooldown>0) m_finishCooldown-=dt;

    // Sterowanie autem gracza
    float th=0,br=0,st=0; bool hb=false;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::W)||sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    th=1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)||sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  br=1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)||sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  st=-1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::D)||sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) st=1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) hb=true;

    m_player->setThrottle(th); m_player->setBrake(br);
    m_player->setSteering(st); m_player->setHandbrake(hb);

    // Mniejsza moc na trawie
    bool onTrack = m_map.isOnTrack(m_player->getPosition());
    if(!onTrack){
        m_player->setThrottle(th * 0.25f);
    }

    m_player->update(dt);

    // Zbieranie monet i nitro
    for(auto& c:m_cols){
        c.update(dt);
        if(!c.isCollected()&&c.getBounds().intersects(m_player->getBounds())){
            c.collect();
            if(c.getType()==CollectibleType::COIN) m_coins+=50;
            else m_player->addNitro(30);
        }
    }

    // Kolizje gracza z botami (spowolnienie i iskry)
    for(auto& b:*m_bots){
        if(m_player->getBounds().intersects(b->getBounds())){
            m_psys.emit(ParticleEffect::SPARK,m_player->getPosition(),0);
            m_player->applyCollisionImpact(b->getSpeed());
            b->applyCollisionImpact(m_player->getSpeed());
        }
    }

    // Generowanie cząsteczek
    {
        float ang=m_player->getAngle()*3.14159f/180.f;
        sf::Vector2f rear=m_player->getPosition()-sf::Vector2f(std::cos(ang)*30,std::sin(ang)*30);

        // Spaliny
        m_psys.emit(ParticleEffect::EXHAUST,rear,m_player->getAngle()+180,m_player->getRPM()/8500.f);

        // Dym i ślady opon podczas driftu
        float da=std::abs(m_player->getSlideAngle());
        if(m_player->isSliding()&&da>8.f){
            float inten=std::min(1.f,da/50.f);
            sf::Vector2f wL=rear+sf::Vector2f(-std::sin(ang)*14,std::cos(ang)*14);
            sf::Vector2f wR=rear+sf::Vector2f(std::sin(ang)*14,-std::cos(ang)*14);

            m_psys.emit(ParticleEffect::SMOKE_TIRE,wL,m_player->getAngle(),inten);
            m_psys.emit(ParticleEffect::SMOKE_TIRE,wR,m_player->getAngle(),inten);
            m_psys.addTireMark(wL,m_player->getAngle());
            m_psys.addTireMark(wR,m_player->getAngle());
        }

        // Płomień z rury przy nitro
        if(m_player->isNitroActive())
            m_psys.emit(ParticleEffect::NITRO_FLAME,rear,m_player->getAngle()+180,1.f);
    }
    m_psys.update(dt);

    // Zaliczanie waypointów przez gracza
    auto& wps=m_map.getWaypoints();
    if(!wps.empty()){
        sf::Vector2f wp=wps[m_wpIdx%wps.size()];
        float dist=std::hypot(m_player->getPosition().x-wp.x,
                                m_player->getPosition().y-wp.y);
        if(dist<120.f){
            m_wpIdx++;
            if(m_wpIdx>=(int)wps.size()) m_wpIdx=0; // Zapętlenie
        }
    }

    // Przejazd przez metę i liczenie okrążeń
    sf::FloatRect fz=m_map.getFinishZone();
    bool inFinish=fz.contains(m_player->getPosition());
    if(m_finishCooldown<=0){
        if(inFinish&&!m_crossedFinish){
            // Aktualizacja rekordu okrążenia
            if(m_bestLap<=0||m_lapTime<m_bestLap) m_bestLap=m_lapTime;

            m_lapTime=0;
            m_lap++;
            m_wpIdx=0;
            m_crossedFinish=true;
            m_finishCooldown=2.f; // Zabezpieczenie przed podwójnym zaliczeniem
        }
    }
    if(!inFinish) m_crossedFinish=false;

    // Kamera podąża za graczem, ale nie wychodzi poza mapę
    auto p=m_player->getPosition();
    auto sz=m_map.getSize();
    m_camera.setCenter(std::clamp(p.x,640.f,sz.x-640.f),
                       std::clamp(p.y,360.f,sz.y-360.f));

    // Przekazanie danych do interfejsu (HUD)
    int pos=calcPos();
    m_hd.speed=m_player->getSpeed(); m_hd.rpm=m_player->getRPM();
    m_hd.gear=m_player->getGear();   m_hd.nitro=m_player->getNitro();
    m_hd.nitroMax=m_player->getNitroMax(); m_hd.coins=m_coins;
    m_hd.raceTime=m_raceTime; m_hd.lapTime=m_lapTime; m_hd.bestLap=m_bestLap;
    m_hd.lap=m_lap; m_hd.totalLaps=CIRCUIT_LAPS; m_hd.position=pos;
    m_hd.totalCars=1+(int)m_bots->size();
    m_hd.showRPM=false; m_hd.showDrift=false; m_hd.showLap=true;
    m_hd.offTrack=!onTrack;

    // Koniec wyścigu po przejechaniu ustalonej liczby okrążeń
    if(m_lap>CIRCUIT_LAPS){
        m_done=true;
        m_result.totalTime=m_raceTime; m_result.bestTime=m_bestLap;
        m_result.coinsEarned=m_coins; m_result.position=pos;
        m_result.finished=true;
    }
}

// Rysowanie sceny
void CircuitMode::draw(sf::RenderTarget& t){
    // Rysowanie świata gry z perspektywy kamery
    auto orig=t.getView(); t.setView(m_camera);
    m_map.draw(t);
    m_psys.draw(t);
    for(auto& c:m_cols) c.draw(t);
    for(auto& b:*m_bots) b->draw(t);
    m_player->draw(t);

    // Przełączenie na standardowy widok i nałożenie HUD-a na wierzch
    t.setView(orig);
    m_hud.draw(t,m_hd,{1280u,720u});
}
