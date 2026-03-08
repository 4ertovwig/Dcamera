/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once
#include <QObject>

#include "../common/args.h"
#include "../audio/common.h"

class AudioStream : public QObject
{
    Q_OBJECT
public:
    AudioStream(CamParams &camParams)
        : m_camParams(camParams)
    {
    }

public slots:
    void start();
    void stopped();

private:
    bool m_stopped = false;
    CamParams &m_camParams;
};
