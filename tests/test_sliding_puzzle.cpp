#include <QApplication>
#include <QPushButton>
#include <QCheckBox>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QSettings>
#include <QEventLoop>
#include <QTimer>
#include <cassert>
#include <algorithm>
#include "Game/SlidingPuzzleState.h"
#include "UI/SlidingPuzzlePage.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    for (int n = 3; n <= 8; ++n) {
        for (unsigned seed = 1; seed <= 16; ++seed) {
            SlidingPuzzleState state(n);
            assert(state.solved());
            state.shuffle(seed);
            assert(!state.solved() && state.moves() == 0);
            auto sorted = state.tiles();
            std::sort(sorted.begin(), sorted.end());
            for (int i = 0; i < n * n; ++i) assert(sorted[i] == i);
            int inversions = 0;
            for (int i = 0; i < n*n; ++i)
                for (int j = i + 1; j < n*n; ++j)
                    if (state.tiles()[i] && state.tiles()[j] && state.tiles()[i] > state.tiles()[j]) ++inversions;
            assert(n % 2 ? inversions % 2 == 0 : (inversions + n - state.blank()/n) % 2 == 1);
            const auto before = state.tiles();
            assert(!state.move(state.blank()));
            for (int i = 0; i < n*n; ++i) if (state.canMove(i)) {
                assert(state.move(i)); assert(state.moves() == 1);
                assert(state.undo()); assert(state.tiles() == before); break;
            }
        }
    }
    SlidingPuzzlePage page;
    page.setAttribute(Qt::WA_DontShowOnScreen);
    page.resize(1100, 760);
    page.show();
    app.processEvents();
    auto* board = page.findChild<SlidingPuzzleBoard*>();
    assert(board);
    for (int n = 3; n <= 8; ++n) {
        auto* button = page.findChild<QPushButton*>(QStringLiteral("puzzleDifficulty%1").arg(n));
        assert(button); button->click(); assert(board->state().size() == n);
    }
    assert(page.grab().save("build/puzzle_preview_1100.png"));
    page.resize(1600, 900);
    page.findChild<QPushButton*>("puzzleDifficulty3")->click();
    app.processEvents();
    assert(page.grab().save("build/puzzle_preview_1600.png"));
    const auto tilesBefore = board->state().tiles();
    const int oldBlank = board->state().blank();
    const int direction = oldBlank % 3 > 0 ? Qt::Key_Right : Qt::Key_Left;
    const int expectedBlank = oldBlank + (direction == Qt::Key_Right ? -1 : 1);
    QKeyEvent key(QEvent::KeyPress, direction, Qt::NoModifier);
    QApplication::sendEvent(board, &key);
    assert(board->state().moves() == 1);
    assert(board->state().blank() == expectedBlank);
    QEventLoop animationWait;
    QTimer::singleShot(200, &animationWait, &QEventLoop::quit);
    animationWait.exec();
    page.findChild<QPushButton*>("puzzleUndo")->click();
    assert(board->state().tiles() == tilesBefore);
    auto* difficultyButton = page.findChild<QPushButton*>("puzzleDifficulty3");
    difficultyButton->setFocus();
    QKeyEvent focusedKey(QEvent::KeyPress, direction, Qt::NoModifier);
    QApplication::sendEvent(difficultyButton, &focusedKey);
    assert(board->state().moves() == 1);
    assert(board->state().blank() == expectedBlank);
    QEventLoop secondAnimation;
    QTimer::singleShot(200, &secondAnimation, &QEventLoop::quit);
    secondAnimation.exec();
    const int verticalBlank = board->state().blank();
    const int verticalKey = verticalBlank / 3 < 2 ? Qt::Key_Up : Qt::Key_Down;
    const int expectedVerticalBlank = verticalBlank + (verticalKey == Qt::Key_Up ? 3 : -3);
    auto* numbers = page.findChild<QCheckBox*>();
    numbers->setFocus();
    QKeyEvent verticalPress(QEvent::KeyPress, verticalKey, Qt::NoModifier);
    QApplication::sendEvent(numbers, &verticalPress);
    assert(board->state().moves() == 2);
    assert(board->state().blank() == expectedVerticalBlank);
    bool returned = false;
    QObject::connect(&page, &SlidingPuzzlePage::returnToMenu, [&]() { returned = true; });
    page.findChild<QPushButton*>("puzzleBackButton")->click();
    assert(returned);
    return 0;
}
