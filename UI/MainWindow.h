#pragma once

#include <QWidget>
#include "Game/Game.h"
#include "HeldMoveState.h"
#include "ui_MainWindow.h"
#include <QTimer>
#include <QString>
#include <vector>
#include <memory>
#include <QTemporaryFile>

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(Game* game, QWidget* parent = nullptr,
                        bool playFirstFloorOpening = false);
    void loadAssets();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    struct VisualNovelPage {
        QString speaker;
        QString text;
        QString portrait;
        QString accent;
        QString cg;
    };

    void updateHUD();
    void updateMonsterPanel();
    void activateItem(int index);
    void updateItemPanel();
    void showOpeningFloorStory(int fromFloor, int toFloor);
    std::vector<VisualNovelPage> storyPages(int scene) const;
    void showScriptSceneOnce(int scene);
    void showStoryMilestones();
    void showStoryPickup(int floor, const QString& name);
    void showFloor20VampireStoryIfNeeded(int floorBefore);
    void showFloor10AmbushStoryIfNeeded();
    void showFloor33TrapStoryIfNeeded(int floorBefore);
    void showFloor32KnightStoryIfNeeded(int floorBefore);
    void showFloor32KnightStoryAfterMovement(int floorBefore);
    void showFloor42CaptureStory();
    void showPendingApproachHazardCgs();
    void showPrisonTrapPrompt();
    void showFloor3PrisonVisualNovel();
    bool showVisualNovelDialogue(const std::vector<VisualNovelPage>& pages, bool mandatory = false);
    bool runBossBattleAt(int x, int y, bool floor32FirstStrike = false);
    void showFirstFloorOpeningStory();
    bool showVisualNovelChoice(const QString& speaker, const QString& text,
                               const QString& portrait, const QString& yesText = QString::fromUtf8("确定"),
                               const QString& noText = QString::fromUtf8("离开"));
    void showOpeningPrisonStory();
    void showStoryMessage(const QString& message);
    void showNPCDialog(int x, int y);
    void showShopDialog(int x, int y);
    void showNotebookDialog();
    void showModifier();
    void showSettings();
    void showSaveLoadDialog(bool initialSave);
    void captureUndoSnapshot();
    void clearUndoHistory();
    void undoLastAction();
    void quickSave();
    void quickLoad();
    void restartGame();
    void gameOver();
    void gameWin();
    QString getItemDescription(const Item* item) const;
    void showBattleFeedback(const QString& message);
    void showTransientFeedback(const QString& message);
    void startMonsterMovementAnimation();
    void handleTeleportResult(int x, int y, int floorBefore,
                              const QString& pickedName,
                              const QString& pickedDescription,
                              Game::MoveResult result);
    void completePendingTeleport();
    void flushPendingMove();

    Game* m_game;
    QTimer m_battleFeedbackTimer;
    QTimer m_movementQueueTimer;
    HeldMoveState m_heldMoveState;
    int m_pendingMoveDx = 0;
    int m_pendingMoveDy = 0;
    bool m_hasPendingMove = false;
    struct PendingTeleport {
        bool active = false;
        int x = 0;
        int y = 0;
        int floorBefore = 0;
        QString pickedName;
        QString pickedDescription;
    };
    PendingTeleport m_pendingTeleport;
    bool m_adminMode = false;
    bool m_battleFeedbackEnabled = true;
    std::vector<std::unique_ptr<QTemporaryFile>> m_undoHistory;
    bool m_prisonReturnStoryShown = false;
    bool m_floor33TrapStoryShown = false;
    int m_floor32KnightStoryFloor = -1;
    bool m_floor42CaptureStoryQueued = false;
    bool m_playFirstFloorOpening = false;
    Ui::MainWindow ui;
};
