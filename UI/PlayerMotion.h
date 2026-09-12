#pragma once

#include <algorithm>
#include <cmath>

// Presentation-only interpolation. Game::Player keeps authoritative integer tiles.
class PlayerMotionState {
public:
    void snapTo(int tileX, int tileY) {
        m_x = m_targetX = static_cast<float>(tileX);
        m_y = m_targetY = static_cast<float>(tileY);
        m_elapsedMs = 0.0f;
        m_durationMs = 0.0f;
    }

    void begin(int tileX, int tileY, float pixelsPerSecond) {
        m_startX = m_x;
        m_startY = m_y;
        m_targetX = static_cast<float>(tileX);
        m_targetY = static_cast<float>(tileY);
        const float distanceTiles = std::hypot(m_targetX - m_x, m_targetY - m_y);
        m_elapsedMs = 0.0f;
        m_durationMs = distanceTiles <= 0.0f
            ? 0.0f
            : distanceTiles * 60.0f * 1000.0f / std::max(1.0f, pixelsPerSecond);
        if (m_durationMs == 0.0f) {
            m_x = m_targetX;
            m_y = m_targetY;
        }
    }

    bool advance(float elapsedMs) {
        if (!isMoving()) return false;
        m_elapsedMs = std::min(m_durationMs, m_elapsedMs + std::max(0.0f, elapsedMs));
        const float t = m_durationMs <= 0.0f ? 1.0f : m_elapsedMs / m_durationMs;
        m_x = m_startX + (m_targetX - m_startX) * t;
        m_y = m_startY + (m_targetY - m_startY) * t;
        if (m_elapsedMs >= m_durationMs) {
            m_x = m_targetX;
            m_y = m_targetY;
        }
        return true;
    }

    bool isMoving() const { return m_elapsedMs < m_durationMs; }
    float x() const { return m_x; }
    float y() const { return m_y; }

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_startX = 0.0f;
    float m_startY = 0.0f;
    float m_targetX = 0.0f;
    float m_targetY = 0.0f;
    float m_elapsedMs = 0.0f;
    float m_durationMs = 0.0f;
};
