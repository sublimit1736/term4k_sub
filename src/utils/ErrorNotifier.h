#pragma once

#include <exception>
#include <functional>
#include <string>

class ErrorNotifier {
public:
    enum class Level { Info, Warning, Error };

    using Sink = std::function<void(Level, const std::string &)>;

    static void notify(const std::string &message);

    static void notify(const std::string &context, const std::string &message);

    static void notifyInfo(const std::string &message);

    static void notifyWarning(const std::string &message);

    static void notifyWarning(const std::string &context, const std::string &message);

    static void notifyException(const std::string &context, const std::exception &ex);

    static void notifyUnknown(const std::string &context);

    // Allows future UI/log backends to replace stderr output centrally.
    static void setSink(Sink sink);

    // Returns the currently installed sink.
    static Sink getSink();
};
