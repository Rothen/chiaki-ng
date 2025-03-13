#ifndef CHIAKI_PY_SESSIONLOG_H
#define CHIAKI_PY_SESSIONLOG_H

#include <chiaki/log.h>
#include <mutex>
#include <fstream>
#include <string>

class StreamSession;

class SessionLog
{
    friend class SessionLogPrivate;

    private:
        StreamSession *session;
        ChiakiLog log;
        std::ofstream *file;
        std::mutex file_mutex;

        void Log(ChiakiLogLevel level, const char *msg);

    public:
        SessionLog(StreamSession *session, uint32_t level_mask, const std::string &filename);
        ~SessionLog();

        ChiakiLog *GetChiakiLog() { return &log; }
};

std::string GetLogBaseDir();
std::string CreateLogFilename();

#endif // CHIAKI_PY_SESSIONLOG_H