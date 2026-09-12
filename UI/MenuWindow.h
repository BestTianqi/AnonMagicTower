#pragma once

#include <QWidget>
#include <QPixmap>
#include "ui_MenuWindow.h"

class MainWindow;

class MenuWindow : public QWidget {
    Q_OBJECT
public:
    explicit MenuWindow(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onNewGame();
    void onLoadGame();
    void onMapEditor();
    void onSettings();

private:
    void enterGame(class Game* game);

    Ui::MenuWindow ui;
    QPixmap m_backgroundImage;
    MainWindow* m_gameWindow = nullptr;
};
