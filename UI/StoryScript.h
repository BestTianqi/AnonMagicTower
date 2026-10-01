#pragma once

#include <string>
#include <string_view>
#include <vector>

// 原创同人 AU。编号及已看标记保持稳定，文本不负责结算地图机关。
enum class StoryEmotion { Calm, Angry, Surprised, Confident, Worried, Sad, Happy, Shy };

struct StoryLine {
    const char* speaker;
    const char* text;
    StoryEmotion emotion = StoryEmotion::Calm;
};

// 从既有剧情标记构造，不增加存档字段，也不替玩家完成任何任务。
struct StoryContext {
    bool metMichelle = false;
    bool michelleCaptured = false;
    bool michelleRescued = false;
    bool promisedSummit = false;
};

inline std::vector<StoryLine> storyScene(int scene, const StoryContext& context = {})
{
    using E = StoryEmotion;
    switch (scene) {
    case 1: return {
        {"旁白", "RiNG 的排练结束了。爱音收好吉他，发现素世没有像往常那样等在门边。"},
        {"千早爱音", "刚才还在说我进拍太早，自己倒先走了……至少等我把包拉上啊。", E::Worried},
        {"旁白", "手机亮了一下。没有署名的留言只有半句：『下次排练，我……』。后台的门后，楼梯一直延伸进黑暗。"},
        {"千早爱音", "群里灯还在问我们到家没有。先回她——咦，发不出去？", E::Surprised},
        {"千早爱音", "素世！你的话还没说完呢。……好高，真的要爬啊。", E::Worried}
    };
    case 2: return {
        {"千早爱音", "粉色的熊……？先说好，我今天可没力气参加什么整蛊节目。", E::Surprised},
        {"米歇尔", "这里没有节目。你右边的鞋带散了，先系好。", E::Worried},
        {"千早爱音", "哦，谢谢。你有没有见过一个浅棕色长发、说话有点不坦率的人？"},
        {"米歇尔", "……五层有铁剑，对应爱音拨片；九层有铁盾，对应素世谱架。先拿到它们。"},
        {"千早爱音", "我在问人，你却把装备路线都报出来了。你是不是知道些什么？", E::Worried},
        {"米歇尔", "知道你这样闯过去会受伤。左边的路替你打开了，别再踩到鞋带。"}
    };
    case 3: return {
        {"旁白", "墙后传来一小段贝斯声。同一个地方，停了两次。"},
        {"千早爱音", "这个停法……素世？我听见了。刚才排练的事，我还没说完。", E::Worried},
        {"旁白", "最后一个音被截断。走廊里只剩爱音的脚步。"}
    };
    case 4: return {
        {"旁白", "暗墙裂开。素世站在前方，四名魔法警卫同时封住了爱音的退路。"},
        {"千早爱音", "终于找到了……你这身衣服是怎么回事？", E::Surprised},
        {"长崎素世", "回去。这里不是你随便推开门就能进来的地方。", E::Angry},
        {"千早爱音", "你突然走掉，我当然会来找啊。刚才我说『下次肯定没问题』，你就一直不说话。", E::Worried},
        {"长崎素世", "你总把『下次』说得那么轻松。好像我只要点头，就一定还会有下一次。", E::Angry},
        {"千早爱音", "……我不知道你在担心这个。那我们先——", E::Surprised},
        {"长崎素世", "别过来！", E::Worried},
        {"旁白", "警卫的面具下闪过藤都子的脸。法阵合拢，素世伸出的手停在半空。"}
    };
    case 5: return {
        {"旁白", "爱音跌回二层。伤口发烫，攻击与防御都只剩十。铁栏另一边传来闷闷的声音。"},
        {"米歇尔", "先别站起来。能听清我说话吗？", E::Worried},
        {"千早爱音", "能。还听见你在叹气……痛的又不是你。", E::Sad},
        {"米歇尔", "痛就说痛。又没人要你现在跑回三层。", E::Worried},
        {"千早爱音", "嗯。真的很痛。……等缓过来，我要问她，为什么最后又朝我伸手。", E::Worried}
    };
    case 6: return {
        {"弦卷心", "欢迎！今天想为哪一段冒险做准备？"},
        {"千早爱音", "有没有爬完楼梯也不会腿软的套餐？预算……你先别看我的钱包。", E::Worried},
        {"弦卷心", "那就从你现在最需要的开始！笑不出来的时候，也可以先休息。"},
        {"千早爱音", "……那我先把伤养好。总不能见到她的第一句又是『救命』吧。", E::Happy}
    };
    case 7: return {
        {"旁白", "爱音拿起拨片。边缘贴着一小块防滑胶，手指刚好能按住。"},
        {"长崎素世〔回忆〕", "拿不稳就慢一点。又没让你今天全部练会。"},
        {"千早爱音", "说得凶，胶倒是贴得挺仔细。", E::Shy},
        {"千早爱音", "下次我会先把那一小节练好。不是随口说的。", E::Confident}
    };
    case 8: return {
        {"旁白", "素世谱架的夹子上，贴着一张写了小节数的便签。字迹整齐，最后一笔压得很重。"},
        {"千早爱音", "原来你把我老弹错的地方都记了。还说只是顺便……", E::Shy},
        {"千早爱音", "等回去，能不能再陪我练一次？这次卡住了我就直接说。", E::Worried}
    };
    case 9: return {
        {"旁白", "上下花门封住通路。第三、第四排的八名侧翼守卫留在原位，海铃退到房间尽头。"},
        {"八幡海铃", "先看两侧。八名守卫都倒下，上下的门才会开。"},
        {"千早爱音", "你不是来拦我的吗，怎么连解法都讲？", E::Surprised},
        {"八幡海铃", "你一直盯着前面，连自己快被围住都没发现。"},
        {"千早爱音", "……好。我先看清楚，再过去找你。", E::Confident}
    };
    case 10: return {
        {"八幡海铃", "最后那一下，你没有急着抢。"},
        {"千早爱音", "我在数拍子。有人念了我一下午，终于派上用场了。", E::Happy},
        {"八幡海铃", "那就把这句话告诉她。补给在这里，往上的路也开了。"},
        {"千早爱音", "会的。不过她听见以后，肯定先说『我早就讲过了』。", E::Happy}
    };
    case 11: return {
        {"旁白", "左上角三名守卫倒下后，原来封死的一格墙退开，红钥匙落在新露出的地面上。"},
        {"千早爱音", "原来藏在这里。先收好，这次可不能走到门前才翻包。", E::Confident}
    };
    case 12: return {
        {"米歇尔", "胳膊怎么了？别把那边袖子往后藏。", E::Worried},
        {"千早爱音", "蹭了一下……好吧，不止一下。你怎么每次都看得出来？", E::Shy},
        {"米歇尔", "因为你每次都说『没事』。左边这面暗墙，我来处理。"},
        {"千早爱音", "谢谢。出去以后请你喝点东西，真的，不拿这个换情报。", E::Happy},
        {"米歇尔", "先记着吧。我要去二十九层，你别为了赶上我又摔一次。"}
    };
    case 13: return {
        {"旁白", "MyGO和解徽章上挤着五个人的名字。爱音用拇指擦掉凹槽里的灰。"},
        {"千早爱音", "灯肯定又在群里问了……还有立希，一定已经开始生气。", E::Worried},
        {"千早爱音", "素世，我们回去还得一起挨念呢。你可别只留我一个。", E::Shy}
    };
    case 14: return {
        {"旁白", "红门化作花门。九只蝙蝠在中央合拢，凑友希那从飞散的影子中睁开眼。"},
        {"凑友希那", "呼吸乱了。你是要往前，还是只想赶紧结束？"},
        {"千早爱音", "有区别吗？……有。我只是很想快点见到她。", E::Worried},
        {"凑友希那", "那就站稳。想见的人不会替你撑完这一段。"},
        {"千早爱音", "知道了。我自己也能走过去。", E::Confident}
    };
    case 15: return {
        {"户山香澄", "这里的回声好奇怪！每次唱到最后一句，就像有人把声音收走了。"},
        {"千早爱音", "我的手机也是。那条留言，总是停在同一个字。", E::Worried},
        {"户山香澄", "去二十六层看看吧。大家说那里住着公主，也许她知道红门后面藏的路。"},
        {"千早爱音", "公主？……先去问本人好了。", E::Surprised}
    };
    case 16: return {
        {"公主娃娃", "你终于来了。接下来，只要说『我会带你离开』……"},
        {"千早爱音", "等等。那是你的回答，还是背好的台词？", E::Worried},
        {"旁白", "娃娃胸口的发条轻响。那句开场白又从头念了一遍。"},
        {"千早爱音", "……怪不得。素世要是听我擅自这么说，早就打断我了。", E::Sad},
        {"公主娃娃", "二十四层红门后的隐藏通道已经显现。通往五十层。请不要让她继续等候。"}
    };
    case 17: return {
        {"米歇尔", "这回倒是记得带着补给。"},
        {"千早爱音", "当然。我还欠你一杯饮料，得活着出去付账嘛。", E::Happy},
        {"旁白", "米歇尔拨开下方的墙缝。门后突然响起铁链声，一束灯光罩住了粉色身影。"},
        {"米歇尔", "别跟着跳！二层右下角……那里的牢笼！", E::Surprised},
        {"千早爱音", "听到了！你别乱动，我去找路！", E::Worried}
    };
    case 18: return {
        {"千早爱音", "守卫解决了。喂，还听得见吗？", E::Worried},
        {"米歇尔", "听得见。你绕这么远回来，不怕耽误找她？", E::Worried},
        {"千早爱音", "怕啊。所以跑得累死了。先伸手，你的袖子卡住了。", E::Worried},
        {"旁白", "毛绒手掌停了片刻，才搭上爱音的手。"},
        {"米歇尔", "……三十五层。我知道魔龙房间的暗道，我去那里帮你。"},
        {"千早爱音", "好。这次走之前告诉我一声。还有，饮料可没抵掉哦。", E::Happy}
    };
    case 19: {
        std::vector<StoryLine> lines = {
            {"旁白", "小小的身影沿地板走到爱音面前。她攥着袖口，仰起头时，那双眼睛熟悉得让爱音愣住。"},
            {"千早爱音", "素世……？不对，你怎么这么小。", E::Surprised},
            {"小长崎素世", "你说下次还来。要是她真的等了，你又不来了呢？", E::Angry}
        };
        if (context.michelleRescued) {
            lines.push_back({"小长崎素世", "你回二层救了那只熊。可一次算什么，下次呢？", E::Angry});
            lines.push_back({"千早爱音", "一次当然不算全部。可那天我确实回去了，你也不能替我擦掉啊。", E::Worried});
        } else if (context.michelleCaptured) {
            lines.push_back({"小长崎素世", "米歇尔还在二层右下角的牢笼里。你不是说要回去吗？", E::Angry});
            lines.push_back({"千早爱音", "……她还在等。击败中级守卫，再去和她说话。我记住了。", E::Worried});
        } else {
            lines.push_back({"小长崎素世", "你只知道往楼上跑。她要是不在那里，你就不找了吗？", E::Angry});
            lines.push_back({"千早爱音", "那就换条路找。等一下，我还没弄明白你是谁——", E::Surprised});
        }
        lines.push_back({"小长崎素世", "我才不信。你不许过去！", E::Angry});
        return lines;
    }
    case 20: return {
        {"小长崎素世", "我只是……这次没站稳！你别跟过来！", E::Angry},
        {"千早爱音", "好，不跟。你先看路，楼梯就在后面。", E::Worried},
        {"旁白", "她转身沿来路跑回黄色楼梯。爱音放下伸到一半的手。"},
        {"千早爱音", "连逞强的样子都……不，这种话还是别当面说了。", E::Sad}
    };
    case 21: return {
        {"旁白", "上下两扇花门落下，四角的守卫转了过来。"},
        {"千早爱音", "又关门！……别慌，四个角。先收拾这四名守卫，再出去。", E::Confident}
    };
    case 22: return {
        {"旁白", "两排守卫被清理后，身后传来几声清脆的落地声。钥匙出现在先前经过的通路旁。"},
        {"千早爱音", "四把黄的，一把红的。刚才要是只顾着往前冲，就全漏了。", E::Happy}
    };
    case 23: return {
        {"米歇尔", "这几面看起来都是实墙。看这里，沿着接缝按下去……"},
        {"旁白", "石面露出浅色纹路。原来封死的墙，变成了能撞开的暗墙。"},
        {"米歇尔", "本层的暗墙都显出来了。要过去，还得你自己撞开，不用钥匙。"},
        {"千早爱音", "你帮完忙就又要走？这回至少把下一站说清楚。", E::Worried},
        {"米歇尔", "五十层。……有件事，我不能再隔着这个告诉你了。", E::Worried},
        {"千早爱音", "好，我会去。别再用『没事』把我打发掉。", E::Confident}
    };
    case 24: return {
        {"小长崎素世", "我知道你会来。上次我没有准备好。", E::Angry},
        {"千早爱音", "我也没有啊。突然被一个小孩子撞过来，哪有人准备好。", E::Worried},
        {"小长崎素世", "你不生气？"},
        {"千早爱音", "生气。疼的时候也想过算了。可是……我还是想知道，她没发完的那条消息是什么。", E::Sad},
        {"小长崎素世", "知道以后，你也不一定会留下。", E::Worried},
        {"千早爱音", "你总抢在我前面，把最坏的那句替我说完。让我自己说一次吧。", E::Confident}
    };
    case 25: return {
        {"旁白", "小素世逃到走廊中央。魔女与四名警卫追上来，围住了她——和三层一模一样。"},
        {"长崎素世", "连一个人都留不住。你还想逃到哪里？", E::Angry},
        {"小长崎素世", "我一直在等……我没有走。", E::Sad},
        {"千早爱音", "够了！她已经说了，没听见吗？", E::Angry},
        {"长崎素世", "她只是留在这里的旧影子。你连这样的我也要管？", E::Worried},
        {"千早爱音", "我又不是只挑你笑的时候才想见你。", E::Worried},
        {"旁白", "魔女移开视线。灯光熄灭，她和警卫的身影渐渐散去。爱音仍站在四十二层的走廊上。"}
    };
    case 26: return {
        {"假魔王", "已经到这里了。只要答应留下，接下来就不会再辛苦。"},
        {"千早爱音", "……不对。素世会先问我为什么把袖子弄成那样，然后才骂我。", E::Worried},
        {"假魔王", "你不喜欢这个回答？我可以再说一遍。"},
        {"千早爱音", "你只是在背台词。再说多少遍，也不是她的回答。", E::Confident},
        {"旁白", "四名魔法警卫维系着王座的法阵。先击败他们，才能完成封印、削弱假魔王。"}
    };
    case 27: {
        std::vector<StoryLine> lines;
        if (context.promisedSummit) {
            lines = {
                {"千早爱音", "五十层。说好了在这里见，你这回真的没走。", E::Happy},
                {"米歇尔", "嗯。我一直在想，见到你该先说哪一句。", E::Worried}
            };
        } else if (context.metMichelle) {
            lines = {
                {"千早爱音", "原来你在这里。我还以为到塔顶也找不到你了。", E::Surprised},
                {"米歇尔", "有些话没来得及告诉你。现在……还愿意听吗？", E::Worried}
            };
        } else {
            lines = {
                {"千早爱音", "这里就是塔顶？……粉色的熊？请问，你见过长崎素世吗？", E::Surprised},
                {"米歇尔", "见过。她没发完的那条消息，也许可以现在说给你听。", E::Worried}
            };
        }
        lines.push_back({"千早爱音", "你慢慢说。这次我不抢了。", E::Worried});
        lines.push_back({"米歇尔", "先等一下。这个样子……果然还是不行。"});
        return lines;
    }
    case 28: return {
        {"旁白", "毛绒手掌扶住头套两侧。粉色熊头被放在地上，露出汗湿的浅棕色头发。"},
        {"千早爱音", "……素世？", E::Surprised},
        {"长崎素世", "是我。躲在头套里面，连关心你都不敢用自己的声音说。", E::Sad},
        {"千早爱音", "那拦着我的那个你呢？", E::Worried},
        {"长崎素世", "塔把我没说出口的话，做成了站在台上的影子。叫你回去，又怕你真的不来。", E::Worried},
        {"长崎素世", "可那也是我想过的话。不是说一句『都是塔的错』，就能让你没受过伤。对不起。", E::Sad},
        {"千早爱音", "我现在很想抱你，也很想生气。你先别替我选一个。", E::Sad},
        {"长崎素世", "……好。"},
        {"千早爱音", "还有，我不是只为了下次排练才来。素世，我喜欢你。所以你不在，我才会这么着急。", E::Shy},
        {"长崎素世", "这句不可以又说是开玩笑。……等结束了，我会回答。", E::Shy}
    };
    case 29: return {
        {"旁白", "王座上的舞台素世睁开眼。她身后的灯，仍照着那条没有发完的消息。"},
        {"舞台素世", "说出口又怎样？排练会结束，门会关上。你能留住哪一次？", E::Angry},
        {"长崎素世", "我留不住。可我还没有问过她，下次能不能陪我多待一会儿。", E::Worried},
        {"千早爱音", "那一小节我还没练会呢。本来就要留下来。", E::Shy},
        {"舞台素世", "只要不开门，就不必再问。", E::Angry},
        {"长崎素世", "爱音，把路打开。我不想再躲回去了。", E::Confident},
        {"千早爱音", "嗯。这次你可要看着我，好好听到最后。", E::Confident}
    };
    case 30: return {
        {"旁白", "王座上的影子碎成光点。灯暗下来，素世扶着台阶，一步一步走到爱音面前。"},
        {"长崎素世", "手机里那句……我想说的是，下次排练结束，你能不能再陪我一会儿。", E::Shy},
        {"千早爱音", "就这句？你让人绕这么大一圈，才听到这句？", E::Surprised},
        {"长崎素世", "……你可以生气。", E::Sad},
        {"千早爱音", "正在生气。所以饮料你请。我还要坐下，腿真的不行了。", E::Worried},
        {"长崎素世", "好。我想和你一起回去。不是因为你赢了，是我自己想。", E::Shy},
        {"旁白", "素世伸出手。这一次，爱音等她把话说完，才握住。"}
    };
    case 31: return {
        {"旁白", "RiNG 的门重新出现在面前。手机终于有了信号，灯的『到家了吗』后面，跟着立希的三个问号。"},
        {"千早爱音", "完了。我怎么解释自己去爬了个塔……", E::Worried},
        {"长崎素世", "先说我们都没事。还有，刚才那句，我还没回答。", E::Shy},
        {"千早爱音", "嗯。我听着。", E::Shy},
        {"长崎素世", "我也喜欢你。不是想让你留下才这么说。", E::Shy},
        {"千早爱音", "……你突然这么直接，我倒不知道接什么了。那，下次排练，我可以坐你旁边吗？", E::Happy},
        {"长崎素世", "你哪次不是自己把椅子搬过来的。", E::Happy},
        {"旁白", "爱音把『我们一起回来了』发进群里。素世没有催她，只把两人挨着的手，握得紧了一点。"}
    };
    default: return {};
    }
}

