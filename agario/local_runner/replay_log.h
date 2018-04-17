#ifndef REPLAY_LOG_H
#define REPLAY_LOG_H

#include <QFile>
#include <QProcessEnvironment>
#include <QMap>
#include <QPair>
#include <QQueue>
#include <memory>

class ReplayLog {
public:
    explicit ReplayLog(QString log_txt):
        env_(QProcessEnvironment::systemEnvironment()),
        commands_()
    {
        QFile file(log_txt);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "cannot read" << log_txt;
            return;
        }
        QTextStream in(&file);
        int tick = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            auto parts = line.splitRef(" ");
            if (line.startsWith("# Dynamic params ")) {
                for (auto& part : parts.mid(3)) {
                    auto kv = part.split("=");
                    env_.insert(kv[0].toString(), kv[1].toString());
                }
            } else if (line.startsWith("T")) {
                tick = line.mid(1).toInt();
            } else if (line.startsWith("C")) {
                auto player_id = parts[0].mid(1, 1).toInt();
                auto x = parts[1].mid(1).toDouble();
                auto y = parts[2].mid(1).toDouble();
                Direct command(x, y);
                if (parts.last() == "S") {
                    command.split = true;
                } else if (parts.last() == "E") {
                    command.eject = true;
                }
                commands_.insert(qMakePair(tick, player_id), command);
            } else if (line.startsWith("+P") && parts[1].startsWith("X")) {
                auto id = parts[0].mid(2).toString();
                auto x = parts[1].mid(1).toDouble();
                auto y = parts[2].mid(1).toDouble();
                player_pos_.insert(qMakePair(tick, id), qMakePair(x, y));
            } else if (line.startsWith("AF") ||
                    line.startsWith("AV") ||
                    line.startsWith("AP")) {
                auto type = line.left(2);
                auto x = parts[1].mid(1).toDouble();
                auto y = parts[2].mid(1).toDouble();
                points_[qMakePair(tick, type)].enqueue(qMakePair(x, y));
            }
        }
    }

    Direct get_command(int tick, int player_id) const {
        auto key = qMakePair(tick, player_id);
        if (!commands_.contains(key)) {
            qFatal("no key %d %d", tick, player_id);
        }
        return commands_.value(key, Direct(0, 0));
    }

    std::unique_ptr<QPair<double, double>> get_player_pos(int tick, const QString& id) {
        auto key = qMakePair(tick, id);
        auto it = player_pos_.constFind(key);
        if (it == player_pos_.constEnd()) {
            return nullptr;
        }
        return std::unique_ptr<QPair<double, double>>(new QPair<double, double>(it->first, it->second));
    }

    QPair<double, double> get_point(int tick, const QString& type) {
        auto key = qMakePair(tick, type);
        if (!points_.contains(key)) {
            qFatal("no points %d %s", tick, type.toUtf8().constData());
        }
        return points_[key].dequeue();
    }

    QProcessEnvironment env() const {
        return env_;
    }

private:
    QProcessEnvironment env_;
    QMap<QPair<int, int>, Direct> commands_;
    QMap<QPair<int, QString>, QPair<double, double>> player_pos_;
    QMap<QPair<int, QString>, QQueue<QPair<double, double>>> points_;
};

#endif // REPLAY_LOG_H
