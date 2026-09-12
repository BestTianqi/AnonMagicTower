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
    assert(std::fabs(motion.x() - 2.30f) < 0.001f);
    assert(motion.isMoving());
    motion.advance(240);
    assert(!motion.isMoving());
    assert(motion.x() == 3.0f && motion.y() == 3.0f);
    motion.begin(3, 2, 180.0f);
    motion.advance(500);
    assert(!motion.isMoving());
    assert(motion.x() == 3.0f && motion.y() == 2.0f);
    return 0;
}
