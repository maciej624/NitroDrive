// Tryb drag race: wyścig na wprost do linii mety.
// Gracz i 4 boty startują na równoległych pasach. Brak okrążeń.
// Wygrywa ten, kto pierwszy przekroczy linię mety.

#include "DragMode.hpp"
#include <cmath>
#include <cstdlib>

// Konstruktor
DragMode::DragMode(){}

// Generowanie tekstur dla zbieranych przedmiotów
void DragMode::buildTex(){
    // Funkcja rysująca okrągłe tekstury
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

// Przygotowanie wyścigu: mapa, pasy startowe, przedmioty i reset liczników
void DragMode::init(Car* player, std::vector<std::unique_ptr<Bot>>& bots){
    m_player=player; m_bots=&bots;

    // Generowanie prostej trasy
    m_map.generate(MapType::DRAG);
    m_hud.loadFont("assets/fonts/racing_font.ttf");
    buildTex();
    m_cols.clear();

    auto& s=m_map.getStartPositions();

    // Gracz startuje na środkowym pasie
    if(!s.empty()){ m_player->setPosition(s[0]); m_player->setAngle(0); }
    m_player->reset();

    // Przypisanie pozostałych pasów dla botów (pomijamy pas gracza)
    for(size_t i=0;i<bots.size()&&i+1<s.size();i++){
        bots[i]->setPosition(s[i+1]); bots[i]->setAngle(0); bots[i]->reset();

        int botLane = (i < 2) ? (int)i : (int)i + 1;
        bots[i]->setWaypoints(m_map.getLaneWaypoints(botLane));
        bots[i]->setDifficulty(m_diff);
        bots[i]->setDragMode(true); // Boty nie zawracają w trybie drag
        bots[i]->setBotId((int)i);
    }

    // Rozmieszczenie monet i nitro na torze (ok. 17% szans na nitro)
    float lanesY[4]={230,310,410,490};
    for(float x=350;x<3000;x+=90){
        int lane=rand()%4;
        bool isNitro=(rand()%6==0);
        m_cols.emplace_back(
            isNitro?CollectibleType::NITRO:CollectibleType::COIN,
            sf::Vector2f{x, lanesY[lane]},
            m_coinTex, m_nitroTex
            );
    }

    // Konfiguracja kamery i ustawienie linii mety
    m_camera.setSize(1280,720);
    m_finishX=3040; m_raceTime=0; m_coins=0; m_done=false;
}

// Obsługa sterowania z klawiatury (biegi i nitro)
void DragMode::handleInput(const sf::Event& e){
    if(e.type!=sf::Event::KeyPressed) return;

    // Wyższy bieg (wymagane odpowiednie obroty)
    if(e.key.code==sf::Keyboard::LShift||e.key.code==sf::Keyboard::RShift){
        if(m_player->getShiftCooldown()<=0.f && m_player->getRPM()>=6500.f)
            m_player->shiftUp();
    }
    // Niższy bieg
    if(e.key.code==sf::Keyboard::LControl||e.key.code==sf::Keyboard::RControl){
        if(m_player->getShiftCooldown()<=0.f)
            m_player->shiftDown();
    }
    // Użycie nitro
    if(e.key.code==sf::Keyboard::E) m_player->activateNitro();
}

// Główna pętla wyścigu
void DragMode::update(float dt){
    if(!m_player) return;

    // Boty jadą do mety niezależnie od statusu gracza
    for(auto& b:*m_bots){
        auto bp=b->getPosition();
        if(bp.x >= m_finishX){
            // Zatrzymanie bota po przekroczeniu mety
            b->setThrottle(0.f); b->setBrake(1.f);
            b->update(dt);
            b->setPosition({m_finishX + 5.f, bp.y});
            continue;
        }
        b->updateAI(dt);

        // Blokada wyjazdu poza wyznaczone pasy
        bp=b->getPosition();
        bp.y=std::clamp(bp.y,175.f,545.f);
        b->setPosition(bp);
        if(bp.x < 150.f){ bp.x=150.f; b->setPosition(bp); }
    }

    // Zatrzymanie logiki gracza po ukończeniu wyścigu
    if(m_done) return;

    m_raceTime+=dt;

    // Sterowanie autem gracza
    float th=0,br=0,st=0; bool hb=false;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::W)||sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    th=1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)||sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  br=1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)||sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  st=-1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::D)||sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) st=1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) hb=true;

    m_player->setThrottle(th); m_player->setBrake(br);
    m_player->setSteering(st); m_player->setHandbrake(hb);

    // Mniejsza moc przy zjechaniu na trawę
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

    // Kolizje gracza z botami (tylko wizualne iskry, brak kar do prędkości w dragu)
    for(auto& b:*m_bots){
        if(m_player->getBounds().intersects(b->getBounds())){
            m_psys.emit(ParticleEffect::SPARK,m_player->getPosition(),0);
        }
    }

    // Generowanie cząsteczek
    {
        float ang=m_player->getAngle()*3.14159f/180.f;
        sf::Vector2f rear=m_player->getPosition()-sf::Vector2f(std::cos(ang)*30,std::sin(ang)*30);

        // Spaliny przy wyższych obrotach
        if(m_player->getRPM()>1200)
            m_psys.emit(ParticleEffect::EXHAUST,rear,m_player->getAngle()+180,m_player->getRPM()/8500.f);

        // Dym z opon (mniejszy próg uślizgu niż na zwykłym torze)
        float da=std::abs(m_player->getSlideAngle());
        if(m_player->isSliding()&&da>6.f){
            float inten=std::min(1.f,da/50.f);
            sf::Vector2f wL=rear+sf::Vector2f(-std::sin(ang)*14,std::cos(ang)*14);
            sf::Vector2f wR=rear+sf::Vector2f(std::sin(ang)*14,-std::cos(ang)*14);
            m_psys.emit(ParticleEffect::SMOKE_TIRE,wL,m_player->getAngle(),inten);
            m_psys.emit(ParticleEffect::SMOKE_TIRE,wR,m_player->getAngle(),inten);
        }

        // Płomień z rury przy nitro
        if(m_player->isNitroActive())
            m_psys.emit(ParticleEffect::NITRO_FLAME,rear,m_player->getAngle()+180,1.f);
    }
    m_psys.update(dt);

    // Kamera podąża za graczem w osi X, Y pozostaje stałe
    auto p=m_player->getPosition();
    float camX=std::clamp(p.x,640.f,m_map.getSize().x-640.f);
    m_camera.setCenter(camX,360.f);

    // Obliczanie pozycji gracza względem botów
    int pos2=1;
    for(auto& b:*m_bots) if(b->getPosition().x>m_player->getPosition().x) pos2++;

    // Przekazanie danych do interfejsu (HUD)
    m_hd.speed=m_player->getSpeed(); m_hd.rpm=m_player->getRPM();
    m_hd.gear=m_player->getGear(); m_hd.nitro=m_player->getNitro();
    m_hd.nitroMax=m_player->getNitroMax(); m_hd.coins=m_coins;
    m_hd.raceTime=m_raceTime; m_hd.position=pos2;
    m_hd.totalCars=1+(int)m_bots->size();
    m_hd.showRPM=true; m_hd.showDrift=false; m_hd.showLap=false;

    // Koniec wyścigu po przekroczeniu mety
    if(m_player->getPosition().x>=m_finishX){
        m_done=true;
        m_result.totalTime=m_raceTime; m_result.bestTime=m_raceTime;
        m_result.coinsEarned=m_coins; m_result.position=pos2; m_result.finished=true;
    }
}

// Rysowanie sceny
void DragMode::draw(sf::RenderTarget& t){
    // Rysowanie świata z perspektywy kamery
    auto orig=t.getView(); t.setView(m_camera);
    m_map.draw(t);
    m_psys.draw(t);
    for(auto& c:m_cols) c.draw(t);
    for(auto& b:*m_bots) b->draw(t);
    m_player->draw(t);

    // Rysowanie białej linii mety
    sf::RectangleShape fl({10,380}); fl.setPosition(m_finishX,170);
    fl.setFillColor({255,255,255,130}); t.draw(fl);

    // Powrót do widoku okna i nałożenie HUD-a na wierzch
    t.setView(orig);
    m_hud.draw(t,m_hd,{1280u,720u});
}
