#pragma once

#include <QWidget>
#include "Game/Game.h"
#include "ui_MainWindow.h"
#include <QTimer>

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(Game* game, QWidget* parent = nullptr);
    void loadAssets();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void updateHUD();
    void updateMonsterPanel();
    void showInventory();
    void showOpeningFloorStory(int fromFloor, int toFloor);
    void showPrisonTrapPrompt();
    void showOpeningPrisonStory();
    void showStoryMessage(const QString& message);
    void showNPCDialog(int x, int y);
    void showShopDialog(int x, int y);
    void showModifier();
    void gameOver();
    void gameWin();
    QString getItemDescription(const Item* item) const;
    void showBattleFeedback(const QString& message);
    void flushPendingMove();

    Game* m_game;
    QTimer m_battleFeedbackTimer;
    QTimer m_movementQueueTimer;
    int m_pendingMoveDx = 0;
    int m_pendingMoveDy = 0;
    bool m_hasPendingMove = false;
    bool m_floor2OpeningShown = false;
    bool m_floor3OpeningShown = false;
    Ui::MainWindow ui;
};
