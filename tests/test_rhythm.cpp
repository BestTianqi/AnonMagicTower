#include "Game/RhythmState.h"
#include <cstdlib>
#include <iostream>
static void require(bool ok, const char* message) { if (!ok) { std::cerr << message << '\n'; std::exit(1); } }
int main() {
    RhythmState state;
    state.notes = {{1000, 0}, {1500, 1}, {2000, 2}, {2400, 3}};
    require(state.hit(1, 1000) == 0, "wrong lane must not consume a note");
    require(state.hit(0, 1040) == 3, "40ms should be perfect");
    require(state.hit(0, 1040) == 0, "same note cannot score twice");
    require(state.hit(1, 1590) == 2, "90ms should be great");
    require(state.combo == 2, "combo accumulates");
    state.advance(2200);
    require(state.miss == 1 && state.combo == 0, "expired note resets combo");
    require(state.hit(3, 2530) == 1, "130ms should be good");
    require(state.maxCombo == 2 && state.finished(), "all notes accounted for");
    require(state.score() > 0 && state.score() < 1000000, "partial accuracy score");
    RhythmState close;
    close.notes={{1000,0},{1100,0}};
    require(close.hit(0,1090)==2 && close.notes[0].result==2 && close.notes[1].result==0,
            "same-lane notes must be consumed in order, not skipped for a closer note");
    RhythmState boundary; boundary.notes={{1000,0},{2000,1}};
    require(boundary.hit(0,850)==1,"early 150ms is included");
    require(boundary.hit(1,2150)==1,"late 150ms is included");
    std::vector<RhythmNote> source;
    for(int i=0;i<48;++i)source.push_back({1000+i*200,i%4});
    for(int difficulty=0;difficulty<3;++difficulty) {
        auto chart=makeRhythmChart(source,difficulty); int lanes=0;
        for(auto n:chart)lanes|=1<<n.lane;
        require(lanes==15,"all difficulties must use all four lanes");
        require(chart.size()==size_t(48/(3-difficulty)),"difficulty changes density only");
    }
    std::cout << "rhythm judgement tests passed\n";
}
