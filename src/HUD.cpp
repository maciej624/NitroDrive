#include "HUD.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>

static const float PI=3.14159265f;
// Jeeli nie znajdzie czcionki w projekcie, szukamy w Windowsie classica
bool HUD::loadFont(const std::string& p){
    m_ok=m_font.loadFromFile(p);
    if(!m_ok) m_ok=m_font.loadFromFile("C:/Windows/Fonts/arial.ttf");
    return m_ok;
}
// Formatowanie czasu wyscigu
std::string HUD::fmtT(float s){
    int m=(int)(s/60); float sc=s-m*60;
    std::ostringstream o;
    o<<std::setfill('0')<<std::setw(2)<<m<<":"
     <<std::setfill('0')<<std::setw(5)<<std::fixed<<std::setprecision(2)<<sc;
    return o.str();
}
sf::Text HUD::txt(const std::string& s,unsigned sz,sf::Color c){
    sf::Text t;
    if(m_ok) t.setFont(m_font);
    t.setString(s); t.setCharacterSize(sz); t.setFillColor(c);
    // Czarny obrys dla lepszej widocznosci tekstu
    t.setOutlineColor(sf::Color::Black); t.setOutlineThickness(1.5f);
    return t;
}
void HUD::panel(sf::RenderTarget& t,float x,float y,float w,float h,
                sf::Color fill,sf::Color outline){
    // Rysowanie tla panelu
    sf::RectangleShape r({w,h}); r.setPosition(x,y);
    r.setFillColor(fill);
    r.setOutlineColor(outline); r.setOutlineThickness(2.f);
    t.draw(r);

    sf::RectangleShape accent({w-4,2}); accent.setPosition(x+2,y+2);
    accent.setFillColor({outline.r,outline.g,outline.b,80});
    t.draw(accent);
}

void HUD::drawSpeedo(sf::RenderTarget& t,const HUDData& d,sf::Vector2u ws){
    float cx=120, cy=(float)ws.y-120.f, r=95.f;
    sf::CircleShape glow(r+6); glow.setOrigin(r+6,r+6); glow.setPosition(cx,cy);
    glow.setFillColor({0,0,0,0});
    glow.setOutlineColor({0,140,255,60}); glow.setOutlineThickness(6);
    t.draw(glow);
    sf::CircleShape bg(r); bg.setOrigin(r,r); bg.setPosition(cx,cy);
    bg.setFillColor({8,10,22,220});
    bg.setOutlineColor({0,120,255,180}); bg.setOutlineThickness(2.5f);
    t.draw(bg);
    int total=14;
    for(int i=0;i<=total;i++){
        float a=(-210.f+i*(240.f/(float)total))*PI/180.f;
        bool major=(i%2==0);
        float r1=r-4.f, r2=major?r-22.f:r-13.f;
        sf::VertexArray ln(sf::Lines,2);
        ln[0].position={cx+std::cos(a)*r1,cy+std::sin(a)*r1};
        ln[1].position={cx+std::cos(a)*r2,cy+std::sin(a)*r2};
        sf::Color tc=(i>=12)?sf::Color{255,60,60,255}:
                     (i>=10)?sf::Color{255,200,0,255}:sf::Color{200,220,255,220};
        ln[0].color=ln[1].color=tc;
        t.draw(ln);
    }
    float ratio=std::min(1.f,d.speed/280.f);
    int arcPts=32;
    sf::VertexArray arc(sf::TriangleFan, arcPts+2);
    arc[0].position={cx,cy}; arc[0].color={0,0,0,0};
    for(int i=0;i<=arcPts;i++){
        float frac=(float)i/(float)arcPts;
        if(frac>ratio) frac=ratio;
        float a=(-210.f+frac*240.f)*PI/180.f;
        float ri=r-8.f;
        arc[i+1].position={cx+std::cos(a)*ri,cy+std::sin(a)*ri};
        uint8_t rr=(uint8_t)(frac*255);
        uint8_t gg=(uint8_t)(180-frac*180);
        arc[i+1].color={rr,gg,60,(uint8_t)(80+frac*100)};
        if((float)i/(float)arcPts>ratio) { arc[i+1].color.a=0; }
    }
    t.draw(arc);
    float na=(-210.f+ratio*240.f)*PI/180.f;
    sf::VertexArray nd(sf::Lines,2);
    nd[0].position={cx,cy};
    nd[1].position={cx+std::cos(na)*(r-16.f),cy+std::sin(na)*(r-16.f)};
    nd[0].color={255,80,0,255}; nd[1].color={255,255,200,255};
    t.draw(nd);
    sf::CircleShape dot(6); dot.setOrigin(6,6); dot.setPosition(cx,cy);
    dot.setFillColor({255,80,0,255}); t.draw(dot);
    auto sp=txt(std::to_string((int)d.speed),26,sf::Color::White);
    auto sb=sp.getLocalBounds();
    sp.setPosition(cx-sb.width/2.f,cy+20);
    t.draw(sp);
    auto km=txt("km/h",13,{140,160,200,200});
    km.setPosition(cx-16,cy+50); t.draw(km);
}

