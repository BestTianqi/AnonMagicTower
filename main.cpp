#include <QApplication>
#include "Game/Game.h"
#include "UI/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Game game;
    game.loadDefaultMap();

    MainWindow w(&game);
    w.loadAssets();
    w.show();

    return app.exec();
}
