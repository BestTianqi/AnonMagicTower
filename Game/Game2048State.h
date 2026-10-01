#pragma once
#include <algorithm>
#include <array>
#include <random>
#include <vector>
#include <sstream>
#include <string>

class Game2048State {
public:
    using Board = std::array<int, 16>;
    enum class Direction { Left, Right, Up, Down };
    struct Motion { int from; int to; int value; };
    struct Slide {
        Board tiles{};
        int gain = 0;
        std::vector<Motion> motions;
    };

    explicit Game2048State(unsigned seed = std::random_device{}()) : m_random(seed) { restart(); }

    static Slide slide(const Board& board, Direction direction) {
        Slide result;
        for (int line = 0; line < 4; ++line) {
            const auto index = [=](int offset) {
                switch (direction) {
                case Direction::Left: return line * 4 + offset;
                case Direction::Right: return line * 4 + 3 - offset;
                case Direction::Up: return offset * 4 + line;
                case Direction::Down: return (3 - offset) * 4 + line;
                }
                return 0;
            };
            std::vector<int> sources;
            for (int i = 0; i < 4; ++i) if (board[index(i)]) sources.push_back(index(i));
            int output = 0;
            for (size_t i = 0; i < sources.size(); ++i) {
                const int target = index(output++);
                const int source = sources[i];
                result.tiles[target] = board[source];
                result.motions.push_back({source, target, board[source]});
                // A newly merged tile cannot merge again during the same turn.
                if (i + 1 < sources.size() && board[source] == board[sources[i + 1]]) {
                    result.tiles[target] *= 2;
                    result.gain += result.tiles[target];
                    const int other = sources[++i];
                    result.motions.push_back({other, target, board[other]});
                }
            }
        }
        return result;
    }

    static bool hasMoves(const Board& board) {
        for (int i = 0; i < 16; ++i) {
            if (!board[i]) return true;
            if (i % 4 < 3 && board[i] == board[i + 1]) return true;
            if (i / 4 < 3 && board[i] == board[i + 4]) return true;
        }
        return false;
    }

    void restart() {
        m_tiles.fill(0);
        m_score = m_moves = 0;
        m_history.clear();
        spawn(); spawn();
    }

    bool move(Direction direction) {
        const auto result = slide(m_tiles, direction);
        if (result.tiles == m_tiles) return false;
        m_history.push_back({m_tiles, m_score, m_moves, m_random, direction});
        m_tiles = result.tiles;
        m_score += result.gain;
        ++m_moves;
        spawn();
        return true;
    }

    bool undo() {
        if (m_history.empty()) return false;
        const auto previous = m_history.back();
        m_history.pop_back();
        m_tiles = previous.tiles;
        m_score = previous.score;
        m_moves = previous.moves;
        m_random = previous.random;
        return true;
    }

    const Board& tiles() const { return m_tiles; }
    int score() const { return m_score; }
    int moves() const { return m_moves; }
    int highestTile() const { return *std::max_element(m_tiles.begin(), m_tiles.end()); }
    bool canUndo() const { return !m_history.empty(); }
    bool gameOver() const { return !hasMoves(m_tiles); }
    std::string serialize() const {
        // Store one RNG state and replay valid moves; avoids one large RNG blob per undo step.
        const auto& tiles=m_history.empty()?m_tiles:m_history.front().tiles;
        const auto& random=m_history.empty()?m_random:m_history.front().random;
        std::ostringstream out;out<<"MERGE2048 1 "<<m_history.size()<<'\n';
        for(int value:tiles)out<<value<<' ';
        out<<'\n'<<random<<'\n';
        for(const auto& entry:m_history)out<<int(entry.direction)<<' ';
        return out.str();
    }
    bool restore(const std::string& data) {
        std::istringstream in(data);std::string tag;int version,count;
        if(!(in>>tag>>version>>count) || tag!="MERGE2048" || version!=1 || count<0 || count>100000)return false;
        Game2048State candidate(0);int occupied=0;
        for(auto& value:candidate.m_tiles){if(!(in>>value) || (value!=0 && value!=2 && value!=4))return false;occupied+=value!=0;}
        if(occupied!=2 || !(in>>candidate.m_random))return false;
        candidate.m_score=candidate.m_moves=0;candidate.m_history.clear();
        for(int i=0;i<count;++i){int direction;if(!(in>>direction) || direction<0 || direction>3 || !candidate.move(static_cast<Direction>(direction)))return false;}
        in>>std::ws;if(!in.eof())return false;
        *this=std::move(candidate);return true;
    }
private:
    void spawn() {
        std::vector<int> empty;
        for (int i = 0; i < 16; ++i) if (!m_tiles[i]) empty.push_back(i);
        if (empty.empty()) return;
        std::uniform_int_distribution<int> cell(0, static_cast<int>(empty.size()) - 1);
        std::uniform_int_distribution<int> chance(0, 9);
        const int index = empty[cell(m_random)];
        m_tiles[index] = chance(m_random) == 0 ? 4 : 2;
    }
    struct Snapshot { Board tiles; int score; int moves; std::mt19937 random; Direction direction; };
    Board m_tiles{};
    int m_score = 0;
    int m_moves = 0;
    std::mt19937 m_random;
    std::vector<Snapshot> m_history;
};
