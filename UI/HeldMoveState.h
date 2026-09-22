#pragma once

#include <algorithm>
#include <cstdlib>
#include <vector>

// Tracks physical direction keys independently from the operating system's
// key-repeat delay.  The most recently pressed direction wins; releasing it
// naturally falls back to another direction that is still held.
class HeldMoveState {
public:
    void press(int dx, int dy) {
        if (std::abs(dx) + std::abs(dy) != 1) return;
        release(dx, dy);
        m_directions.push_back({dx, dy});
    }

    void release(int dx, int dy) {
        m_directions.erase(
            std::remove_if(m_directions.begin(), m_directions.end(),
                           [dx, dy](const Direction& direction) {
                               return direction.dx == dx && direction.dy == dy;
                           }),
            m_directions.end());
    }

    bool current(int& dx, int& dy) const {
        if (m_directions.empty()) return false;
        dx = m_directions.back().dx;
        dy = m_directions.back().dy;
        return true;
    }

    void clear() { m_directions.clear(); }

private:
    struct Direction {
        int dx;
        int dy;
    };
    std::vector<Direction> m_directions;
};
