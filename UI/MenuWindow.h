#pragma once

#include <QWidget>
#include "ui_MenuWindow.h"

class MainWindow;

class MenuWindow : public QWidget {
    Q_OBJECT
public:
    explicit MenuWindow(QWidget* parent = nullptr);

private slots:
    void onNewGame();
    void onLoadGame();
    void onMapEditor();
    void onSettings();

private:
    void enterGame(class Game* game);

    Ui::MenuWindow ui;
    MainWindow* m_gameWindow = nullptr;
};
