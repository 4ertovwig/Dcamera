/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "main.h"
#include "camerawidget.h"
#include "painter.h"
#include "videostream.h"
#include "audiostream.h"
#include "../sync.h"
#include "../common/args.h"

extern "C"
{
    extern AudioStreamProvider ifaceAudio;
    SharedVideoFrame *sharedFrame;
    extern CamParams camParams;
    SynchronizeData syncData = {
        .stopStream = false,
        .frameReady = false};
}

int run(int argc, char **argv)
{
    try
    {
        // VideoFrame shared for all components in graphical pipeline
        std::unique_ptr<SharedVideoFrame> frameMemory{new SharedVideoFrame};
        sharedFrame = frameMemory.get();
        memset(sharedFrame, 0, sizeof(SharedVideoFrame));
        // first surface in double video buffering will be 0
        sharedFrame->videobufSurface = 0;

#ifdef AUDIO_BACKEND_ALSA
        signal(SIGINT, (void (*)(int))ifaceAudio.cbSigint);
        qDebug("Set SIGINT callback for ALSA\n...");
#endif

        QApplication app(argc, argv);
        QWidget window;
        QVBoxLayout layout(&window);
        QPushButton btn{"Stop"};

        Painter painter;
        VideoStream videostream{camParams};
        AudioStream audiostream{camParams};

        CameraWidget сameraWidget{&painter, &videostream, &audiostream, &btn};

        layout.addWidget(&сameraWidget);
        layout.addWidget(&btn);
        window.show();

        return app.exec();
    }
    catch (const std::bad_alloc &e)
    {
        qDebug() << "Allocation error. Exit\n";
        return EXIT_FAILURE;
    }
    catch (const std::exception &e)
    {
        qDebug() << "Exception... " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}

#include "main.moc"