void HUD::drawGear(sf::RenderTarget& t,const HUDData& d,sf::Vector2u ws){
    float gx=230, gy=(float)ws.y-130.f;
    panel(t,gx,gy,64,64,{10,12,28,220},{0,180,255,160});
    auto gl=txt(std::to_string(d.gear),40,{0,220,255,255});
    auto gb=gl.getLocalBounds();
    gl.setPosition(gx+(64-gb.width)/2.f-gb.left, gy+8);
    t.draw(gl);
    auto gn=txt("GEAR",11,{100,140,180,180});
    gn.setPosition(gx+10,gy+50); t.draw(gn);
}

void HUD::drawNitroBar(sf::RenderTarget& t,const HUDData& d,sf::Vector2u ws){
    float bx=(float)ws.x-185.f, by=18.f, bw=165.f, bh=24.f;
    panel(t,bx-2,by-22,bw+4,bh+26,{8,10,22,210},{0,180,255,150});
    auto lb=txt("NITRO",12,{0,200,255,220});
    lb.setPosition(bx,by-19); t.draw(lb);
    sf::RectangleShape bg({bw,bh}); bg.setPosition(bx,by);
    bg.setFillColor({0,0,0,180});
    bg.setOutlineColor({0,100,200,150}); bg.setOutlineThickness(1.5f);
    t.draw(bg);
    float ratio=(d.nitroMax>0)?std::clamp(d.nitro/d.nitroMax,0.f,1.f):0.f;
    if(ratio>0.001f){
        int segs=10;
        float sw=bw/segs;
        for(int i=0;i<segs;i++){
            float fr=(float)(i+1)/(float)segs;
            if(fr>ratio+0.01f) break;
            float alpha=std::min(1.f,ratio/fr)*220.f;
            sf::RectangleShape seg({sw-3,bh-4});
            seg.setPosition(bx+i*sw+1,by+2);
            uint8_t bb=(uint8_t)(80+fr*175);
            seg.setFillColor({0,(uint8_t)(fr<0.5f?200:150),bb,(uint8_t)alpha});
            t.draw(seg);
        }
    }
    auto co=txt("$"+std::to_string(d.coins),22,{255,215,0,255});
    co.setPosition((float)ws.x-180.f, by+bh+10.f);
    t.draw(co);
}

void HUD::drawTopCenter(sf::RenderTarget& t,const HUDData& d,sf::Vector2u ws){
    float cx=(float)ws.x/2.f;
    float panW=210.f, panH=d.showLap?118.f:72.f;
    panel(t,cx-panW/2,2,panW,panH,{8,10,22,200},{80,100,180,200});
    // Wyswietlanie Pozycji w wyscigu - zaleznie od miejsca zmiana koloru (1=Zolty, 2=Zielony, itp.)
    sf::Color posCol=d.position==1?sf::Color::Yellow:
                     d.position==2?sf::Color::Green:
                     d.position==3?sf::Color{100,200,255,255}:sf::Color::White;
    auto posT=txt(std::to_string(d.position)+"/"+std::to_string(d.totalCars),30,posCol);
    auto pb=posT.getLocalBounds();
    posT.setPosition(cx-pb.width/2.f-pb.left,6); t.draw(posT);
    // Czas gry
    auto timeT=txt(fmtT(d.raceTime),19,{200,210,255,230});
    auto tb=timeT.getLocalBounds();
    timeT.setPosition(cx-tb.width/2.f-tb.left,42); t.draw(timeT);
    // Lap info
    if(d.showLap){
        auto lapT=txt("LAP "+std::to_string(d.lap)+"/"+std::to_string(d.totalLaps),
                      14,{180,200,255,210});
        auto lb2=lapT.getLocalBounds();
        lapT.setPosition(cx-lb2.width/2.f-lb2.left,68); t.draw(lapT);
        if(d.bestLap>0){
            auto blT=txt("BEST "+fmtT(d.bestLap),12,{80,255,120,210});
            auto bb2=blT.getLocalBounds();
            blT.setPosition(cx-bb2.width/2.f-bb2.left,88); t.draw(blT);
        }
    }
}

void HUD::draw(sf::RenderTarget& t,const HUDData& d,sf::Vector2u ws){
    drawSpeedo(t,d,ws);
    drawGear(t,d,ws);
    drawNitroBar(t,d,ws);
    drawTopCenter(t,d,ws);

    if(d.showDrift){
        float px=4, py=(float)ws.y-240.f;
        panel(t,px,py,240,110,{8,10,22,200},{80,0,180,200});
        auto sc=txt("DRIFT  "+std::to_string(d.driftScore),20,{200,100,255,255});
        sc.setPosition(px+8,py+8); t.draw(sc);
        sf::Color cc=d.combo>=4?sf::Color::Red:d.combo>=3?sf::Color::Yellow:sf::Color::White;
        auto co=txt("x"+std::to_string(d.combo),32,cc);
        co.setPosition(px+8,py+36); t.draw(co);
        std::ostringstream oa; oa<<std::fixed<<std::setprecision(1)<<std::abs(d.driftAngle)<<"deg";
        auto an=txt(oa.str(),15,{180,180,255,255});
        an.setPosition(px+8,py+76); t.draw(an);
    }

    // Off-track warning
    if(d.offTrack){
        auto w=txt("! OFF TRACK - SLOWING !",22,{255,60,0,255});
        auto wb=w.getLocalBounds();
        w.setPosition((float)ws.x/2.f-wb.width/2.f-wb.left, (float)ws.y/2.f-60.f);
        t.draw(w);
    }
}
