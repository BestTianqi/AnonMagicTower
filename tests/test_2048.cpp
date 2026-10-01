#include "Game/Game2048State.h"
#include "UI/Game2048Page.h"
#include <QApplication>
#include <QDir>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMediaPlayer>
#include <QComboBox>
#include <QEventLoop>
#include <QFile>
#include <QTimer>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include "Audio/GameAudio.h"
#include "UI/MiniGameSession.h"
#include <cassert>
#include <numeric>

int main(int argc, char** argv) {
    using State = Game2048State;
    using Direction = State::Direction;
    const auto sum = [](const State::Board& board) { return std::accumulate(board.begin(), board.end(), 0); };
    const auto slide = State::slide({2, 2, 2, 2, 4, 4, 8, 0, 0, 2, 0, 2, 0, 0, 0, 0}, Direction::Left);
    assert((slide.tiles == State::Board{4, 4, 0, 0, 8, 8, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0}));
    assert(slide.gain == 20);
    const auto right = State::slide({2, 2, 2, 0}, Direction::Right);
    assert((right.tiles == State::Board{0, 0, 2, 4}));
    const auto up = State::slide({2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2}, Direction::Up);
    assert((up.tiles == State::Board{4, 0, 0, 0, 4}));
    const auto down = State::slide({2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2}, Direction::Down);
    assert((down.tiles == State::Board{0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 4}));
    assert(State::slide({1024, 1024}, Direction::Left).tiles[0] == 2048);
    assert(State::slide({2048, 2048}, Direction::Left).tiles[0] == 4096);
    assert(State::slide({4096, 4096}, Direction::Left).tiles[0] == 8192);
    assert(State::slide({8192, 8192}, Direction::Left).tiles[0] == 16384);
    assert(State::slide({65536, 65536}, Direction::Left).tiles[0] == 131072);
    State::Board locked{2,4,2,4,4,2,4,2,2,4,2,4,4,2,4,2};
    assert(!State::hasMoves(locked));
    locked[0] = 4;
    assert(State::hasMoves(locked));

    for (unsigned seed = 1; seed <= 20; ++seed) {
        State state(seed);
        assert(std::count(state.tiles().begin(), state.tiles().end(), 0) == 14);
        for (int turn = 0; turn < 120; ++turn) {
            const auto direction = static_cast<Direction>(turn % 4);
            const auto before = state.tiles();
            const auto score = state.score();
            const auto moves = state.moves();
            const auto result = State::slide(before, direction);
            assert(sum(result.tiles) == sum(before));
            const bool moved = state.move(direction);
            assert(moved == (result.tiles != before));
            if (!moved) {
                assert(state.tiles() == before && state.score() == score && state.moves() == moves);
                continue;
            }
            assert(state.score() == score + result.gain && state.moves() == moves + 1);
            assert(sum(state.tiles()) - sum(before) == 2 || sum(state.tiles()) - sum(before) == 4);
            int changedCells = 0;
            for (int i = 0; i < 16; ++i) if (state.tiles()[i] != result.tiles[i]) {
                ++changedCells;
                assert(result.tiles[i] == 0 && (state.tiles()[i] == 2 || state.tiles()[i] == 4));
            }
            assert(changedCells == 1);
            const auto after = state.tiles();
            assert(state.undo());
            assert(state.tiles() == before && state.score() == score && state.moves() == moves);
            assert(state.move(direction) && state.tiles() == after);
        }
        while (state.canUndo()) assert(state.undo());
        assert(state.moves() == 0 && state.score() == 0);
    }

    QApplication app(argc, argv);
    QTemporaryDir settingsDirectory(QDir::currentPath() + "/build/2048-settings-XXXXXX");
    assert(settingsDirectory.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "MyGO-Mota", "MyGO-Mota");
    settings.setValue("movementAnimation", false);
    for (const QString& cue : {"menu_select", "step", "door", "pickup", "battle",
                               "victory", "blocked", "stairs", "dialogue", "shop",
                               "puzzle_slide", "undo", "puzzle_solved", "merge_move",
                               "merge_small", "merge_large", "merge_goal"}) {
        QFile asset(QStringLiteral(":/Audio/sfx/%1.wav").arg(cue));
        assert(asset.open(QIODevice::ReadOnly) && asset.size() > 1000);
        const QByteArray header = asset.read(12);
        assert(header.startsWith("RIFF") && header.mid(8, 4) == "WAVE");
    }
    QFile music(QStringLiteral(":/Audio/music/haruhikage_8bit.wav"));
    assert(music.open(QIODevice::ReadOnly) && music.size() > 1000000);
    const QByteArray musicHeader = music.read(44);
    assert(musicHeader.startsWith("RIFF") && musicHeader.mid(8, 4) == "WAVE");
    QFile secondMusic(QStringLiteral(":/Audio/music/killkiss_8bit.wav"));
    assert(secondMusic.open(QIODevice::ReadOnly) && secondMusic.size() > 1000000);
    assert(secondMusic.read(12).startsWith("RIFF"));
    GameAudio::prepare();
    auto* musicPlayer = app.findChild<QMediaPlayer*>();
    assert(musicPlayer && musicPlayer->source().toString() ==
        QStringLiteral("qrc:/Audio/music/haruhikage_8bit.wav"));
    GameAudio::startMiniGameMusic();
    QEventLoop musicStartup;
    QTimer::singleShot(400, &musicStartup, &QEventLoop::quit);
    musicStartup.exec();
    assert(musicPlayer->error() == QMediaPlayer::NoError);
    GameAudio::setMiniGameMusicTrack(1);
    assert(GameAudio::miniGameMusicTrack() == 1);
    assert(musicPlayer->source().toString() ==
        QStringLiteral("qrc:/Audio/music/killkiss_8bit.wav"));
    QEventLoop secondMusicStartup;
    QTimer::singleShot(400, &secondMusicStartup, &QEventLoop::quit);
    secondMusicStartup.exec();
    assert(musicPlayer->error() == QMediaPlayer::NoError);
    GameAudio::setMiniGameMusicTrack(0);
    GameAudio::stopMiniGameMusic();
    for (int i = 0; i < Game2048Board::CharacterCount; ++i) {
        assert(!Game2048Board::characterImage(i).isNull());
        assert(Game2048Board::characterName(i) != QString::fromUtf8("弦卷心"));
    }
    assert(Game2048Board::characterName(8) == QString::fromUtf8("藤都子"));
    assert(Game2048Board::characterName(11) == QString::fromUtf8("凑友希那"));
    Game2048Page page;
    page.setAttribute(Qt::WA_DontShowOnScreen);
    page.resize(1100, 760); page.show(); app.processEvents();
    auto* musicChoice = page.findChild<QComboBox*>("mergeMusicChoice");
    assert(musicChoice && musicChoice->count() == 2 && musicChoice->currentIndex() == 0);
    musicChoice->setCurrentIndex(1);
    assert(GameAudio::miniGameMusicTrack() == 1);
    assert(musicPlayer->source().toString() ==
        QStringLiteral("qrc:/Audio/music/killkiss_8bit.wav"));
    musicChoice->setCurrentIndex(0);
    auto* board = page.findChild<Game2048Board*>(); assert(board);
    const auto beforeMusicKey = board->state().tiles();
    QKeyEvent chooseMusic(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    QApplication::sendEvent(musicChoice, &chooseMusic);
    assert(musicChoice->currentIndex() == 1);
    assert(board->state().tiles() == beforeMusicKey);
    musicChoice->setCurrentIndex(0);
    auto* restart = page.findChild<QPushButton*>("mergeRestart"); assert(restart);
    const int keys[] = {Qt::Key_Left, Qt::Key_Right, Qt::Key_Up, Qt::Key_Down};
    for (int direction = 0; direction < 4; ++direction) {
        const auto before = board->state().tiles();
        if (State::slide(before, static_cast<Direction>(direction)).tiles == before) continue;
        restart->setFocus();
        QKeyEvent press(QEvent::KeyPress, keys[direction], Qt::NoModifier);
        QApplication::sendEvent(restart, &press);
        assert(board->state().moves() == 1);
        QKeyEvent undo(QEvent::KeyPress, Qt::Key_Z, Qt::ControlModifier);
        QApplication::sendEvent(restart, &undo);
        assert(board->state().tiles() == before && board->state().moves() == 0);
        break;
    }
    for (int turn = 0; turn < 120; ++turn) board->move(static_cast<Direction>(turn % 4));
    assert(board->state().score() > 0);
    assert(settings.value("miniGames/2048/bestScore").toInt() == board->state().score());
    app.processEvents();
    assert(page.size() == QSize(1100, 760));
    assert(page.grab().save("build/2048_preview_1100.png"));
    page.resize(1600, 900); app.processEvents();
    assert(page.grab().save("build/2048_preview_1600.png"));
    restart->click();
    assert(board->state().moves() == 0 && board->state().score() == 0 && !board->state().canUndo());
    settings.setValue("movementAnimation", true);
    for (int direction = 0; direction < 4; ++direction) {
        const auto before = board->state().tiles();
        if (State::slide(before, static_cast<Direction>(direction)).tiles == before) continue;
        const QPointF deltas[] = {{-80, 0}, {80, 0}, {0, -80}, {0, 80}};
        const QPointF start(board->rect().center());
        QMouseEvent press(QEvent::MouseButtonPress, start, board->mapToGlobal(start.toPoint()),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease, start + deltas[direction],
                            board->mapToGlobal((start + deltas[direction]).toPoint()),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(board, &press); QApplication::sendEvent(board, &release);
        assert(board->state().moves() == 1);
        const auto duringSlide = board->grab().toImage();
        QEventLoop wait; QTimer::singleShot(190, &wait, &QEventLoop::quit); wait.exec();
        assert(board->state().moves() == 1);
        assert(duringSlide != board->grab().toImage());
        board->undo(); assert(board->state().tiles() == before);
        break;
    }
    for(int turn=0;turn<8;++turn){board->move(static_cast<Direction>(turn%4));QEventLoop wait;QTimer::singleShot(160,&wait,&QEventLoop::quit);wait.exec();}
    GameAudio::startMiniGameMusic();
    QEventLoop audioReady;QTimer::singleShot(150,&audioReady,&QEventLoop::quit);audioReady.exec();
    auto* pause=page.findChild<QPushButton*>("mergePause");assert(pause);pause->click();assert(pause->isChecked());
    assert(musicPlayer->playbackState()==QMediaPlayer::PausedState);
    const auto saved=board->snapshot();
    for(int i=0;i<4;++i)assert(!board->move(static_cast<Direction>(i)));
    board->undo();assert(board->snapshot()==saved);
    page.findChild<QPushButton*>("mergeSave")->click();restart->click();assert(board->state().moves()==0);
    page.findChild<QPushButton*>("mergeLoad")->click();assert(board->snapshot()==saved && pause->isChecked());
    const auto beforeUndo=board->state().moves();pause->click();board->undo();assert(board->state().moves()==beforeUndo-1);
    QEvent deactivate(QEvent::WindowDeactivate);QApplication::sendEvent(&page,&deactivate);assert(pause->isChecked());
    const auto autoState=board->snapshot();
    bool returned = false;
    QObject::connect(&page, &Game2048Page::returnToMenu, [&]() { returned = true; });
    page.findChild<QPushButton*>("mergeBack")->click(); assert(returned);
    {
        Game2048Page reopened;
        assert(reopened.findChild<Game2048Board*>()->snapshot()==autoState);
        assert(reopened.findChild<QPushButton*>("mergePause")->isChecked());
    }
    page.resize(1100,760);app.processEvents();assert(page.grab().save("build/2048_session_preview.png"));
}
