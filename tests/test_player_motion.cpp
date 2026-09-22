#include <cassert>
#include <cmath>
#include "UI/HeldMoveState.h"
#include "UI/PlayerMotion.h"

int main() {
    HeldMoveState held;
    int heldDx = 0;
    int heldDy = 0;
    assert(!held.current(heldDx, heldDy));
    held.press(0, -1);
    assert(held.current(heldDx, heldDy) && heldDx == 0 && heldDy == -1);
    held.press(1, 0);
    assert(held.current(heldDx, heldDy) && heldDx == 1 && heldDy == 0);
    held.release(1, 0);
    assert(held.current(heldDx, heldDy) && heldDx == 0 && heldDy == -1);
    held.release(0, -1);
    assert(!held.current(heldDx, heldDy));

    PlayerMotionState motion;
    motion.snapTo(2, 3);
    assert(motion.x() == 2.0f && motion.y() == 3.0f);
    motion.begin(3, 3, 180.0f);
    assert(motion.isMoving());
    motion.advance(100);
    assert(std::fabs(motion.progress() - 0.30f) < 0.001f);
    // 连续移动采用恒速插值，避免每个格子边界因减速而产生卡顿。
    assert(std::fabs(motion.x() - 2.300f) < 0.002f);
    // A calmer 75 ms cadence avoids rapid left/right flicker in 8-frame art.
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
    motion.advance(50);
    // The walk cycle continues across tile boundaries instead of restarting.
    assert(motion.walkingFrame(8) == 3);

    // A long render frame can cross a tile boundary.  Preserve the unused
    // time so the next grid step starts within the same visual frame instead
    // of pausing for one timer tick on every tile edge.
    motion.snapTo(2, 3);
    assert(motion.beginGridStep(2, 3, 3, 3, 360.0f));
    const float overflow = motion.advanceWithOverflow(180.0f);
    assert(!motion.isMoving());
    assert(std::fabs(overflow - (180.0f - 1000.0f / 6.0f)) < 0.01f);
    assert(motion.beginGridStep(3, 3, 4, 3, 360.0f));
    motion.advanceWithOverflow(overflow);
    assert(motion.isMoving());
    assert(motion.x() > 3.04f && motion.x() < 3.10f);
    assert(motion.walkingFrame(8) == 2);

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
