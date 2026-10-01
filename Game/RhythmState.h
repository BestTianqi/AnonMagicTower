#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
struct RhythmNote { int time = 0, lane = 0, result = 0; };
struct RhythmState {
    std::vector<RhythmNote> notes;
    int combo = 0, maxCombo = 0, perfect = 0, great = 0, good = 0, miss = 0;
    int lastError = 0;
    void advance(int time) {
        for (auto& n : notes) if (!n.result && time - n.time > 150) {
            n.result = -1; ++miss; combo = 0;
        }
    }
    int hit(int lane, int time) {
        advance(time);
        RhythmNote* nearest = nullptr;
        int delta = 151;
        for (auto& n : notes) if (!n.result && n.lane == lane) {
            if (std::abs(time-n.time) < delta) { nearest = &n; delta = std::abs(time-n.time); }
            break; // Never skip an earlier unresolved note in the same lane.
        }
        if (!nearest) return 0;
        lastError = time - nearest->time;
        int result = delta <= 55 ? 3 : delta <= 105 ? 2 : 1;
        nearest->result = result;
        if (result == 3) ++perfect; else if (result == 2) ++great; else ++good;
        maxCombo = std::max(maxCombo, ++combo);
        return result;
    }
    bool finished() const { return perfect+great+good+miss == int(notes.size()); }
    double accuracy() const { int n=perfect+great+good+miss; return n ? (perfect+great*.75+good*.4)*100/n : 100.; }
    int score() const { return notes.empty() ? 0 : int((perfect+great*.75+good*.4)*1000000/notes.size()); }
};

inline std::vector<RhythmNote> makeRhythmChart(const std::vector<RhythmNote>& source, int difficulty) {
    const int stride = difficulty == 0 ? 3 : difficulty == 1 ? 2 : 1;
    const int lanes[] = {0,1,2,3,1,2,0,3};
    std::vector<RhythmNote> result;
    for (size_t i=0; i<source.size(); i+=stride)
        result.push_back({source[i].time, lanes[result.size()%8]});
    return result;
}
