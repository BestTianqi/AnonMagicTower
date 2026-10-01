#include "UI/StoryScript.h"
#include "UI/StoryPresentation.h"

#include <cassert>
#include <fstream>
#include <iterator>
#include <set>
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
    // Localized equipment and actual dark-wall behavior must match the game.
    assert(joined(7).find("拨片") != std::string::npos);
    assert(joined(8).find("谱架") != std::string::npos);
    assert(joined(23).find("撞开") != std::string::npos);
    // The finale explains the AU's separate stage image, not a second real Soyo.
    assert(joined(28).find("塔") != std::string::npos);
    assert(joined(31).find("排练") != std::string::npos);
    const auto withContext = [](int scene, const StoryContext& context) {
        std::string result;
        for (const auto& line : storyScene(scene, context)) result += line.text;
        return result;
    };
    StoryContext captured;
    captured.metMichelle = captured.michelleCaptured = true;
    assert(withContext(19, captured).find("还在二层右下角") != std::string::npos);
    assert(withContext(19, captured).find("我确实回去了") == std::string::npos);
    StoryContext rescued = captured;
    rescued.michelleRescued = true;
    assert(withContext(19, rescued).find("我确实回去了") != std::string::npos);
    assert(withContext(19, rescued).find("还在二层右下角") == std::string::npos);
    assert(withContext(19, {}).find("牢笼") == std::string::npos);
    StoryContext summit = rescued;
    summit.promisedSummit = true;
    assert(withContext(27, summit).find("说好了") != std::string::npos);
    assert(withContext(27, {}).find("说好了") == std::string::npos);
    assert(withContext(27, {}).find("请问") != std::string::npos);
    assert(withContext(27, captured).find("原来你在这里") != std::string::npos);
    assert(joined(25).find("关掉聚光灯") == std::string::npos); // No invented player action.

    std::ifstream resourceFile("resources.qrc");
    assert(resourceFile.good());
    const std::string resources((std::istreambuf_iterator<char>(resourceFile)), {});
    std::set<std::string> anonExpressions, soyoExpressions;
    for (int flags = 0; flags < 16; ++flags) {
        const StoryContext context{bool(flags & 1), bool(flags & 2), bool(flags & 4), bool(flags & 8)};
        for (int scene = 1; scene <= 31; ++scene) {
            const auto lines = storyScene(scene, context);
            assert(lines.size() >= 2 && lines.size() <= 10);
            for (const auto& line : lines) {
                assert(line.speaker && *line.speaker && line.text && *line.text);
                const auto portrait = storyPortrait(line, scene);
                if (!portrait.empty()) assert(resources.find(portrait.substr(2)) != std::string::npos);
                if (std::string(line.speaker) == "千早爱音") anonExpressions.insert(portrait);
                if (std::string(line.speaker) == "长崎素世") soyoExpressions.insert(portrait);
                if (scene < 28) assert(std::string(line.text).find("我喜欢你") == std::string::npos);
                int characters = 0;
                for (const unsigned char ch : std::string(line.text)) if ((ch & 0xc0) != 0x80) ++characters;
                assert(characters <= 90); // Fits a short dialogue page.
            }
            const std::string cg = storySceneCg(scene);
            if (!cg.empty()) assert(resources.find(cg.substr(2)) != std::string::npos);
        }
    }
    assert(anonExpressions.size() >= 5 && soyoExpressions.size() >= 5);
    assert(storyPortrait({"长崎素世", "", StoryEmotion::Worried}, 29).find("soyo_school_") != std::string::npos);
    assert(storyPortrait({"舞台素世", "", StoryEmotion::Angry}, 29).find("soyo_witch_") != std::string::npos);
    assert(storyScene(0).empty() && storyScene(32).empty());

    StoryPortraitStage stage("anon");
    stage.observe(false, "michelle");
    stage.observe(true, "anon_worried");
    assert(stage.left() == "anon_worried" && stage.right() == "michelle");
    stage.observe(false, ""); // Narration never replaces the interlocutor.
    assert(stage.right() == "michelle");
    stage.observe(false, "soyo_shy");
    stage.observe(true, "anon_shy");
    assert(stage.left() == "anon_shy" && stage.right() == "soyo_shy");
    StoryPortraitStage solo("anon");
    solo.observe(false, "");
    assert(solo.right().empty());
    return 0;
}
