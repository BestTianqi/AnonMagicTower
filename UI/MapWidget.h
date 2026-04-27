#pragma once

#include <QWidget>
#include "Game/Game.h"

class MapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MapWidget(Game* game, QWidget* parent = nullptr);
    void setGame(Game* game) { m_game = game; }
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Game* m_game;
    int m_tileSize = 48;
};
