#include <QApplication>
#include <QIcon>
#include "UI/MenuWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/images/runtime/ui/bangdream_ournotes_app_icon.jpg")));

    MenuWindow menu;
    menu.show();

    return app.exec();
}
