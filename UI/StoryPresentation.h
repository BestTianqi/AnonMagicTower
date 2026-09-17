#pragma once

#include <algorithm>

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
