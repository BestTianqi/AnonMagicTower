#include "UI/StoryScript.h"

#include <cassert>
#include <string>

int main()
{
    for (int scene = 1; scene <= 31; ++scene)
        assert(!storyScene(scene).empty());

    const auto joined = [](int scene) {
        std::string result;
        for (const auto& line : storyScene(scene)) {
            result += line.speaker;
            result += line.text;
        }
        return result;
    };
    assert(joined(23).find("摘下") == std::string::npos);
    assert(joined(23).find("都是我") == std::string::npos);
    assert(joined(23).find("摘下头套") == std::string::npos);
    assert(joined(28).find("是我") != std::string::npos);
    assert(joined(30).find("我自己想") != std::string::npos);
    assert(joined(17).find("二层") != std::string::npos);
    assert(joined(18).find("三十五层") != std::string::npos);
    return 0;
}
