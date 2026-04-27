#pragma once

#include <QWidget>
#include "Game/Game.h"
#include "ui_MainWindow.h"

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(Game* game, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    Game* m_game;
    Ui::MainWindow ui;
};
