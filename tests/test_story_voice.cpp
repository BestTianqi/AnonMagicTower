#include "Audio/StoryVoice.h"
#include "UI/StoryScript.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFileInfo>
#include <cassert>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const QString anon = QString::fromUtf8("千早爱音");
    const QString soyo = QString::fromUtf8("长崎素世");
    const QString line = QString::fromUtf8("素世，你在哪里？");
    assert(storyVoiceModel(anon) == QStringLiteral("aiyi"));
    assert(storyVoiceModel(soyo) == QStringLiteral("sushi"));
    assert(storyVoiceModel(QString::fromUtf8("小长崎素世")) == QStringLiteral("sushi"));
    assert(storyVoiceModel(QString::fromUtf8("旁白")) == QStringLiteral("deng"));
    assert(storyVoiceModel(QString::fromUtf8("米歇尔")).isEmpty());
    const QByteArray expected = QCryptographicHash::hash(
        (anon + QStringLiteral("\n") + line).toUtf8(), QCryptographicHash::Sha1).toHex();
    assert(storyVoiceFilename(anon, line) == QString::fromLatin1(expected) + QStringLiteral(".wav"));
    assert(storyVoiceFilename(soyo, line) != storyVoiceFilename(anon, line));
    for (int flags = 0; flags < 16; ++flags) {
        const StoryContext context{bool(flags & 1), bool(flags & 2),
                                   bool(flags & 4), bool(flags & 8)};
        for (int scene = 1; scene <= 31; ++scene) {
            for (const auto& item : storyScene(scene, context)) {
                const QString speaker = QString::fromUtf8(item.speaker);
                if (storyVoiceModel(speaker).isEmpty()) continue;
                const QString clip = QStringLiteral(MOTA_SOURCE_DIR "/Audio/voice/")
                    + storyVoiceFilename(speaker, QString::fromUtf8(item.text));
                assert(QFileInfo::exists(clip));
            }
        }
    }
    return 0;
}
