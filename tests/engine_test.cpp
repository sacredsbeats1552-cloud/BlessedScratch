#include "../Source/ScratchEngine.h"
#include <cmath>
#include <cstdio>
#include <chrono>
using namespace bs;
static Audio makeVocal(double sr, double secs){
    Audio a; size_t n=(size_t)(secs*sr); a.resize(n);
    for(size_t i=0;i<n;++i){ double t=i/sr; double f=180+60*std::sin(2*3.14159*1.3*t);
        double env=0.5+0.5*std::sin(2*3.14159*3.0*t);
        float v=(float)(0.4*env*(std::sin(2*3.14159*f*t)+0.5*std::sin(2*3.14159*2*f*t)+0.25*std::sin(2*3.14159*3*f*t))*0.6);
        a.ch[0][i]=v; a.ch[1][i]=v*0.9f; }
    return a;
}
static void stats(const char*name,const Audio&o){
    double peak=0,rms=0; size_t nan=0; double maxd=0; 
    for(size_t i=0;i<o.size();++i){ float v=o.ch[0][i]; if(!std::isfinite(v)){nan++;continue;} peak=std::max(peak,(double)std::fabs(v)); rms+=v*v;
        if(i) maxd=std::max(maxd,(double)std::fabs(v-o.ch[0][i-1])); }
    rms=std::sqrt(rms/o.size());
    printf("%-28s len=%zu peak=%.3f rms=%.4f maxStep=%.3f nan=%zu\n",name,o.size(),peak,rms,maxd,nan);
}
int main(){
    double sr=48000; Audio src=makeVocal(sr,4.0);
    Settings s; s.bpm=90; s.durationBeats=16;
    auto t0=std::chrono::steady_clock::now();
    s.mode=Mode::Scratch; stats("scratch", render(src,0,src.size(),s,sr));
    for(int v=0;v<4;v++){ s.variation=(Variation)v; char b[40]; sprintf(b,"scratch var%d",v); stats(b,render(src,0,src.size(),s,sr)); }
    s.mode=Mode::StopTape;
    const char* names[]={"vinyl","tape","slow","deep","hard","reverse"};
    for(int t=0;t<6;t++){ s.stopType=(StopType)t; s.restart=(t%2==0); char b[40]; sprintf(b,"stop %s%s",names[t],s.restart?"+restart":""); stats(b,render(src,0,src.size(),s,sr)); }
    s.mode=Mode::Combo; s.durationBeats=16; stats("combo all", render(src,0,src.size(),s,sr));
    for(int k=0;k<4;k++){ s.combos={false,false,false,false}; s.combos[k]=true; char b[40]; sprintf(b,"combo only %d",k); stats(b,render(src,0,src.size(),s,sr)); }
    s.durationBeats=2; s.combos={true,true,true,true}; stats("combo short(2 beats)",render(src,0,src.size(),s,sr));
    // edge cases
    Audio empty; stats("empty src",render(empty,0,0,s,sr));
    Audio tiny; tiny.resize(10); stats("tiny src",render(tiny,0,10,s,sr));
    s.bpm=300; s.stepsPerBeat=8; s.mode=Mode::Scratch; s.durationBeats=32; stats("extreme",render(src,0,src.size(),s,sr));
    auto t1=std::chrono::steady_clock::now();
    printf("total ms: %.0f\n", std::chrono::duration<double,std::milli>(t1-t0).count());
}
