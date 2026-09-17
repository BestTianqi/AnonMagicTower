#pragma once

#include <QWidget>
#include "Game/Game.h"
#include "ui_MainWindow.h"
#include <QTimer>
#include <QString>
#include <vector>
#include <unordered_set>
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
    void showInventory(int focusIndex = -1);
    void activateItem(int index);
    void updateItemPanel();
    void showOpeningFloorStory(int fromFloor, int toFloor);
    void showFloor20VampireStoryIfNeeded(int floorBefore);
    void showFloor10AmbushStoryIfNeeded();
    void showFloor33TrapStoryIfNeeded(int floorBefore);
    void showFloor32KnightStoryIfNeeded(int floorBefore);
    void showFloor32KnightStoryAfterMovement(int floorBefore);
    void showFloor42CaptureStory();
    void showPendingApproachHazardCgs();
    void showPrisonTrapPrompt();
    void showFloor3PrisonVisualNovel();
    void showVisualNovelDialogue(const std::vector<VisualNovelPage>& pages);
    void showFirstFloorOpeningStory();
    bool showVisualNovelChoice(const QString& speaker, const QString& text,
                               const QString& portrait, const QString& yesText = QString::fromUtf8("确定"),
                               const QString& noText = QString::fromUtf8("离开"));
    void showOpeningPrisonStory();
    void showStoryMessage(const QString& message);
    void showNPCDialog(int x, int y);
    void showShopDialog(int x, int y);
    void showModifier();
    void showSettings();
    void showSaveLoadDialog(bool initialSave);
    void captureUndoSnapshot();
    void clearUndoHistory();
    void undoLastAction();
    void quickSave();
    void quickLoad();
    void gameOver();
    void gameWin();
    QString getItemDescription(const Item* item) const;
    void showBattleFeedback(const QString& message);
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
    bool m_floor2OpeningShown = false;
    bool m_floor3OpeningShown = false;
    bool m_prisonReturnStoryShown = false;
    bool m_floor33TrapStoryShown = false;
    int m_floor32KnightStoryFloor = -1;
    bool m_floor42CaptureStoryQueued = false;
    bool m_playFirstFloorOpening = false;
    std::unordered_set<int> m_floorStoriesShown;
    Ui::MainWindow ui;
};
