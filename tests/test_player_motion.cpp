#include <cassert>
#include <cmath>
#include "UI/PlayerMotion.h"

int main() {
    PlayerMotionState motion;
    motion.snapTo(2, 3);
    assert(motion.x() == 2.0f && motion.y() == 3.0f);
    motion.begin(3, 3, 180.0f);
    assert(motion.isMoving());
    motion.advance(100);
    assert(std::fabs(motion.progress() - 0.30f) < 0.001f);
    assert(std::fabs(motion.x() - 2.216f) < 0.002f);
    assert(motion.walkingFrame() == 1);
    assert(motion.isMoving());
    motion.advance(240);
    assert(!motion.isMoving());
    assert(motion.x() == 3.0f && motion.y() == 3.0f);
    motion.begin(3, 2, 180.0f);
    motion.advance(500);
    assert(!motion.isMoving());
    assert(motion.x() == 3.0f && motion.y() == 2.0f);

    // 动画完成后状态立即可用于开始下一格。
    motion.snapTo(2, 3);
    motion.beginGridStep(2, 3, 3, 3, 260.0f);
    motion.advance(250);
    assert(!motion.isMoving());
    assert(motion.beginGridStep(3, 3, 4, 3, 260.0f));
    assert(motion.isMoving());

    // 游戏位置始终按格计算：只有相邻格才播放移动动画，传送等跨格变化直接吸附。
    motion.snapTo(2, 3);
    assert(motion.beginGridStep(2, 3, 3, 3, 180.0f));
    assert(motion.isMoving());
    motion.snapTo(2, 3);
    assert(!motion.beginGridStep(2, 3, 4, 3, 180.0f));
    assert(!motion.isMoving());
    assert(motion.x() == 4.0f && motion.y() == 3.0f);
    return 0;
}
