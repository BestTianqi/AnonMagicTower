#include <QApplication>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QSettings>
#include <QDir>
#include <QTemporaryDir>
#include <QEventLoop>
#include <QTimer>
#include <QLabel>
#include <QFile>
#include <QJsonDocument>
#include <QSet>
#include "UI/MiniGameSession.h"
#include <cassert>
#include <algorithm>
#include "Game/SlidingPuzzleState.h"
#include "UI/SlidingPuzzlePage.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QTemporaryDir settingsDirectory(QDir::currentPath() + "/build/puzzle-settings-XXXXXX");
    assert(settingsDirectory.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
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
    auto* musicChoice = page.findChild<QComboBox*>("puzzleMusicChoice");
    assert(musicChoice && musicChoice->count() == 2);
    musicChoice->setCurrentIndex(1);
    assert(musicChoice->currentIndex() == 1);
    musicChoice->setCurrentIndex(0);
    const auto beforeMusicKey = board->state().tiles();
    QKeyEvent chooseMusic(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    QApplication::sendEvent(musicChoice, &chooseMusic);
    assert(musicChoice->currentIndex() == 1);
    assert(board->state().tiles() == beforeMusicKey);
    musicChoice->setCurrentIndex(0);
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
    auto* images=page.findChild<QComboBox*>("puzzleImageChoice");
    assert(images && images->count()==21); // Original Anon/Soyo + 20 distinct card IDs.
    assert(images->itemData(0).toString()==":/images/backgrounds/anon_soyo_puzzle.jpg");
    QSet<QString> galleryPaths;
    for(int i=1;i<images->count();++i){
        const QString path=images->itemData(i).toString();
        assert(path.startsWith(":/images/puzzle_cards/card_"));
        QPixmap card(path);assert(!card.isNull() && card.width()>=512 && card.height()>=512);
        galleryPaths.insert(path);
    }
    assert(galleryPaths.size()==20);
    assert(images->findData(":/images/puzzle_cards/card_0947_trained.png")>0);
    assert(images->findData(":/images/backgrounds/mujica_theater.png")==-1);
    assert(images->findData(":/images/backgrounds/bangdream_gbp_cover.jpg")==-1);
    const auto imageTiles=board->state().tiles();
    images->setCurrentIndex(images->findData(":/images/puzzle_cards/card_0947_trained.png"));
    board->setPreview(true);app.processEvents();
    assert(page.grab().save("build/puzzle_card947_preview.png"));
    board->setPreview(false);
    images->setCurrentIndex(2);assert(board->state().tiles()==imageTiles);
    auto* session=page.findChild<QWidget*>("puzzleSession");assert(session);
    auto* pause=page.findChild<QPushButton*>("puzzlePause");assert(pause);
    pause->click();assert(pause->isChecked());
    const auto pausedTiles=board->state().tiles();
    QApplication::sendEvent(board,&verticalPress);
    board->undo();assert(board->state().tiles()==pausedTiles);
    auto* time=page.findChild<QLabel*>("puzzleTime");
    QEventLoop settle;QTimer::singleShot(250,&settle,&QEventLoop::quit);settle.exec();
    const QString pausedTime=time->text();
    QEventLoop frozen;QTimer::singleShot(1100,&frozen,&QEventLoop::quit);frozen.exec();
    assert(time->text()==pausedTime);
    page.findChild<QPushButton*>("puzzleSave")->click();
    const auto saved=board->snapshot();
    page.findChild<QPushButton*>("puzzleDifficulty5")->click();assert(board->state().size()==5);
    page.findChild<QPushButton*>("puzzleLoad")->click();
    assert(board->snapshot()==saved && images->currentIndex()==2 && pause->isChecked());
    assert(page.findChild<QPushButton*>("puzzleDifficulty3")->isChecked());
    QFile legacySave(MiniGameSession::storageDirectory()+"/puzzle-1.json");
    assert(legacySave.open(QIODevice::ReadOnly));
    QJsonObject legacyRoot=QJsonDocument::fromJson(legacySave.readAll()).object();legacySave.close();
    for(const QString& retired : {":/images/backgrounds/bangdream_gbp_cover.jpg",
            ":/images/backgrounds/mygo_stage.png", ":/images/backgrounds/mujica_theater.png",
            ":/images/backgrounds/yumemita_dream.png", ":/images/backgrounds/tower_hub.png"}) {
        QJsonObject payload=legacyRoot["payload"].toObject();payload["picture"]=retired;legacyRoot["payload"]=payload;
        assert(legacySave.open(QIODevice::WriteOnly|QIODevice::Truncate));
        legacySave.write(QJsonDocument(legacyRoot).toJson());legacySave.close();
        page.findChild<QPushButton*>("puzzleLoad")->click();
        assert(board->snapshot()==saved && images->currentIndex()==0 && pause->isChecked());
        assert(images->count()==21);
    }
    const int loadedMoves=board->state().moves();pause->click();board->undo();assert(board->state().moves()==loadedMoves-1);
    assert(page.importPicture(":/images/backgrounds/anon_soyo_puzzle.jpg"));
    assert(images->currentData().toString()=="custom");
    images->setCurrentIndex(1);images->setCurrentIndex(images->findData("custom"));
    page.findChild<QPushButton*>("puzzleSave")->click();
    images->setCurrentIndex(0);page.findChild<QPushButton*>("puzzleLoad")->click();
    assert(images->currentData().toString()=="custom");
    const auto intact=board->snapshot();
    QFile corrupt(MiniGameSession::storageDirectory()+"/puzzle-3.json");assert(corrupt.open(QIODevice::WriteOnly));corrupt.write("broken");corrupt.close();
    page.findChild<QComboBox*>("puzzleSaveSlot")->setCurrentIndex(2);
    page.findChild<QPushButton*>("puzzleLoad")->click();assert(board->snapshot()==intact);
    bool returned = false;
    QObject::connect(&page, &SlidingPuzzlePage::returnToMenu, [&]() { returned = true; });
    page.findChild<QPushButton*>("puzzleBackButton")->click();
    assert(returned);
    {
        SlidingPuzzlePage reopened;
        assert(reopened.findChild<SlidingPuzzleBoard*>()->snapshot()==intact);
        assert(reopened.findChild<QPushButton*>("puzzlePause")->isChecked());
        assert(reopened.findChild<QComboBox*>("puzzleImageChoice")->currentData().toString()=="custom");
    }
    page.resize(1100,760);app.processEvents();
    assert(page.grab().save("build/puzzle_session_preview.png"));
    return 0;
}
