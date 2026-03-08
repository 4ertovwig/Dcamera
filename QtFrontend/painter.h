/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <QObject>
#include <QImage>

class Painter : public QObject
{
    Q_OBJECT

public:
    Painter();
    ~Painter();

signals:
    void run();
    void readyFrame(const QImage &image);

public slots:
    void getFrames();
};