inline std::string storyPortrait(const StoryLine& line, int scene)
{
    const std::string_view name(line.speaker);
    const auto is = [&](std::string_view character) { return name.find(character) != name.npos; };
    const auto mood = static_cast<size_t>(line.emotion);
    const std::string base = ":/images/characters/portraits/";
    if (is("爱音")) {
        static const char* expressions[] = {"calm", "angry", "surprised", "confident", "worried", "sad", "laughing", "shy"};
        return base + "variants/anon_" + expressions[mood] + ".png";
    }
    if (is("小长崎")) {
        static const char* expressions[] = {"calm", "angry", "surprised", "excited", "caring", "sad", "happy", "happy"};
        return base + "variants/soyo_child_" + expressions[mood] + ".png";
    }
    if (is("素世") || is("假魔王")) {
        if (scene == 4 || scene == 25 || scene == 26 || is("舞台素世") || is("假魔王")) {
            static const char* expressions[] = {"calm", "angry", "surprised", "confident", "extra_thoughtful", "extra_thoughtful", "extra_warm", "extra_warm"};
            return base + "variants/soyo_witch_" + expressions[mood] + ".png";
        }
        static const char* expressions[] = {"calm", "angry", "surprised", "confident", "worried", "sad", "laughing", "shy"};
        return base + "variants/soyo_school_" + expressions[mood] + ".png";
    }
    if (is("米歇尔")) {
        static const char* expressions[] = {"caring", "confident", "surprised", "confident", "caring", "caring", "wave", "caring"};
        return base + "variants/michelle_" + expressions[mood] + ".png";
    }
    if (is("海铃")) return base + "umiri.png";
    if (is("友希那")) return base + "variants/yukina_new_stage_singing.png";
    if (is("香澄")) return base + "variants/kasumi_new_stage_guitar.png";
    if (is("弦卷心")) return base + "kokoro_shop.png";
    return {};
}

inline const char* storySceneCg(int scene)
{
    switch (scene) {
    case 1: return ":/images/runtime/cg/floor01_arrival.png";
    case 4: return ":/images/runtime/cg/floor03_ambush.png";
    case 9: case 10: return ":/images/runtime/cg/floor10_ambush.png";
    case 14: return ":/images/runtime/cg/boss20_yukina_vampire_prebattle.png";
    case 19: case 20: return ":/images/runtime/cg/floor32_child_soyo_charge.png";
    case 21: return ":/images/runtime/cg/floor33_trap.png";
    case 24: return ":/images/runtime/cg/boss40_knight_prebattle.png";
    case 25: return ":/images/runtime/cg/floor42_child_soyo_capture.png";
    case 26: return ":/images/runtime/cg/boss49_soyo_phantom_prebattle.png";
    case 28: return ":/images/runtime/cg/floor50_michelle_reveal.png";
    case 29: return ":/images/runtime/cg/boss50_soyo_final_prebattle.png";
    case 30: case 31: return ":/images/runtime/cg/soyo_defeated_ending.png";
    default: return "";
    }
}
