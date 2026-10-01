#pragma once

#include <algorithm>
#include <string>
#include <string_view>

// Speaking changes an expression, not the other character's identity.
class StoryPortraitStage {
public:
    explicit StoryPortraitStage(std::string_view protagonist) : m_left(protagonist) {}
    void observe(bool playerSpeaking, std::string_view portrait) {
        if (portrait.empty()) return;
        (playerSpeaking ? m_left : m_right) = portrait;
    }
    const std::string& left() const { return m_left; }
    const std::string& right() const { return m_right; }
private:
    std::string m_left;
    std::string m_right;
};

enum class StoryBackdropSource {
    ExplicitCg,
    SceneSnapshot
};

inline StoryBackdropSource storyBackdropSource(bool hasExplicitCg)
{
    return hasExplicitCg ? StoryBackdropSource::ExplicitCg
                         : StoryBackdropSource::SceneSnapshot;
}

inline bool shouldShowFirstFloorOpening(bool isNewGame, int floor, bool alreadyShown)
{
    return isNewGame && floor == 1 && !alreadyShown;
}

class StoryCompletionPolicy {
public:
    explicit StoryCompletionPolicy(bool mandatory) : m_mandatory(mandatory) {}

    bool canReject(bool finished) const { return !m_mandatory || finished; }

private:
    bool m_mandatory = false;
};

class StoryPager {
public:
    explicit StoryPager(int pageCount)
        : m_pageCount(std::max(0, pageCount))
    {
    }

    int page() const { return m_page; }
    bool finished() const { return m_finished; }

    // 返回 true 表示切换到下一页；false 表示本次点击结束剧情。
    bool advance()
    {
        if (m_finished || m_pageCount <= 0) {
            m_finished = true;
            return false;
        }
        if (m_page + 1 < m_pageCount) {
            ++m_page;
            return true;
        }
        m_finished = true;
        return false;
    }

private:
    int m_pageCount = 0;
    int m_page = 0;
    bool m_finished = false;
};
