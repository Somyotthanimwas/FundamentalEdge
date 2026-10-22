#include "SwingSignal.h"
#include "Config.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
struct Bar { std::string date; double high=0, low=0, close=0, volume=0; };
struct Setup {
    std::string symbol,date,stage,reasons;
    double close=0,sma5=0,sma10=0,sma20=0,roc5=0,roc10=0,rsi=0,rsiPrev=0;
    double macdHist=0,macdHistPrev=0,macdHistPrev2=0,volumeRatio=0,averageTurnover20=0,reboundFromLow10=0;
    int score=0;
    double supertrendTrigger=0, distanceToTriggerAtr=0;
};
std::vector<std::string> SplitCsv(const std::string& line) {
    std::vector<std::string> fields; std::string value; bool quoted=false;
    for(char ch:line) { if(ch=='"') quoted=!quoted; else if(ch==','&&!quoted){fields.push_back(value);value.clear();} else value+=ch; }
    fields.push_back(value); return fields;
}
std::string Trim(std::string s) {
    while(!s.empty()&&std::isspace(static_cast<unsigned char>(s.front())))s.erase(s.begin());
    while(!s.empty()&&std::isspace(static_cast<unsigned char>(s.back())))s.pop_back(); return s;
}
double Number(const std::string& s){try{return std::stod(s);}catch(...){return 0.0;}}
double SMA(const std::vector<Bar>& b,std::size_t end,int n){if(end+1<static_cast<std::size_t>(n))return 0;double s=0;for(std::size_t i=end+1-n;i<=end;++i)s+=b[i].close;return s/n;}
double ROC(const std::vector<Bar>& b,std::size_t end,int n){if(end<static_cast<std::size_t>(n)||b[end-n].close==0)return 0;return (b[end].close/b[end-n].close-1)*100.0;}
double RSI(const std::vector<Bar>& b,std::size_t end,int n){if(end<static_cast<std::size_t>(n))return 0;double g=0,l=0;for(std::size_t i=end-n+1;i<=end;++i){double d=b[i].close-b[i-1].close;if(d>0)g+=d;else l-=d;}if(l==0)return 100;double rs=g/l;return 100-100/(1+rs);}
double AvgVolume(const std::vector<Bar>& b,std::size_t end,int n){if(end<static_cast<std::size_t>(n))return 0;double s=0;for(std::size_t i=end-n;i<end;++i)s+=b[i].volume;return s/n;}
double AverageDailyTurnover(const std::vector<Bar>& b,std::size_t end,int n){if(end<static_cast<std::size_t>(n))return 0;double s=0;for(std::size_t i=end-n;i<end;++i)s+=b[i].close*b[i].volume;return s/n;}
std::vector<double> MACDHist(const std::vector<Bar>& b,std::size_t end){std::vector<double> h(end+1,0);if(end<30)return h;const double a12=2.0/13,a26=2.0/27,a9=2.0/10;double e12=b[0].close,e26=b[0].close,signal=0;for(std::size_t i=1;i<=end;++i){e12+=a12*(b[i].close-e12);e26+=a26*(b[i].close-e26);double line=e12-e26;signal+=a9*(line-signal);h[i]=line-signal;}return h;}
struct STState { int trend=1; double trigger=0, atr=0; bool buySignal=false; };
STState SupertrendState(const std::vector<Bar>& b,std::size_t end,int period=10,double multiplier=3.0){
    STState result; if(end<static_cast<std::size_t>(period+2)) return result;
    std::vector<double> tr(end+1,0),atr(end+1,0),up(end+1,0),dn(end+1,0); std::vector<int> trend(end+1,1);
    for(std::size_t i=1;i<=end;++i) tr[i]=std::max({b[i].high-b[i].low,std::abs(b[i].high-b[i-1].close),std::abs(b[i].low-b[i-1].close)});
    double seed=0; for(int i=1;i<=period;++i) seed+=tr[i]; atr[period]=seed/period;
    for(std::size_t i=static_cast<std::size_t>(period+1);i<=end;++i) atr[i]=(atr[i-1]*(period-1)+tr[i])/period;
    for(std::size_t i=static_cast<std::size_t>(period);i<=end;++i){
        double src=(b[i].high+b[i].low)/2.0; double rawUp=src-multiplier*atr[i],rawDn=src+multiplier*atr[i];
        double up1=i>static_cast<std::size_t>(period)?up[i-1]:rawUp; double dn1=i>static_cast<std::size_t>(period)?dn[i-1]:rawDn;
        up[i]=(i>0 && b[i-1].close>up1)?std::max(rawUp,up1):rawUp;
        dn[i]=(i>0 && b[i-1].close<dn1)?std::min(rawDn,dn1):rawDn;
        int prior=i>static_cast<std::size_t>(period)?trend[i-1]:1;
        trend[i]=(prior==-1 && b[i].close>dn1)?1:((prior==1 && b[i].close<up1)?-1:prior);
    }
    result.trend=trend[end]; result.atr=atr[end];
    if(end>static_cast<std::size_t>(period)){ result.trigger=dn[end-1]; result.buySignal=(trend[end]==1 && trend[end-1]==-1); }
    return result;
}
bool IsDerivative(const std::string& s){static const std::regex w(R"(-W[0-9]*$)",std::regex::icase);static const std::regex dw(R"(^[A-Z]+[0-9]{2}[CP][0-9]{4}[A-Z]?$)",std::regex::icase);return std::regex_search(s,w)||std::regex_match(s,dw);}
}

