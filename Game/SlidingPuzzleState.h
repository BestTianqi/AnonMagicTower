#pragma once
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>
#include <sstream>
#include <string>

class SlidingPuzzleState {
public:
    explicit SlidingPuzzleState(int size = 3) { reset(size); }
    void reset(int size) {
        m_size = std::clamp(size, 3, 8);
        m_tiles.resize(m_size * m_size);
        std::iota(m_tiles.begin(), m_tiles.end(), 1);
        m_tiles.back() = 0;
        m_blank = static_cast<int>(m_tiles.size()) - 1;
        m_history.clear();
    }
    void shuffle(unsigned seed) {
        reset(m_size);
        std::mt19937 random(seed);
        int previous = -1;
        // 从完成状态执行合法滑动，奇偶阶都保证有解。
        for (int step = 0; step < m_size * m_size * 100; ++step) {
            std::vector<int> choices;
            for (int i = 0; i < static_cast<int>(m_tiles.size()); ++i)
                if (canMove(i) && i != previous) choices.push_back(i);
            const int oldBlank = m_blank;
            const int next = choices[random() % choices.size()];
            std::swap(m_tiles[next], m_tiles[m_blank]);
            m_blank = next;
            previous = oldBlank;
        }
        if (solved()) move(m_blank - 1);
        m_history.clear();
    }
    bool canMove(int index) const {
        return index >= 0 && index < static_cast<int>(m_tiles.size()) &&
            std::abs(index / m_size - m_blank / m_size) +
            std::abs(index % m_size - m_blank % m_size) == 1;
    }
    bool move(int index) {
        if (!canMove(index)) return false;
        m_history.push_back(m_blank);
        std::swap(m_tiles[index], m_tiles[m_blank]);
        m_blank = index;
        return true;
    }
    bool undo() {
        if (m_history.empty()) return false;
        const int previous = m_history.back();
        m_history.pop_back();
        std::swap(m_tiles[previous], m_tiles[m_blank]);
        m_blank = previous;
        return true;
    }
    bool solved() const {
        for (int i = 0; i + 1 < static_cast<int>(m_tiles.size()); ++i)
            if (m_tiles[i] != i + 1) return false;
        return m_tiles.back() == 0;
    }
    int size() const { return m_size; }
    int blank() const { return m_blank; }
    int moves() const { return static_cast<int>(m_history.size()); }
    const std::vector<int>& tiles() const { return m_tiles; }
    std::string serialize() const {
        std::ostringstream out; out << "PUZZLE 1 " << m_size << ' ' << m_history.size() << '\n';
        for(int value:m_tiles)out<<value<<' ';
        for(int index:m_history)out<<index<<' ';
        return out.str();
    }
    bool restore(const std::string& data) {
        std::istringstream in(data);std::string tag;int version,size,count;
        if(!(in>>tag>>version>>size>>count) || tag!="PUZZLE" || version!=1 || size<3 || size>8 || count<0 || count>100000)return false;
        SlidingPuzzleState candidate(size);
        std::vector<bool> seen(size*size);
        for(int i=0;i<size*size;++i){int value;if(!(in>>value) || value<0 || value>=size*size || seen[value])return false;seen[value]=true;candidate.m_tiles[i]=value;if(!value)candidate.m_blank=i;}
        int inversions=0;
        for(int i=0;i<size*size;++i)for(int j=i+1;j<size*size;++j)if(candidate.m_tiles[i] && candidate.m_tiles[j] && candidate.m_tiles[i]>candidate.m_tiles[j])++inversions;
        if(size%2 ? inversions%2!=0 : (inversions+size-candidate.m_blank/size)%2!=1)return false;
        for(int i=0;i<count;++i){int index;if(!(in>>index) || index<0 || index>=size*size)return false;candidate.m_history.push_back(index);}
        in>>std::ws;if(!in.eof())return false;
        auto reverse=candidate;
        while(!reverse.m_history.empty()){if(!reverse.canMove(reverse.m_history.back()))return false;reverse.undo();}
        *this=std::move(candidate);return true;
    }
private:
    int m_size = 3;
    int m_blank = 8;
    std::vector<int> m_tiles;
    std::vector<int> m_history;
};
