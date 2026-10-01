#pragma once

#include <QCryptographicHash>
#include <QString>

inline QString storyVoiceModel(const QString& speaker) {
    if (speaker.contains(QString::fromUtf8("爱音"))) return QStringLiteral("aiyi");
    if (speaker.contains(QString::fromUtf8("素世"))) return QStringLiteral("sushi");
    if (speaker == QString::fromUtf8("旁白")) return QStringLiteral("deng");
    return {};
}

inline QString storyVoiceFilename(const QString& speaker, const QString& text) {
    const QByteArray key = (speaker + QStringLiteral("\n") + text).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha1).toHex())
        + QStringLiteral(".wav");
}
