#pragma once

#include <QWidget>
#include "Game/Game.h"
#include "ui_MainWindow.h"

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(Game* game, QWidget* parent = nullptr);
    void loadAssets();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void updateHUD();
    void showInventory();
    void showNPCDialog(int x, int y);
    QString getItemDescription(const Item* item) const;

    Game* m_game;
    Ui::MainWindow ui;
};