bool SwingSignal::Run(const std::filesystem::path& projectDir,const std::filesystem::path& ohlcHistoryCsvPath){
    std::ifstream in(ohlcHistoryCsvPath);if(!in){std::cerr<<"Cannot open OHLC history: "<<ohlcHistoryCsvPath<<'\n';return false;}
    std::string line;if(!std::getline(in,line))return false;auto header=SplitCsv(line);
    int sym=-1,date=-1,hi=-1,lo=-1,cl=-1,vol=-1;
    for(std::size_t i=0;i<header.size();++i){std::string h=Trim(header[i]);std::transform(h.begin(),h.end(),h.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});if(h=="symbol"||h=="ticker")sym=i;else if(h=="date")date=i;else if(h=="high")hi=i;else if(h=="low")lo=i;else if(h=="close"||h=="last")cl=i;else if(h=="volume"||h=="vol")vol=i;}
    if(sym<0||date<0||hi<0||lo<0||cl<0||vol<0){std::cerr<<"OHLC CSV missing required columns\n";return false;}
    std::unordered_map<std::string,std::vector<Bar>> data;
    while(std::getline(in,line)){if(line.empty())continue;auto f=SplitCsv(line);int mx=std::max({sym,date,hi,lo,cl,vol});if(mx>=static_cast<int>(f.size()))continue;std::string ticker=Trim(f[sym]);if(ticker.empty()||IsDerivative(ticker))continue;Bar b;b.date=Trim(f[date]);b.high=Number(f[hi]);b.low=Number(f[lo]);b.close=Number(f[cl]);b.volume=Number(f[vol]);if(b.close>0)data[ticker].push_back(b);}
    std::vector<Setup> setups;std::size_t shortHistory=0;
    for(auto& item:data){const std::string& ticker=item.first;auto& b=item.second;std::sort(b.begin(),b.end(),[](const Bar&a,const Bar&c){return a.date<c.date;});if(b.size()<static_cast<std::size_t>(SwingConfig::MIN_BARS)){++shortHistory;continue;}std::size_t i=b.size()-1;if(i<35)continue;
        double close=b[i].close; STState st=SupertrendState(b,i,10,3.0); if(st.buySignal || st.trend!=-1 || st.atr<=0 || st.trigger<=close) continue; double triggerGap=(st.trigger-close)/st.atr; if(triggerGap>1.0) continue;
        double sma5=SMA(b,i,5),sma10=SMA(b,i,10),sma20=SMA(b,i,20),sma5p=SMA(b,i-1,5),sma20p=SMA(b,i-3,20),roc5=ROC(b,i,5),roc10=ROC(b,i,10),rsi=RSI(b,i,14),rsiP=RSI(b,i-1,14),rsiP2=RSI(b,i-2,14);
        auto hist=MACDHist(b,i);double h0=hist[i],h1=hist[i-1],h2=hist[i-2],av=AvgVolume(b,i,20),vr=av>0?b[i].volume/av:0,turnover20=AverageDailyTurnover(b,i,20);
        double low10=b[i].low;for(std::size_t j=i-9;j<=i;++j)low10=std::min(low10,b[j].low);double rebound=low10>0?(close/low10-1)*100:0;
        if(turnover20<=10000000.0)continue;
        int score=0;std::vector<std::string> why;
        if(close>=sma20){score+=15;why.push_back("close_above_SMA20");}else if(close>=sma20*.985){score+=8;why.push_back("near_SMA20");}
        if(sma20>sma20p){score+=10;why.push_back("SMA20_rising");}
        if(sma5>sma10){score+=15;why.push_back("SMA5_over_SMA10");}else if(sma5>sma5p&&close>=sma5*.99){score+=8;why.push_back("short_MA_turning_up");}
        if(roc5>0){score+=10;why.push_back("ROC5_positive");}if(roc10>0){score+=8;why.push_back("ROC10_positive");}
        if(rsi>=45&&rsi<=72){score+=8;why.push_back("RSI_recovery_zone");}if(rsi>rsiP){score+=8;why.push_back("RSI_rising");}if(rsi>rsiP&&rsiP>=rsiP2){score+=5;why.push_back("RSI_rising_2bars");}
        if(h0>h1){score+=10;why.push_back("MACD_hist_improving");}if(h0>h1&&h1>h2){score+=8;why.push_back("MACD_hist_rising_2bars");}if(h0>0){score+=5;why.push_back("MACD_hist_positive");}else if(std::abs(h0)/close*100<=.35&&h0>h1){score+=5;why.push_back("MACD_near_cross");}
        if(vr>=1.2){score+=10;why.push_back("volume_surge");}else if(vr>=.8){score+=5;why.push_back("volume_support");}if(rebound>=1&&rebound<=12){score+=8;why.push_back("rebound_from_10d_low");}
        if(rsi>78||roc5>18)score-=10;if(score<80||!(roc5>0||rsi>rsiP||h0>h1))continue;
        Setup x;x.symbol=ticker;x.date=b[i].date;x.close=close;x.score=score;x.sma5=sma5;x.sma10=sma10;x.sma20=sma20;x.roc5=roc5;x.roc10=roc10;x.rsi=rsi;x.rsiPrev=rsiP;x.macdHist=h0;x.macdHistPrev=h1;x.macdHistPrev2=h2;x.volumeRatio=vr;x.averageTurnover20=turnover20;x.reboundFromLow10=rebound;x.supertrendTrigger=st.trigger;x.distanceToTriggerAtr=triggerGap;
        x.stage="PRE_BUY_SUPERTREND_ZONE"; why.push_back("Supertrend_bearish_not_bought"); why.push_back("within_1.0_ATR_of_buy_flip");for(std::size_t k=0;k<why.size();++k){if(k)x.reasons+=';';x.reasons+=why[k];}setups.push_back(x);
    }
    std::sort(setups.begin(),setups.end(),[](const Setup&a,const Setup&b){if(a.score!=b.score)return a.score>b.score;if(a.volumeRatio!=b.volumeRatio)return a.volumeRatio>b.volumeRatio;return a.symbol<b.symbol;});
    auto output=projectDir/"Data"/"near_buy_results.csv";std::ofstream out(output);if(!out){std::cerr<<"Cannot write: "<<output<<'\n';return false;}
    out<<"symbol,date,last_close,setup,score,sma5,sma10,sma20,roc5_pct,roc10_pct,rsi14,rsi14_prev,macd_hist,macd_hist_prev,volume_ratio_20d,average_turnover_20d_baht,supertrend_buy_trigger,distance_to_trigger_atr,rebound_from_10d_low_pct,reasons\n";out<<std::fixed<<std::setprecision(4);
    for(const auto&x:setups)out<<x.symbol<<','<<x.date<<','<<x.close<<','<<x.stage<<','<<x.score<<','<<x.sma5<<','<<x.sma10<<','<<x.sma20<<','<<x.roc5<<','<<x.roc10<<','<<x.rsi<<','<<x.rsiPrev<<','<<x.macdHist<<','<<x.macdHistPrev<<','<<x.volumeRatio<<','<<x.averageTurnover20<<','<<x.supertrendTrigger<<','<<x.distanceToTriggerAtr<<','<<x.reboundFromLow10<<",\""<<x.reasons<<"\"\n";
    out.close();std::cout<<"\nNear-Buy / Reversal Watchlist\nUniverse symbols (after DW/W-pattern filter): "<<data.size()<<"\nPre-Buy Supertrend candidates (max 1.0 ATR) (20d avg traded value > THB 10m): "<<setups.size()<<"\nShort-history symbols skipped: "<<shortHistory<<"\nOutput: "<<output<<'\n';
    for(std::size_t n=0;n<std::min<std::size_t>(10,setups.size());++n)std::cout<<setups[n].symbol<<" score="<<setups[n].score<<" "<<setups[n].stage<<" RSI="<<setups[n].rsi<<" MACD_hist="<<setups[n].macdHist<<'\n';return true;
}
