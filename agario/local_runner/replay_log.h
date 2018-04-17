#ifndef REPLAY_LOG_H
#define REPLAY_LOG_H

#include <QFile>
#include <QProcessEnvironment>
#include <QMap>
#include <QPair>
#include <QQueue>

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
        return commands_.value(qMakePair(tick, player_id), Direct(0, 0));
    }

    QPair<double, double> get_player_pos(int tick, const QString& id) {
        return player_pos_[qMakePair(tick, id)];
    }

    QPair<double, double> get_point(int tick, const QString& type) {
        return points_[qMakePair(tick, type)].dequeue();
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
